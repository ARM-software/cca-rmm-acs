/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __VAL_HOST_DOE_H__
#define __VAL_HOST_DOE_H__

#define VAL_HOST_DOE_HEADER_1        0x10001
#define VAL_HOST_DOE_HEADER_2        0x20001
#define VAL_HOST_DOE_HEADER_LENGTH   0x2

uint64_t val_host_pcie_doe_communicate(uint32_t bdf, uint32_t doe_cap_base,
             void *req_buf, size_t req_sz, void *rsp_buf, size_t *rsp_sz, uint8_t req_type);
uint32_t val_host_pcie_doe_send_req(uint32_t bdf, uint32_t doe_cap_base, uint32_t *req_addr,
                                                        uint64_t req_len, uint8_t req_type);
uint32_t val_host_pcie_doe_recv_resp(uint32_t bdf, uint32_t doe_cap_base, uint32_t *resp_addr,
                                                                 uint64_t *resp_len);

#endif /* __VAL_HOST_DOE_H__ */
