/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_pcie.h"
#include "val_host_doe.h"

uint64_t val_host_pcie_doe_communicate(uint32_t bdf, uint32_t doe_cap_base,
             void *req_buf, size_t req_sz, void *rsp_buf, size_t *rsp_sz, uint8_t req_type)
{
    uint64_t rc;

    rc = val_host_pcie_doe_send_req(bdf, doe_cap_base,
                   (uint32_t *)req_buf, (uint32_t)req_sz, req_type);
    if (rc != 0) {
        LOG(ERROR, "PCIe DOE %s failed %d\n", "Request", rc);
        return rc;
    }

    rc = val_host_pcie_doe_recv_resp(bdf, doe_cap_base, (uint32_t *)rsp_buf,
                rsp_sz);
    return rc;
}

/**
  @brief   This API sends DOE request to PCI device.
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   doe_cap_base - DOE capability base offset
  @param   *req_addr  - DOE request payload buffer
  @param   req_len  - DOE request payload length

  @return  success/failure
**/
uint32_t val_host_pcie_doe_send_req(uint32_t bdf, uint32_t doe_cap_base, uint32_t *req_addr,
                                                         uint64_t req_len, uint8_t req_type)
{
    uint32_t value;
    uint64_t i, doe_length;

    val_pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG, &value);
    if (VAL_EXTRACT_BITS(value, DOE_STATUS_REG_BUSY, DOE_STATUS_REG_BUSY))
    {
        LOG(ERROR, "DOE Busy bit is set\n");
        return VAL_ERROR;
    }

    val_pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG, &value);
    if (VAL_EXTRACT_BITS(value, DOE_STATUS_REG_ERROR, DOE_STATUS_REG_ERROR))
    {
        LOG(ERROR, "DOE Error bit is set\n");
        return VAL_ERROR;
    }

    doe_length = (req_len % 4) ? ((req_len >> 2) + 1) : (req_len >> 2);

    if (req_type == 1) {
        val_pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG, VAL_HOST_DOE_HEADER_2);
    } else {
        val_pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG, VAL_HOST_DOE_HEADER_1);
    }
    val_pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG,
                                    (uint32_t)doe_length + VAL_HOST_DOE_HEADER_LENGTH);

    for (i = 0; i < doe_length; i++)
    {
        val_pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG, req_addr[i]);
    }

    /*Set Go bit*/
    val_pcie_write_cfg(bdf, doe_cap_base + DOE_CTRL_REG, (uint32_t)(1 << 31));
    return VAL_SUCCESS;
}

/**
  @brief   This API receives DOE response from PCI device.
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   doe_cap_base - DOE capability base offset
  @param   *resp_addr  - DOE response payload buffer
  @param   resp_len  - DOE response payload length

  @return  success/failure
**/
uint32_t val_host_pcie_doe_recv_resp(uint32_t bdf, uint32_t doe_cap_base, uint32_t *resp_addr,
                                                                  uint64_t *resp_len)
{
    uint32_t value, length;
    uint64_t i;

    val_pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG, &value);
    if (!(VAL_EXTRACT_BITS(value, DOE_STATUS_REG_READY, DOE_STATUS_REG_READY)))
    {
        LOG(ERROR, "DOE Ready bit is not set: %x\n", value);
        return VAL_ERROR;
    }

    val_pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG, &value);

    if (VAL_EXTRACT_BITS(value, DOE_STATUS_REG_ERROR, DOE_STATUS_REG_ERROR))
    {
        LOG(ERROR, "DOE Error bit is set\n");
        return VAL_ERROR;
    }

    /* Reading DOE Header 1 */
    val_pcie_read_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, &value);
    val_pcie_write_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, 0);

    /* Reading DOE Header 2 - Length */
    val_pcie_read_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, &value);
    val_pcie_write_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, 0);

    length = value - VAL_HOST_DOE_HEADER_LENGTH;
    *resp_len = (uint64_t)length * 4;

    for (i = 0; i < length; i++)
    {
        val_pcie_read_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, &value);
        val_pcie_write_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, 0);
        *resp_addr++ = value;
    }

    val_pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG, &value);
    if (VAL_EXTRACT_BITS(value, DOE_STATUS_REG_READY, DOE_STATUS_REG_READY))
    {
        LOG(ERROR, "DOE Busy bit is not clear\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
