/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_da.h"
#include "val_host_rmi_sro.h"
#include "val_host_doe.h"
#include "val_host_pcie_spec.h"
#include "val_host_pcie.h"
#include "val_crypto.h"
#include "pal_common_support.h"

static uint64_t g_active_psmmu;
static uint32_t g_active_psmmu_users;

static uint64_t val_host_pdev_doe_communicate(val_host_pdev_ts *pdev_obj,
                            val_host_dev_comm_enter_ts *dev_comm_enter,
                              val_host_dev_comm_exit_ts *dev_comm_exit)
{
    size_t resp_len;
    uint64_t rc;

    resp_len = 0UL;
    rc = val_host_pcie_doe_communicate(pdev_obj->bdf,
                  pdev_obj->doe_cap_base,
                  (void *)dev_comm_enter->req_addr,
                  dev_comm_exit->req_len,
                  (void *)dev_comm_enter->rsp_addr, GRANULE_SIZE,
                  &resp_len, dev_comm_exit->protocol);

    /*
     * Set dev_comm_enter args for next pdev_communicate. Upon
     * success or error call pdev_communicate
     */
    if (rc == 0) {
        dev_comm_enter->status = RMI_DEV_COMM_RESPONSE;
        dev_comm_enter->rsp_len = resp_len;
    } else {
        dev_comm_enter->status = RMI_DEV_COMM_ERROR;
        dev_comm_enter->rsp_len = 0;
        rc = VAL_ERROR;
    }

    return rc;
}

static uint64_t val_host_pdev_cache_device_object(val_host_pdev_ts *pdev_obj,
                                                     uint8_t dev_obj_id,
                                             const uint8_t *dev_obj_buf,
                                          unsigned long dev_obj_buf_len)
{
    uint64_t rc = VAL_ERROR;

    /*
     * During PDEV communicate device object of type certificate or VCA is
     * cached
     */
    if (dev_obj_id == RMI_DEV_CERTIFICATE) {
        if ((pdev_obj->cert_chain_len > VAL_HOST_PDEV_CERT_LEN_MAX) ||
            (dev_obj_buf_len >
             (VAL_HOST_PDEV_CERT_LEN_MAX - pdev_obj->cert_chain_len))) {
            return VAL_ERROR;
        }

        LOG(DBG, "\tcache_cert: offset: 0x%lx, len: 0x%lx\n",
              pdev_obj->cert_chain_len, dev_obj_buf_len);

        val_memcpy((void *)(pdev_obj->cert_chain + pdev_obj->cert_chain_len),
                                           dev_obj_buf, dev_obj_buf_len);
        pdev_obj->cert_chain_len += dev_obj_buf_len;
        rc = 0;
    } else if (dev_obj_id == RMI_DEV_VCA) {
        if ((pdev_obj->vca_len > VAL_HOST_PDEV_VCA_LEN_MAX) ||
            (dev_obj_buf_len >
             (VAL_HOST_PDEV_VCA_LEN_MAX - pdev_obj->vca_len))) {
            return VAL_ERROR;
        }

        LOG(DBG, "\tvca: offset: 0x%lx, len: 0x%lx\n",
             pdev_obj->vca_len, dev_obj_buf_len);

        val_memcpy((void *)(pdev_obj->vca + pdev_obj->vca_len),
               dev_obj_buf, dev_obj_buf_len);
        pdev_obj->vca_len += dev_obj_buf_len;
        rc = 0;
    }
    return rc;
}

static uint64_t val_host_vdev_cache_device_object(val_host_vdev_ts *vdev_obj,
                     uint8_t dev_obj_id,
                     const uint8_t *dev_obj_buf,
                     unsigned long dev_obj_buf_len)
{
    uint64_t rc = VAL_ERROR;

    /*
     * During VDEV communicate either measurement or interface report is
     * cached
     */
    if (dev_obj_id == RMI_DEV_MEASUREMENTS) {
        if ((vdev_obj->meas_len > VAL_HOST_VDEV_MEAS_LEN_MAX) ||
            (dev_obj_buf_len >
             (VAL_HOST_VDEV_MEAS_LEN_MAX - vdev_obj->meas_len))) {
            return VAL_ERROR;
        }

        LOG(DBG, "\toffset: 0x%lx, len: 0x%lx\n",
             vdev_obj->meas_len, dev_obj_buf_len);

        val_memcpy((void *)(vdev_obj->meas + vdev_obj->meas_len), dev_obj_buf,
               dev_obj_buf_len);
        vdev_obj->meas_len += dev_obj_buf_len;
        rc = 0;
    } else if (dev_obj_id == RMI_DEV_INTERFACE_REPORT) {
        if ((vdev_obj->ifc_report_len > VAL_HOST_VDEV_IFC_REPORT_LEN_MAX) ||
            (dev_obj_buf_len >
             (VAL_HOST_VDEV_IFC_REPORT_LEN_MAX - vdev_obj->ifc_report_len))) {
            return VAL_ERROR;
        }

        LOG(DBG, "\tcache_ifc_report: offset: 0x%lx, len: 0x%lx\n",
             vdev_obj->ifc_report_len, dev_obj_buf_len);

        val_memcpy((void *)(vdev_obj->ifc_report + vdev_obj->ifc_report_len),
               dev_obj_buf, dev_obj_buf_len);
        vdev_obj->ifc_report_len += dev_obj_buf_len;
        rc = 0;
    }

    return rc;
}

static uint64_t val_host_dev_cache_dev_object(val_host_pdev_ts *pdev_obj,
                     val_host_vdev_ts *vdev_obj,
                     uint8_t *dev_obj_buf,
                     unsigned char cache_object_id,
                     size_t cache_offset, size_t cache_len)
{
    uint8_t *dev_obj_cache;
    uint64_t rc;

    if ((cache_len != 0) &&
        ((cache_offset + cache_len) > GRANULE_SIZE)) {
        LOG(ERROR, "Invalid cache offset/length\n");
        return VAL_ERROR;
    }

    dev_obj_cache = dev_obj_buf + cache_offset;

    if (vdev_obj) {
        rc = val_host_vdev_cache_device_object(vdev_obj,
                           cache_object_id,
                           dev_obj_cache,
                           cache_len);
    } else {
        rc = val_host_pdev_cache_device_object(pdev_obj,
                           cache_object_id,
                           dev_obj_cache,
                           cache_len);
    }

    if (rc != 0) {
        LOG(ERROR, "host_dev_cache_device_object failed %x \n", cache_object_id);
    }

    return rc;
}

