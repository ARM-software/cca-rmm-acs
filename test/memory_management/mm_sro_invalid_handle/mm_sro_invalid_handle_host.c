/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

#define RMI_OP_INVALID_HANDLE 0xFFFFFFFFFFFFFFFFULL

static uint64_t addr_range_desc(uint64_t addr)
{
    return RMI_PAGE_L3 | (1UL << 2) | ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           ((uint64_t)RMI_OP_MEM_UNDELEGATED << 63);
}

static uint64_t check_invalid_handle_result(const char *cmd_name, val_smc_param_ts cmd_ret)
{
    if (RMI_STATUS(cmd_ret.x0) != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "%s accepted invalid SRO handle, ret=0x%lx\n", cmd_name, cmd_ret.x0);
        return VAL_ERROR;
    }

    LOG(TEST, "%s rejected invalid SRO handle, ret=0x%lx\n", cmd_name, cmd_ret.x0);
    return VAL_SUCCESS;
}

void mm_sro_invalid_handle_host(void)
{
    val_smc_param_ts cmd_ret;
    uint64_t list_addr = 0;
    uint64_t mem_addr = 0;
    uint64_t *list;

    list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    mem_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((list_addr == 0) || (mem_addr == 0))
    {
        LOG(ERROR, "Failed to allocate SRO address-list test memory\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto cleanup;
    }

    list = (uint64_t *)list_addr;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = addr_range_desc(mem_addr);

    cmd_ret = val_host_rmi_op_continue_smc(RMI_OP_INVALID_HANDLE, RMI_CONTINUE_KEEP_GOING);
    if (check_invalid_handle_result("RMI_OP_CONTINUE", cmd_ret))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    cmd_ret = val_host_rmi_op_cancel_smc(RMI_OP_INVALID_HANDLE);
    if (check_invalid_handle_result("RMI_OP_CANCEL", cmd_ret))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    cmd_ret = val_host_rmi_op_mem_donate_smc(RMI_OP_INVALID_HANDLE, list_addr, 1);
    if (check_invalid_handle_result("RMI_OP_MEM_DONATE", cmd_ret))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    cmd_ret = val_host_rmi_op_mem_reclaim_smc(RMI_OP_INVALID_HANDLE, list_addr, 1);
    if (check_invalid_handle_result("RMI_OP_MEM_RECLAIM", cmd_ret))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if (mem_addr != 0)
        val_host_mem_free((void *)mem_addr);
    if (list_addr != 0)
        val_host_mem_free((void *)list_addr);
}
