/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_da.h"
#include "val_host_doe.h"
#include "val_host_pcie_spec.h"
#include "val_host_pcie.h"

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
                  (void *)dev_comm_enter->resp_addr, &resp_len, dev_comm_exit->protocol);

    /*
     * Set dev_comm_enter args for next pdev_communicate. Upon
     * success or error call pdev_communicate
     */
    if (rc == 0) {
        dev_comm_enter->status = RMI_DEV_COMM_RESPONSE;
        dev_comm_enter->resp_len = resp_len;
    } else {
        dev_comm_enter->status = RMI_DEV_COMM_ERROR;
        dev_comm_enter->resp_len = 0;
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
        if ((pdev_obj->cert_chain_len + dev_obj_buf_len) >
            VAL_HOST_PDEV_CERT_LEN_MAX) {
            return VAL_ERROR;
        }

        LOG(DBG, "\tcache_cert: offset: 0x%lx, len: 0x%lx\n",
              pdev_obj->cert_chain_len, dev_obj_buf_len);

        val_memcpy((void *)(pdev_obj->cert_chain + pdev_obj->cert_chain_len),
                                           dev_obj_buf, dev_obj_buf_len);
        pdev_obj->cert_chain_len += dev_obj_buf_len;
        rc = 0;
    } else if (dev_obj_id == RMI_DEV_VCA) {
        if ((pdev_obj->vca_len + dev_obj_buf_len) >
            VAL_HOST_PDEV_VCA_LEN_MAX) {
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
        if ((vdev_obj->meas_len + dev_obj_buf_len) >
            VAL_HOST_VDEV_MEAS_LEN_MAX) {
            return VAL_ERROR;
        }

        LOG(DBG, "\toffset: 0x%lx, len: 0x%lx\n",
             vdev_obj->meas_len, dev_obj_buf_len);

        val_memcpy((void *)(vdev_obj->meas + vdev_obj->meas_len), dev_obj_buf,
               dev_obj_buf_len);
        vdev_obj->meas_len += dev_obj_buf_len;
        rc = 0;
    } else if (dev_obj_id == RMI_DEV_INTERFACE_REPORT) {
        if ((vdev_obj->ifc_report_len + dev_obj_buf_len) >
            VAL_HOST_VDEV_IFC_REPORT_LEN_MAX) {
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
                     unsigned char cache_obj_id,
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
                           cache_obj_id,
                           dev_obj_cache,
                           cache_len);
    } else {
        rc = val_host_pdev_cache_device_object(pdev_obj,
                           cache_obj_id,
                           dev_obj_cache,
                           cache_len);
    }

    if (rc != 0) {
        LOG(ERROR, "host_dev_cache_device_object failed %x \n", cache_obj_id);
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
                       dev_comm_exit->cache_obj_id,
                       dev_comm_exit->req_cache_offset,
                       dev_comm_exit->req_cache_len);

    if (rc != 0) {
        LOG(ERROR, "host_dev_cache_device_object req failed\n");
        return VAL_ERROR;
    }

    rc = val_host_dev_cache_dev_object(pdev_obj, vdev_obj,
                       (uint8_t *)dev_comm_enter->resp_addr,
                       dev_comm_exit->cache_obj_id,
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

uint64_t val_host_dev_communicate(val_host_realm_ts *realm, val_host_pdev_ts *pdev_obj,
                             val_host_vdev_ts *vdev_obj, unsigned char target_state)
{
    uint64_t rc;
    uint64_t state;
    uint64_t error_state;
    uint64_t ret;
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_exit_ts *dev_comm_exit = NULL;
    bool stop;
    (void)realm;
    (void)ret;

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
    dev_comm_enter->resp_len = 0;

    rc = val_host_dev_get_state(pdev_obj, vdev_obj, &state);
    if (rc != 0) {
        return rc;
    }

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
                LOG(ERROR, "val_host_pdev_doe_communicate failed\n");
                break;
            }
        } else {
            dev_comm_enter->status = RMI_DEV_COMM_NONE;
        }

        rc = val_host_dev_get_state(pdev_obj, vdev_obj, &state);
        if (rc != 0) {
            break;
        }
        if (state == target_state) {
            /* The target state was reached, but for some
             * transitions this is not enough, ned to continue
             * calling it till certain flags are cleared in the
             * exit. wait for that to happen.
             */
            stop = dev_comm_exit->flags == 0U;
        } else if (state == error_state) {
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

uint32_t val_host_vdev_teardown(val_host_realm_ts *realm,
                val_host_pdev_ts *pdev_obj,
                val_host_vdev_ts *vdev_obj)
{
    val_smc_param_ts args;
    size_t i;
    val_host_rtt_entry_ts rtte;
    uint64_t ret;

    if ((vdev_obj == NULL) || (vdev_obj->vdev == 0U)) {
        return VAL_SUCCESS;
    }

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
                args = val_host_rmi_vdev_unmap(realm->rd, ipa, VAL_RTT_MAX_LEVEL);
                if (args.x0) {
                    LOG(ERROR, "VDEV unmap failed during teardown, ipa=0x%lx ret=0x%lx\n",
                        (unsigned long)ipa, (unsigned long)args.x0);
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

    if ((args.x1 == RMI_VDEV_LOCKED) || (args.x1 == RMI_VDEV_STARTED)) {
        args = val_host_rmi_vdev_unlock(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
        if (args.x0) {
            args = val_host_rmi_vdev_get_state(vdev_obj->vdev);
            if (args.x0) {
                LOG(ERROR, "VDEV get state failed, ret=0x%lx\n", (unsigned long)args.x0);
                return VAL_ERROR;
            }
            if (args.x1 == RMI_VDEV_ERROR) {
                args = val_host_rmi_vdev_destroy(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
                if (args.x0) {
                    LOG(ERROR, "VDEV destroy failed, ret=0x%lx\n", (unsigned long)args.x0);
                    return VAL_ERROR;
                }
            }
            return VAL_SUCCESS;
        }

        if (val_host_dev_communicate(realm, pdev_obj, vdev_obj, RMI_VDEV_UNLOCKED)) {
            args = val_host_rmi_vdev_get_state(vdev_obj->vdev);
            if (args.x0) {
                LOG(ERROR, "VDEV get state failed, ret=0x%lx\n", (unsigned long)args.x0);
                return VAL_ERROR;
            }
            if (args.x1 == RMI_VDEV_ERROR) {
                args = val_host_rmi_vdev_destroy(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
                if (args.x0) {
                    LOG(ERROR, "VDEV destroy failed, ret=0x%lx\n", (unsigned long)args.x0);
                    return VAL_ERROR;
                }
            }
            return VAL_SUCCESS;
        }
    }

    realm->dev_granules_mapped_count = 0;

    args = val_host_rmi_vdev_destroy(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
    if (args.x0) {
        LOG(ERROR, "VDEV destroy failed, ret=0x%lx\n", (unsigned long)args.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint32_t val_host_pdev_teardown(val_host_pdev_ts *pdev_obj, uint64_t pdev_ptr)
{
    val_smc_param_ts args;
    uint64_t state;

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

    if ((state == RMI_PDEV_COMMUNICATING) || (state == RMI_PDEV_IDE_RESETTING)) {
        if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_READY)) {
            LOG(ERROR, "PDEV communicate failed while exiting transitional state\n");
            return VAL_ERROR;
        }

        args = val_host_rmi_pdev_get_state(pdev_ptr);
        if (args.x0) {
            LOG(ERROR, "PDEV get state failed after communicating, ret=0x%lx\n",
                (unsigned long)args.x0);
            return VAL_ERROR;
        }

        state = args.x1;
    }

    if (state == RMI_PDEV_STOPPING) {
        if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_STOPPED)) {
            LOG(ERROR, "PDEV communicate failed while waiting for stop\n");
            return VAL_ERROR;
        }

        args = val_host_rmi_pdev_get_state(pdev_ptr);
        if (args.x0) {
            LOG(ERROR, "PDEV get state failed while waiting for stop, ret=0x%lx\n",
                (unsigned long)args.x0);
            return VAL_ERROR;
        }

        state = args.x1;
    }

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
            pdev_obj->dev_comm_data->enter.resp_addr = resp_addr;

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

    args = val_host_rmi_pdev_destroy(pdev_ptr);
    if (args.x0) {
        LOG(ERROR, "PDEV destroy failed, ret=0x%lx\n", (unsigned long)args.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
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