static uint64_t val_host_dev_cache_dev_req_resp(val_host_pdev_ts *pdev_obj,
                    val_host_vdev_ts *vdev_obj,
                       val_host_dev_comm_enter_ts *dev_comm_enter,
                       val_host_dev_comm_exit_ts *dev_comm_exit)
{
    uint64_t rc;

    if ((dev_comm_exit->req_cache_len == 0) &&
        (dev_comm_exit->rsp_cache_len == 0)) {
        LOG(ERROR, "Both cache_req_len and cache_rsp_len are 0\n");
        return VAL_ERROR;
    }

    rc = val_host_dev_cache_dev_object(pdev_obj, vdev_obj,
                       (uint8_t *)dev_comm_enter->req_addr,
                       dev_comm_exit->cache_object_id,
                       dev_comm_exit->req_cache_offset,
                       dev_comm_exit->req_cache_len);

    if (rc != 0) {
        LOG(ERROR, "host_dev_cache_device_object req failed\n");
        return VAL_ERROR;
    }

    rc = val_host_dev_cache_dev_object(pdev_obj, vdev_obj,
                       (uint8_t *)dev_comm_enter->rsp_addr,
                       dev_comm_exit->cache_object_id,
                       dev_comm_exit->rsp_cache_offset,
                       dev_comm_exit->rsp_cache_len);

    if (rc != 0) {
        LOG(ERROR, "host_dev_cache_device_object rsp failed\n");
    }

    return rc;
}

static uint64_t val_host_rmi_dev_communicate(val_host_realm_ts *realm, val_host_pdev_ts *pdev_obj,
                                                                    val_host_vdev_ts *vdev_obj)
{
    val_smc_param_ts rets;

    if (vdev_obj != NULL) {

        rets = val_host_rmi_vdev_communicate((uint64_t)realm->rd,
                                             (uint64_t)vdev_obj->pdev,
                                             (uint64_t)vdev_obj->vdev,
                                             (uint64_t)vdev_obj->dev_comm_data);
        return (uint64_t)rets.x0;
    } else {

        rets = val_host_rmi_pdev_communicate((uint64_t)pdev_obj->pdev,
                                             (uint64_t)pdev_obj->dev_comm_data);
        return (uint64_t)rets.x0;
    }
}

static uint64_t val_host_vdev_get_state(val_host_vdev_ts *vdev_obj, uint64_t *state)
{
    val_smc_param_ts rets;

    if ((vdev_obj == NULL) || (state == NULL)) {
        return VAL_ERROR;
    }

    rets = val_host_rmi_vdev_get_state((uint64_t)vdev_obj->vdev);

    if ((uint64_t)rets.x0 != (uint64_t)RMI_SUCCESS) {
        return VAL_ERROR;
    }

    *state = (uint64_t)rets.x1;
    return 0;
}

static uint64_t val_host_pdev_get_state(val_host_pdev_ts *pdev_obj, uint64_t *state)
{
    val_smc_param_ts rets;

    if ((pdev_obj == NULL) || (state == NULL)) {
        return VAL_ERROR;
    }

    rets = val_host_rmi_pdev_get_state((uint64_t)pdev_obj->pdev);

    if ((uint64_t)rets.x0 != (uint64_t)RMI_SUCCESS) {
        return VAL_ERROR;
    }

    *state = (uint64_t)rets.x1;
    return 0;
}

static uint64_t val_host_dev_get_state(val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj,
                                                                      uint64_t *state)
{
    if (vdev_obj) {
        return val_host_vdev_get_state(vdev_obj, state);
    } else {
        return val_host_pdev_get_state(pdev_obj, state);
    }
}

bool val_host_dev_comm_complete(uint64_t flags)
{
    const uint64_t action_mask =
        RMI_DEV_COMM_EXIT_FLAGS_REQ_CACHE_BIT |
        RMI_DEV_COMM_EXIT_FLAGS_RSP_CACHE_BIT |
        RMI_DEV_COMM_EXIT_FLAGS_REQ_SEND_BIT |
        RMI_DEV_COMM_EXIT_FLAGS_RSP_WAIT_BIT |
        RMI_DEV_COMM_EXIT_FLAGS_RSP_RESET_BIT |
        RMI_DEV_COMM_EXIT_FLAGS_MULTI_BIT |
        RMI_DEV_COMM_EXIT_FLAGS_STREAM_WAIT_BIT;

    return (flags & action_mask) == 0U;
}

uint64_t val_host_dev_communicate_error(val_host_realm_ts *realm,
                             val_host_pdev_ts *pdev_obj,
                             val_host_vdev_ts *vdev_obj)
{
    val_host_dev_comm_enter_ts *dev_comm_enter;
    val_host_dev_comm_exit_ts *dev_comm_exit;
    uint64_t rc;
    uint64_t ret;

    if ((pdev_obj == NULL) || (vdev_obj == NULL))
        return VAL_ERROR;

    dev_comm_enter = &vdev_obj->dev_comm_data->enter;
    dev_comm_exit = &vdev_obj->dev_comm_data->exit;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    do {
        ret = val_host_rmi_dev_communicate(realm, pdev_obj, vdev_obj);
        if (ret != RMI_SUCCESS)
            return VAL_ERROR;

        if ((dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_REQ_CACHE_BIT) ||
            (dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_RSP_CACHE_BIT)) {
            rc = val_host_dev_cache_dev_req_resp(pdev_obj, vdev_obj,
                                                  dev_comm_enter,
                                                  dev_comm_exit);
            if (rc != VAL_SUCCESS)
                return VAL_ERROR;
        }

        if (dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_REQ_SEND_BIT) {
            rc = val_host_pdev_doe_communicate(pdev_obj, dev_comm_enter,
                                                dev_comm_exit);
            if (rc != VAL_SUCCESS)
                return VAL_ERROR;

            dev_comm_enter->status = RMI_DEV_COMM_ERROR;
            dev_comm_enter->rsp_len = 0;
            ret = val_host_rmi_dev_communicate(realm, pdev_obj, vdev_obj);
            return (ret == RMI_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
        }

        dev_comm_enter->status = RMI_DEV_COMM_NONE;
    } while (!val_host_dev_comm_complete(dev_comm_exit->flags));

    return VAL_ERROR;
}

static uint64_t val_host_dev_communicate_to_state(val_host_realm_ts *realm,
                             val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj,
                             unsigned char target_state, bool stop_at_key_state)
{
    uint64_t rc;
    uint64_t state;
    uint64_t error_state;
    uint64_t ret;
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_exit_ts *dev_comm_exit = NULL;
    bool started_in_error_state;
    bool stop;

    if (pdev_obj == NULL) {
        return VAL_ERROR;
    }

    if (vdev_obj) {
        dev_comm_enter = &vdev_obj->dev_comm_data->enter;
        dev_comm_exit = &vdev_obj->dev_comm_data->exit;

        error_state = RMI_VDEV_ERROR;
    } else {
        dev_comm_enter = &pdev_obj->dev_comm_data->enter;
        dev_comm_exit = &pdev_obj->dev_comm_data->exit;

        error_state = RMI_PDEV_ERROR;
    }

    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    rc = val_host_dev_get_state(pdev_obj, vdev_obj, &state);
    if (rc != 0) {
        return rc;
    }
    started_in_error_state = (state == error_state);

    do {
        ret = val_host_rmi_dev_communicate(realm, pdev_obj, vdev_obj);
        if (ret != RMI_SUCCESS) {
            LOG(ERROR, "host_rmi_dev_communicate failed\n");
            rc = VAL_ERROR;
            break;
        }

        /*
         * If cache is set, then the corresponding buffer(s) has the
         * device object to be cached.
         */
        if ((dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_REQ_CACHE_BIT) ||
            (dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_RSP_CACHE_BIT))
        {

            rc = val_host_dev_cache_dev_req_resp(pdev_obj, vdev_obj,
                               dev_comm_enter, dev_comm_exit);
            if (rc != 0) {
                LOG(ERROR, "host_dev_cache_dev_object failed\n");
                break;
            }
        }

        /* Send request to PDEV's DOE and get response */
        if (dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_REQ_SEND_BIT) {
            rc = val_host_pdev_doe_communicate(pdev_obj, dev_comm_enter,
                               dev_comm_exit);
            if (rc != 0) {
                /* Report the transport error to complete the active operation. */
                LOG(WARN, "Device communication failed; reporting it to RMM\n");
                rc = VAL_SUCCESS;
            }
        } else {
            dev_comm_enter->status = RMI_DEV_COMM_NONE;
        }

        rc = val_host_dev_get_state(pdev_obj, vdev_obj, &state);
        if (rc != 0) {
            break;
        }
        if (dev_comm_exit->flags & RMI_DEV_COMM_EXIT_FLAGS_STREAM_WAIT_BIT) {
            stop = true;
        } else if (state == target_state) {
            /* The target state was reached, but for some
             * transitions this is not enough, need to continue
             * calling it till certain flags are cleared in the
             * exit. wait for that to happen.
             */
            stop = val_host_dev_comm_complete(dev_comm_exit->flags);
        } else if (stop_at_key_state && (vdev_obj != NULL) &&
                   ((state == RMI_VDEV_KEY_REFRESH) ||
                    (state == RMI_VDEV_KEY_PURGE))) {
            /* Let teardown perform the requested stream key operation. */
            stop = val_host_dev_comm_complete(dev_comm_exit->flags);
        } else if ((state == error_state) && !started_in_error_state) {
            LOG(ERROR, "Failed to reach target_state, current state: %lu \
                expected state: instead of %u\n", state, (unsigned int)target_state);
            rc = VAL_ERROR;
            stop = true;
        } else {
            stop = false;
        }
    } while (!stop);

    return rc;
}

uint64_t val_host_dev_communicate(val_host_realm_ts *realm, val_host_pdev_ts *pdev_obj,
                             val_host_vdev_ts *vdev_obj, unsigned char target_state)
{
    return val_host_dev_communicate_to_state(realm, pdev_obj, vdev_obj,
                                              target_state, false);
}

static uint32_t val_host_vdev_pdev_stream_teardown(val_host_pdev_ts *pdev_obj,
                val_host_vdev_ts *vdev_obj)
{
    if (vdev_obj == NULL)
        return VAL_ERROR;

    if (vdev_obj->pdev_stream == NULL)
        return VAL_SUCCESS;

    if (pdev_obj == NULL)
        return VAL_ERROR;

    if (val_host_pdev_stream_teardown(pdev_obj, vdev_obj->pdev_stream))
        return VAL_ERROR;

    val_host_mem_free(vdev_obj->pdev_stream);
    vdev_obj->pdev_stream = NULL;

    return VAL_SUCCESS;
}

static uint32_t val_host_vdev_teardown_stream_keys(val_host_pdev_ts *pdev_obj,
                val_host_vdev_ts *vdev_obj, uint64_t vdev_state)
{
    val_host_pdev_stream_ts *stream = vdev_obj->pdev_stream;
    val_smc_param_ts args;

    if ((stream == NULL) || (stream->stream_handle == 0U)) {
        LOG(ERROR, "No PDEV stream available for VDEV key operation\n");
        return VAL_ERROR;
    }

    if (vdev_state == RMI_VDEV_KEY_REFRESH) {
        args = val_host_rmi_pdev_stream_key_refresh(pdev_obj->pdev,
                    stream->rp_pdev_obj.pdev, stream->stream_handle);
    } else if (vdev_state == RMI_VDEV_KEY_PURGE) {
        args = val_host_rmi_pdev_stream_key_purge(pdev_obj->pdev,
                    stream->rp_pdev_obj.pdev, stream->stream_handle);
    } else {
        return VAL_ERROR;
    }

    if (args.x0 != RMI_SUCCESS)
        return VAL_ERROR;

    if (val_host_pdev_stream_communicate(pdev_obj, &stream->rp_pdev_obj))
        return VAL_ERROR;

    args = val_host_rmi_pdev_stream_complete(pdev_obj->pdev,
                                              stream->rp_pdev_obj.pdev,
                                              stream->stream_handle);
    if (args.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "PDEV stream completion failed, ret=0x%lx\n", args.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint32_t val_host_vdev_complete_unlock(val_host_realm_ts *realm,
                val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj)
{
    val_smc_param_ts args;
    uint64_t state;
    uint64_t key_state = UINT64_MAX;
    uint64_t ret;
    bool error_unlock_started = false;
    bool key_operation_attempted = false;
    unsigned int attempts;

    if (vdev_obj->dev_comm_data == NULL)
        return VAL_ERROR;

    for (attempts = 0U; attempts < 8U; attempts++) {
        args = val_host_rmi_vdev_get_state(vdev_obj->vdev);
        if (args.x0 != RMI_SUCCESS)
            return VAL_ERROR;

        state = args.x1;

        if (state == RMI_VDEV_UNLOCKED) {
            if (val_host_dev_comm_complete(vdev_obj->dev_comm_data->exit.flags))
                return VAL_SUCCESS;
        } else if (state == RMI_VDEV_ERROR) {
            if (error_unlock_started)
                return VAL_ERROR;
            args = val_host_rmi_vdev_unlock(realm->rd, vdev_obj->pdev,
                                            vdev_obj->vdev);
            if (args.x0 != RMI_SUCCESS)
                return VAL_ERROR;

            error_unlock_started = true;
            key_state = UINT64_MAX;
        } else if ((state == RMI_VDEV_KEY_REFRESH) ||
                   (state == RMI_VDEV_KEY_PURGE)) {
            if (state != key_state) {
                key_state = state;
                key_operation_attempted = false;
            }

            if (!key_operation_attempted) {
                key_operation_attempted = true;
                if (val_host_vdev_teardown_stream_keys(pdev_obj, vdev_obj, state)) {
                    LOG(ERROR, "VDEV stream key operation failed during unlock\n");
                }
            }
        } else if ((state != RMI_VDEV_STARTED) &&
                   (state != RMI_VDEV_LOCKED)) {
            return VAL_ERROR;
        }

        ret = val_host_dev_communicate_to_state(realm, pdev_obj, vdev_obj,
                                                 RMI_VDEV_UNLOCKED, true);
        if (ret != VAL_SUCCESS) {
            args = val_host_rmi_vdev_get_state(vdev_obj->vdev);
            if ((args.x0 != RMI_SUCCESS) || (args.x1 != RMI_VDEV_ERROR))
                return VAL_ERROR;
        }
    }

    LOG(ERROR, "VDEV unlock did not reach VDEV_UNLOCKED\n");
    return VAL_ERROR;
}

uint32_t val_host_vdev_teardown(val_host_realm_ts *realm,
                val_host_pdev_ts *pdev_obj,
                val_host_vdev_ts *vdev_obj)
{
    val_smc_param_ts args;
    size_t i;
    val_host_rtt_entry_ts rtte;
    uint64_t ret;
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    uint64_t flags = 0;

    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &unmap_flags, sizeof(unmap_flags));

    if (vdev_obj == NULL)
        return VAL_SUCCESS;

    if (vdev_obj->vdev == 0U)
        return val_host_vdev_pdev_stream_teardown(pdev_obj, vdev_obj);

    if ((realm == NULL) || (pdev_obj == NULL) || (vdev_obj->pdev == 0U)) {
        LOG(ERROR, "Invalid arguments provided for VDEV teardown\n");
        return VAL_ERROR;
    }

    for (i = 0; i < realm->dev_granules_mapped_count; ++i) {
        uint64_t ipa = realm->dev_granules[i].ipa;
        uint64_t size = realm->dev_granules[i].size;
        uint64_t top = ipa + size;

        if ((size == 0U) || (size % PAGE_SIZE)) {
            LOG(ERROR, "Invalid tracked VDEV_MAP region: base=0x%lx size=0x%lx\n",
                (unsigned long)ipa, (unsigned long)size);
            return VAL_ERROR;
        }

        while (ipa < top) {
            ret = val_host_rmi_rtt_read_entry(realm->rd, ipa, VAL_RTT_MAX_LEVEL, &rtte);
            if (ret)
            {
                LOG(ERROR, "rtt_read_entry failed ret = %x\n", ret);
                return VAL_ERROR;
            }
            if ((rtte.state == RMI_ASSIGNED_DEV))
            {
                ret = val_host_rmi_rtt_dev_unmap(realm->rd, ipa,
                                                 ipa + PAGE_SIZE, flags, 0);
                if (ret) {
                    LOG(ERROR, "VDEV unmap failed during teardown, ipa=0x%lx ret=0x%lx\n",
                        (unsigned long)ipa, (unsigned long)ret);
                    return VAL_ERROR;
                }
            }

            ipa += PAGE_SIZE;
        }
    }

    args = val_host_rmi_vdev_get_state(vdev_obj->vdev);
    if (args.x0) {
        LOG(ERROR, "VDEV get state failed, ret=0x%lx\n", (unsigned long)args.x0);
        return VAL_ERROR;
    }

    if (args.x1 == RMI_VDEV_NEW) {
        if (val_host_dev_communicate(realm, pdev_obj, vdev_obj,
                                     RMI_VDEV_UNLOCKED)) {
            LOG(ERROR, "VDEV initialization failed during teardown\n");
            return VAL_ERROR;
        }

        args = val_host_rmi_vdev_get_state(vdev_obj->vdev);
        if ((args.x0 != RMI_SUCCESS) || (args.x1 != RMI_VDEV_UNLOCKED)) {
            LOG(ERROR, "VDEV did not reach UNLOCKED during teardown\n");
            return VAL_ERROR;
        }
    } else if ((args.x1 == RMI_VDEV_LOCKED) ||
               (args.x1 == RMI_VDEV_STARTED)) {
        args = val_host_rmi_vdev_unlock(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
        if (args.x0 != RMI_SUCCESS) {
            LOG(ERROR, "VDEV unlock failed, ret=0x%lx addr=0x%lx\n",
                (unsigned long)args.x0, (unsigned long)args.x1);
            return VAL_ERROR;
        }

        if (val_host_vdev_complete_unlock(realm, pdev_obj, vdev_obj))
            return VAL_ERROR;
    } else if ((args.x1 == RMI_VDEV_ERROR) ||
               (args.x1 == RMI_VDEV_KEY_REFRESH) ||
               (args.x1 == RMI_VDEV_KEY_PURGE)) {
        if (val_host_vdev_complete_unlock(realm, pdev_obj, vdev_obj))
            return VAL_ERROR;
    } else if (args.x1 != RMI_VDEV_UNLOCKED) {
        return VAL_ERROR;
    }

    realm->dev_granules_mapped_count = 0;

    ret = val_host_rmi_vdev_destroy(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
    if (ret) {
        LOG(ERROR, "VDEV destroy failed, ret=0x%lx\n", (unsigned long)ret);
        return VAL_ERROR;
    }

    vdev_obj->vdev = 0;

    return val_host_vdev_pdev_stream_teardown(pdev_obj, vdev_obj);
}

uint32_t val_host_pdev_teardown(val_host_pdev_ts *pdev_obj, uint64_t pdev_ptr)
{
    val_smc_param_ts args;
    uint64_t state, ret;

    if ((pdev_obj == NULL) || (pdev_ptr == 0U)) {
        LOG(ERROR, "Invalid arguments provided for PDEV teardown\n");
        return VAL_ERROR;
    }

    args = val_host_rmi_pdev_get_state(pdev_ptr);
    if (args.x0) {
        LOG(ERROR, "PDEV get state failed during teardown, ret=0x%lx\n",
            (unsigned long)args.x0);
        return VAL_ERROR;
    }

    state = args.x1;

    if (state != RMI_PDEV_STOPPED) {
        if (state == RMI_PDEV_NEW) {
            uint64_t req_addr, resp_addr;

            /* Allocate buffer to cache VCA */
            pdev_obj->vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
            pdev_obj->vca_len = 0;
            if (pdev_obj->vca == NULL) {
                return VAL_ERROR;
            }

            /* Allocate memory for req addr buffer */
            req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            if (!req_addr)
            {
                LOG(ERROR, "Failed to allocate memory for req_addr");
                return VAL_ERROR;
            }

            /* Allocate memory for resp_addr buffer */
            resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            if (!resp_addr)
            {
                LOG(ERROR, "Failed to allocate memory for resp_addr");
                return VAL_ERROR;
            }

            pdev_obj->pdev = pdev_ptr;
            pdev_obj->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            pdev_obj->dev_comm_data->enter.req_addr = req_addr;
            pdev_obj->dev_comm_data->enter.rsp_addr = resp_addr;

        }

        if (state != RMI_PDEV_READY) {
            /* Cancel setup communication before stopping a pre-READY PDEV. */
            args = val_host_rmi_pdev_abort(pdev_ptr);
            if ((args.x0 != RMI_SUCCESS) &&
                (args.x0 != RMI_ERROR_DEVICE)) {
                LOG(ERROR, "PDEV abort failed during teardown, ret=0x%lx\n",
                    (unsigned long)args.x0);
                return VAL_ERROR;
            }
        }

        args = val_host_rmi_pdev_stop(pdev_ptr);
        if (args.x0) {
            LOG(ERROR, "PDEV stop failed, ret=0x%lx\n", (unsigned long)args.x0);
            return VAL_ERROR;
        }

        if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_STOPPED)) {
            LOG(ERROR, "PDEV communicate failed during teardown\n");
            return VAL_ERROR;
        }

        args = val_host_rmi_pdev_get_state(pdev_ptr);
        if (args.x0) {
            LOG(ERROR, "PDEV get state failed during teardown, ret=0x%lx\n",
                (unsigned long)args.x0);
            return VAL_ERROR;
        }

        if (args.x1 != RMI_PDEV_STOPPED) {
            LOG(ERROR, "Unexpected PDEV state during teardown, state=0x%lx\n",
                (unsigned long)args.x1);
            return VAL_ERROR;
        }
    }

    ret = val_host_rmi_pdev_destroy(pdev_ptr);
    if (ret) {
        LOG(ERROR, "PDEV destroy failed, ret=0x%lx\n", (unsigned long)ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint32_t val_host_pdev_stream_communicate(
                val_host_pdev_ts *pdev_obj,
                val_host_pdev_ts *rp_pdev_obj)
{
    bool pdev_complete = false;
    bool rp_pdev_complete = false;

    do
    {
        if (!pdev_complete)
        {
            if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_READY))
                return VAL_ERROR;
            pdev_complete = val_host_dev_comm_complete(
                                pdev_obj->dev_comm_data->exit.flags);
        }

        if (!rp_pdev_complete)
        {
            if (val_host_dev_communicate(NULL, rp_pdev_obj, NULL, RMI_PDEV_READY))
                return VAL_ERROR;
            rp_pdev_complete = val_host_dev_comm_complete(
                                rp_pdev_obj->dev_comm_data->exit.flags);
        }
    } while (!pdev_complete || !rp_pdev_complete);

    return VAL_SUCCESS;
}

uint32_t val_host_pdev_stream_setup(val_host_pdev_ts *pdev_obj,
                uint32_t rp_bdf,
                uint64_t psmmu,
                val_host_pdev_stream_ts *pdev_stream)
{
    val_host_rmi_pdev_params_ts *rp_pdev_params = NULL;
    val_host_rmi_pdev_stream_params_ts *stream_params = NULL;
    val_host_rmi_psmmu_params_ts *psmmu_params = NULL;
    val_host_pdev_flags_ts pdev_flags;
    val_smc_param_ts args;
    uint64_t req_addr;
    uint64_t resp_addr;
    uint64_t ret;
    uint64_t i;

    if ((pdev_obj == NULL) || (pdev_stream == NULL))
        return VAL_ERROR;

    val_memset(pdev_stream, 0, sizeof(*pdev_stream));
    pdev_stream->psmmu = psmmu;
    pdev_stream->endpoint_bdf = pdev_obj->bdf;

    pdev_stream->rp_pdev_obj.pdev =
                    (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    rp_pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((pdev_stream->rp_pdev_obj.pdev == 0U) || (rp_pdev_params == NULL))
        goto error;

    ret = val_host_rmi_granule_range_delegate(
                    pdev_stream->rp_pdev_obj.pdev,
                    pdev_stream->rp_pdev_obj.pdev + PAGE_SIZE);
    if (ret)
        goto error;
    pdev_stream->rp_pdev_delegated = 1;

    val_memset(rp_pdev_params, 0, PAGE_SIZE);
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    pdev_flags.category = RMI_PDEV_ROOT_PORT;
    val_memcpy(&rp_pdev_params->flags, &pdev_flags, sizeof(pdev_flags));
    rp_pdev_params->hb_base = val_pcie_get_ecam_base(rp_bdf);
    rp_pdev_params->pdev_id = PCIE_CREATE_BDF_PACKED(rp_bdf);
    rp_pdev_params->routing_id = PCIE_EXTRACT_BDF_SEG(rp_bdf);
    rp_pdev_params->id_index = 0;
    rp_pdev_params->rid_base = PCIE_CREATE_BDF_PACKED(rp_bdf);
    rp_pdev_params->rid_top = rp_pdev_params->rid_base + 1;

    ret = val_host_rmi_pdev_create(pdev_stream->rp_pdev_obj.pdev,
                                   (uint64_t)rp_pdev_params);
    val_host_mem_free(rp_pdev_params);
    rp_pdev_params = NULL;
    if (ret)
        goto error;
    pdev_stream->rp_pdev_created = 1;

    pdev_stream->rp_pdev_obj.bdf = rp_bdf;
    pdev_stream->rp_pdev_obj.dev_comm_data =
                    val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((pdev_stream->rp_pdev_obj.dev_comm_data == NULL) ||
        (req_addr == 0U) || (resp_addr == 0U))
        goto error;

    val_memset(pdev_stream->rp_pdev_obj.dev_comm_data, 0, PAGE_SIZE);
    pdev_stream->rp_pdev_obj.dev_comm_data->enter.req_addr = req_addr;
    pdev_stream->rp_pdev_obj.dev_comm_data->enter.rsp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, &pdev_stream->rp_pdev_obj, NULL,
                                 RMI_PDEV_READY))
        goto error;

    stream_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (stream_params == NULL)
        goto error;
    val_memset(stream_params, 0, PAGE_SIZE);
    stream_params->stream_type = RMI_PDEV_STREAM_NCOH;
    stream_params->pdev_1 = pdev_obj->pdev;
    stream_params->pdev_2 = pdev_stream->rp_pdev_obj.pdev;
    stream_params->num_addr_range = pdev_obj->ncoh_num_addr_range;
    for (i = 0; i < stream_params->num_addr_range; i++)
    {
        stream_params->addr_range[i].base =
                    pdev_obj->ncoh_addr_range[i].base;
        stream_params->addr_range[i].top =
                    pdev_obj->ncoh_addr_range[i].top;
    }

    args = val_host_rmi_pdev_stream_connect((uint64_t)stream_params);
    val_host_mem_free(stream_params);
    stream_params = NULL;
    if (args.x0)
        goto error;
    pdev_stream->stream_handle = args.x1;

    if (val_host_pdev_stream_communicate(pdev_obj,
                                         &pdev_stream->rp_pdev_obj))
        goto error;

    args = val_host_rmi_pdev_stream_complete(pdev_obj->pdev,
                                              pdev_stream->rp_pdev_obj.pdev,
                                              pdev_stream->stream_handle);
    if (args.x0)
        goto error;

    if (g_active_psmmu_users == 0U)
    {
        psmmu_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (psmmu_params == NULL)
            goto error;
        val_memset(psmmu_params, 0, PAGE_SIZE);

        ret = val_host_rmi_psmmu_activate(psmmu,
                                         (uint64_t)psmmu_params);
        val_host_mem_free(psmmu_params);
        psmmu_params = NULL;
        if (ret && (ret != RMI_ERROR_INPUT))
            goto error;

        if (ret == RMI_SUCCESS)
        {
            g_active_psmmu = psmmu;
            g_active_psmmu_users++;
            pdev_stream->psmmu_active = 1;
        }
    }
    else if (g_active_psmmu != psmmu)
    {
        goto error;
    }
    else
    {
        g_active_psmmu_users++;
        pdev_stream->psmmu_active = 1;
    }

    ret = val_host_rmi_psmmu_st_l2_create(psmmu, pdev_obj->bdf);
    if (ret && (ret != RMI_ERROR_INPUT))
        goto error;
    if (ret == RMI_SUCCESS)
        pdev_stream->psmmu_l2_created = 1;

    return VAL_SUCCESS;

error:
    if (rp_pdev_params != NULL)
        val_host_mem_free(rp_pdev_params);
    if (stream_params != NULL)
        val_host_mem_free(stream_params);
    if (psmmu_params != NULL)
        val_host_mem_free(psmmu_params);
    (void)val_host_pdev_stream_teardown(pdev_obj, pdev_stream);
    return VAL_ERROR;
}

uint32_t val_host_pdev_stream_teardown(val_host_pdev_ts *pdev_obj,
                val_host_pdev_stream_ts *pdev_stream)
{
    val_smc_param_ts args;
    uint32_t status = VAL_SUCCESS;
    uint64_t ret;

    if ((pdev_obj == NULL) || (pdev_stream == NULL))
        return VAL_ERROR;

    if (pdev_stream->stream_handle != 0U)
    {
        args = val_host_rmi_pdev_stream_disconnect(pdev_obj->pdev,
                                                   pdev_stream->rp_pdev_obj.pdev,
                                                   pdev_stream->stream_handle);
        if (args.x0 ||
            val_host_pdev_stream_communicate(pdev_obj,
                                             &pdev_stream->rp_pdev_obj))
        {
            status = VAL_ERROR;
        }
        else
        {
            args = val_host_rmi_pdev_stream_complete(pdev_obj->pdev,
                                                      pdev_stream->rp_pdev_obj.pdev,
                                                      pdev_stream->stream_handle);
            if (args.x0)
                status = VAL_ERROR;
        }
        pdev_stream->stream_handle = 0;
    }

    if (pdev_stream->rp_pdev_created)
    {
        if (val_host_pdev_teardown(&pdev_stream->rp_pdev_obj,
                                   pdev_stream->rp_pdev_obj.pdev))
            status = VAL_ERROR;
        else
            pdev_stream->rp_pdev_created = 0;
    }

    if (pdev_stream->rp_pdev_delegated && !pdev_stream->rp_pdev_created)
    {
        ret = val_host_rmi_granule_range_undelegate(
                    pdev_stream->rp_pdev_obj.pdev,
                    pdev_stream->rp_pdev_obj.pdev + PAGE_SIZE);
        if (ret)
            status = VAL_ERROR;
        else
            pdev_stream->rp_pdev_delegated = 0;
    }

    if (pdev_stream->psmmu_l2_created)
    {
        ret = val_host_rmi_psmmu_st_l2_destroy(pdev_stream->psmmu,
                                               pdev_stream->endpoint_bdf);
        if (ret)
            status = VAL_ERROR;
        else
            pdev_stream->psmmu_l2_created = 0;
    }

    if (pdev_stream->psmmu_active)
    {
        if (g_active_psmmu_users > 1U)
        {
            g_active_psmmu_users--;
            pdev_stream->psmmu_active = 0;
        }
        else
        {
            ret = val_host_rmi_psmmu_deactivate(pdev_stream->psmmu);
            if (ret)
            {
                status = VAL_ERROR;
            }
            else
            {
                g_active_psmmu_users = 0;
                g_active_psmmu = 0;
                pdev_stream->psmmu_active = 0;
            }
        }
    }

    return status;
}

void val_host_get_addr_range(val_host_pdev_ts *pdev_obj)
{
    uint32_t offset = BAR0_OFFSET;
    uint64_t i;

    pdev_obj->ncoh_num_addr_range = 0UL;
    val_memset(pdev_obj->ncoh_addr_range, 0, sizeof(pdev_obj->ncoh_addr_range));

    for (i = 0; i < NCOH_ADDR_RANGE_NUM; i++) {
        uint64_t addr, size;

        offset = val_pcie_get_bar(pdev_obj->bdf, offset, &addr, &size);
        if (size == 0ULL) {
            break;
        }

        pdev_obj->ncoh_addr_range[i].base = addr;
        pdev_obj->ncoh_addr_range[i].top = addr + size;
        pdev_obj->ncoh_num_addr_range++;

        if (offset > BAR_TYPE_0_MAX_OFFSET) {
            break;
        }
    }
}

uint64_t g_pdev_new_prep_sequence(val_host_pdev_ts *pdev_dev)
{
    uint64_t ret;
    val_host_rmi_pdev_params_ts *pdev_params;
    val_smc_param_ts args;
    uint64_t req_addr, resp_addr;

    /* Allocate and delegate PDEV */
    pdev_dev->pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pdev_dev->pdev)
    {
        LOG(ERROR, "Failed to allocate memory for pdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_range_delegate(pdev_dev->pdev, pdev_dev->pdev + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "pdev delegation failed, pdev=0x%x, ret=0x%x\n", pdev_dev->pdev, ret);
            goto exit;
        }
    }

    /* Allocate memory for params */
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for pdev_params\n");
        goto exit;
    }

    val_memset(pdev_params, 0, PAGE_SIZE_4K);

    if (pdev_dev->ncoh_num_addr_range == 0U)
        val_host_get_addr_range(pdev_dev);
    val_memcpy(&pdev_params->flags, &pdev_dev->pdev_flags,
               sizeof(pdev_dev->pdev_flags));
    pdev_params->hb_base = val_pcie_get_ecam_base(pdev_dev->bdf);
    pdev_params->pdev_id = PCIE_CREATE_BDF_PACKED(pdev_dev->bdf);
    pdev_params->routing_id = PCIE_EXTRACT_BDF_SEG(pdev_dev->bdf);
    pdev_params->id_index = 0;
    if (pdev_dev->rid_top > pdev_dev->rid_base)
    {
        pdev_params->rid_base = pdev_dev->rid_base;
        pdev_params->rid_top = pdev_dev->rid_top;
    } else {
        pdev_params->rid_base = PCIE_CREATE_BDF_PACKED(pdev_dev->bdf);
        pdev_params->rid_top = pdev_params->rid_base + 1;
    }
    pdev_params->hash_algo = RMI_HASH_SHA_256;

    /* Create pdev */
    ret = val_host_rmi_pdev_create(pdev_dev->pdev,
                                   (uint64_t)pdev_params);
    if (ret)
    {
        LOG(ERROR, "Pdev create failed\n");
        goto exit;
    }

    args = val_host_rmi_pdev_get_state(pdev_dev->pdev);
    if (args.x0)
    {
        LOG(ERROR, "Pdev get state failed\n");
        goto exit;
    }

    if (args.x1 != RMI_PDEV_NEW)
    {
        LOG(ERROR, "Pdev state should be PDEV_NEW\n");
        goto exit;
    }

    /* Allocate buffer to cache VCA */
    pdev_dev->vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_dev->vca_len = 0;
    if (pdev_dev->vca == NULL) {
        goto exit;
    }

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr");
        goto exit;
    }

    pdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev->dev_comm_data->enter.req_addr = req_addr;
    pdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;

    LOG(DBG, "\nPDEV Create is successful and PDEV state is PDEV_NEW\n");
    return pdev_dev->pdev;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}

uint64_t g_vdev_state_prep_sequence(val_host_pdev_ts *pdev_dev, val_host_vdev_ts *vdev_dev,
                                    val_host_realm_ts *realm, uint8_t vdev_state)
{
    uint64_t ret, req_addr, resp_addr, vdev;
    val_host_rmi_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    uint64_t j;
    val_host_realm_flags_ts realm_flags;
    val_host_public_key_params_ts *pubkey_params;
    int rc;
    uint8_t public_key_algo;
    val_host_vdev_measure_params_ts *vdev_measure_params;

    /* Check whether RMM supports Device Assignment. */
    if (!val_host_rmm_supports_da()) {
        LOG(ERROR, "DA feature not supported\n");
        goto exit;
    }

    pdev_dev->pdev = g_pdev_new_prep_sequence(pdev_dev);
    if (pdev_dev->pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Allocate buffer to cache device certificate */
    pdev_dev->cert_slot_id = 0;
    pdev_dev->cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_dev->cert_chain_len = 0;
    if (pdev_dev->cert_chain == NULL) {
        goto exit;
    }

    /* Allocate buffer to store extracted public key */
    pdev_dev->public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev->public_key == NULL) {
        goto exit;
    }
    pdev_dev->public_key_len = PAGE_SIZE;

    /* Allocate buffer to store public key metadata */
    pdev_dev->public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev->public_key_metadata == NULL) {
        goto exit;
    }
    pdev_dev->public_key_metadata_len = PAGE_SIZE;

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr\n");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr\n");
        goto exit;
    }

    pdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev->dev_comm_data->enter.req_addr = req_addr;
    pdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, pdev_dev, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed \n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_NEEDS_KEY\n");

    /* Get public key */
    rc = val_host_get_public_key_from_cert_chain(pdev_dev->cert_chain,
                         pdev_dev->cert_chain_len,
                         pdev_dev->public_key,
                         &pdev_dev->public_key_len,
                         pdev_dev->public_key_metadata,
                         &pdev_dev->public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0) {
        LOG(ERROR, "Get public key failed\n");
        goto exit;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
        pdev_dev->public_key_sig_algo = RMI_SIG_ECDSA_P256;
    } else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
        pdev_dev->public_key_sig_algo = RMI_SIG_ECDSA_P384;
    } else {
        pdev_dev->public_key_sig_algo = RMI_SIG_RSASSA_3072;
    }

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        goto exit;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);
    pubkey_params->key_addr = (uint64_t)pdev_dev->public_key;
    pubkey_params->metadata_addr = (uint64_t)pdev_dev->public_key_metadata;
    pubkey_params->key_len = pdev_dev->public_key_len;
    pubkey_params->metadata_len = pdev_dev->public_key_metadata_len;

    args = val_host_rmi_pdev_set_pubkey(pdev_dev->pdev, (uint64_t)pubkey_params);
    if (args.x0)
    {
        LOG(ERROR, "Pdev set key failed %lx\n", args.x0, 0);
        goto exit;
    }

    args = val_host_rmi_pdev_get_state(pdev_dev->pdev);
    if (args.x0)
    {
        LOG(ERROR, "Pdev get state failed\n");
        goto exit;
    }

    /* PDEV state should be RMI_PDEV_HAS_KEY */
    if (args.x1 != RMI_PDEV_HAS_KEY)
    {
        LOG(ERROR, "Pdev state should be PDEV_HAS_KEY\n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_HAS_KEY\n");

    if (val_host_dev_communicate(NULL, pdev_dev, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate failed \n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to RMI_PDEV_READY\n");

    vdev_dev->pdev_stream = val_host_mem_alloc(
        PAGE_SIZE, sizeof(*vdev_dev->pdev_stream));
    if ((vdev_dev->pdev_stream == NULL) ||
        val_host_pdev_stream_setup(pdev_dev, pdev_dev->root_id,
                                   PLATFORM_OVERRIDE_SMMUV3_BASE,
                                   vdev_dev->pdev_stream))
    {
        LOG(ERROR, "PDEV stream setup failed\n");
        goto exit;
    }

    val_memset(realm, 0, sizeof(*realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(realm);
    realm->vmid = vdev_dev->vmid;
    realm->max_num_vdevs = 2;
    if (vdev_dev->rec_count)
        realm->rec_count = vdev_dev->rec_count;

    val_memcpy(&realm->flags, &realm_flags, sizeof(realm->flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        goto exit;
    }

    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_range_delegate(vdev, vdev + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x", vdev, ret);
            goto exit;
        }
    }

    /* Allocate memory for vdev params */
    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params\n");
        goto exit;
    }
    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    /* Populate params */
    vdev_params->flags = 0;
    vdev_params->vdev_id = vdev_dev->vdev_id;
    vdev_params->tdi_id = (vdev_dev->num_addr_range != 0U) ? vdev_dev->tdi_id :
                          PCIE_CREATE_BDF_PACKED(pdev_dev->bdf);
    if (vdev_dev->num_addr_range != 0U)
    {
        vdev_params->num_addr_range = vdev_dev->num_addr_range;
        for (j = 0; j < vdev_params->num_addr_range; j++)
            vdev_params->addr_range[j] = vdev_dev->addr_range[j];
    } else {
        vdev_params->num_addr_range = pdev_dev->ncoh_num_addr_range;
        for (j = 0; j < vdev_params->num_addr_range; j++)
            vdev_params->addr_range[j] = pdev_dev->ncoh_addr_range[j];
    }

    /* Create vdev */
    ret = val_host_rmi_vdev_create(realm->rd, pdev_dev->pdev, vdev,
                                   (uint64_t)vdev_params);
    if (ret)
    {
        LOG(ERROR, "Vdev create failed\n");
        goto exit;
    }

    vdev_dev->vdev = vdev;
    vdev_dev->tdi_id = vdev_params->tdi_id;
    vdev_dev->vdev_id = vdev_params->vdev_id;
    vdev_dev->realm_rd = realm->rd;
    vdev_dev->pdev = pdev_dev->pdev;

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {

        LOG(ERROR, "Vdev get state failed ret %d\n", args.x0, 0);
        goto exit;
    }

    /* Check VDEV state is RMI_VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "Vdev state should be VDEV_NEW, ret %d\n", args.x1, 0);
        goto exit;
    }

    vdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    vdev_dev->dev_comm_data->enter.req_addr = req_addr;
    vdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;

    LOG(TEST, "\n\tVDEV Create is successful and VDEV state is VDEV_NEW\n");

    vdev_dev->pdev = pdev_dev->pdev;
    vdev_dev->vdev = vdev;
    vdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    vdev_dev->dev_comm_data->enter.req_addr = req_addr;
    vdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;

   if (vdev_state == RMI_VDEV_NEW)
        return vdev;

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    LOG(TEST, "\n\tVDEV state is RMI_VDEV_UNLOCKED\n");
    if (vdev_state == RMI_VDEV_UNLOCKED)
        return vdev;

    /* Allocate buffer to cache device measurements */
    vdev_dev->meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    if (vdev_dev->meas == NULL) {
        goto exit;
    }

    vdev_dev->meas_len = 0;

    /* Allocate buffer to cache device interface report */
    vdev_dev->ifc_report = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_IFC_REPORT_LEN_MAX);
    vdev_dev->ifc_report_len = 0;
    if (vdev_dev->ifc_report == NULL) {
        goto exit;
    }

    /* Allocate memory for vdev params */
    vdev_measure_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_measure_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_measure_params");
        goto exit;
    }

    val_memset(vdev_measure_params, 0, PAGE_SIZE);

    /*
     * vdev_measure_params->flags is left as 0 as we don't want signed nor
     * raw output.
     */
    /*
     * Set indices value  to (1 << 254) to retrieve all measurements
     * supported by the device.
     */
    vdev_measure_params->nonce[31] = (unsigned char)1U << 6U;

    args = val_host_rmi_vdev_get_measurements(realm->rd, vdev_dev->pdev, vdev_dev->vdev,
                        (uint64_t)vdev_measure_params);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_GET_MEASUREMENTS failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_lock(realm->rd, vdev_dev->pdev, vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_LOCK failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_get_interface_report(realm->rd, vdev_dev->pdev,
                                                           vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INTERFACE_REPORT failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    LOG(TEST, "\n\tVDEV state is RMI_VDEV_LOCKED\n");
    if (vdev_state == RMI_VDEV_LOCKED)
        return vdev;

    args = val_host_rmi_vdev_start(realm->rd, vdev_dev->pdev, vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_START failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_STARTED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }
    LOG(TEST, "\n\tVDEV state is RMI_VDEV_STARTED\n");

    return vdev;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}

void val_host_set_pdev_flags(val_host_pdev_flags_ts *pdev_flags)
{
    pdev_flags->spdm = RMI_SPDM_TRUE;
    pdev_flags->category = RMI_PDEV_ENDPOINT_ACCEL_OFF_CHIP;
}
