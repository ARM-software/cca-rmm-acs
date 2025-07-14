/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_planes.h"
#include "val_realm_framework.h"
#include "val_hvc.h"
#include "val_libc.h"

/**
 *   @brief    Requests REALM_CONFIG via PSI interface.
 *   @param    buff pointer to the buffer to which the configuration is written.
 *   @return   SUCCESS/FAILURE.
**/
uint64_t val_realm_psi_realm_config(uint64_t buff)
{
    return (val_hvc_call(PSI_REALM_CONFIG, buff, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x1;
}

/**
 *   @brief    Return execution to P0.
 *   @param    none.
 *   @return   none.
**/
void val_realm_return_to_p0(void)
{
    val_hvc_call(PSI_P0_CALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    P0's exception handler.
 *   @param    run Pointer to plane run object.
 *   @return   return path.
**/
uint64_t val_realm_plane_handle_exception(val_realm_rsi_plane_run_ts *run)
{
    uint64_t ec = ESR_EL2_EC(run->exit.esr_el2);

    if (ec == ESR_EL2_EC_SMC) {
        uint64_t smc_id = run->exit.gprs[0];

        if (smc_id == PSI_TEST_SMC || smc_id == RSI_HOST_CALL)
            return PSI_RETURN_TO_P0;

        run->enter.gprs[0] = VAL_SMC_NOT_SUPPORTED;
        return PSI_RETURN_TO_PN;
    }

    if (ec == ESR_EL2_EC_HVC) {
        uint64_t hvc_id = run->exit.gprs[0];

        switch (hvc_id) {
        case PSI_REALM_CONFIG:
            return hvc_handle_realm_config(run);
        case PSI_PRINT_MSG:
            return hvc_handle_realm_print();
        case PSI_P0_CALL:
            return PSI_RETURN_TO_P0;
        default:
            return PSI_RETURN_TO_P0;
        }
    }

    return PSI_RETURN_TO_P0;
}

/**
 *   @brief    Handles REALM_CONFIG requests from Pn.
 *   @param    run Pointer to plane run object.
 *   @return   return path.
**/
uint64_t hvc_handle_realm_config(val_realm_rsi_plane_run_ts *run)
{
    LOG(DBG, "handling PSI realm_config\n");
    run->enter.gprs[0] = RSI_SUCCESS;
    run->enter.gprs[1] = val_realm_get_ipa_width();

    return PSI_RETURN_TO_PN;
}

/**
 *   @brief    Handles print requests from Pn.
 *   @param    none.
 *   @return   return path.
**/
uint64_t hvc_handle_realm_print(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_print_rsi_host_call_t realm_print;

    realm_print.imm = VAL_REALM_PRINT_MSG;
    val_smc_call(RSI_HOST_CALL, (uint64_t)&realm_print, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return PSI_RETURN_TO_PN;
}

/**
 *   @brief    Saves plane context from Plane exit structure to Plane enter structure.
 *   @param    run Pointer to plane run object.
 *   @return   none
**/
void val_realm_preserve_plane_context(val_realm_rsi_plane_run_ts *run)
{
    for (int j = 0; j < VAL_PLANE_ENTRY_GPRS; ++j) {
        run->enter.gprs[j] = run->exit.gprs[j];
    }

    run->enter.pc = run->exit.elr_el2;
}

/**
 *   @brief    Run Plane.
 *   @param    plane_idx Plane index.
 *   @param    run_ptr   Pointer to PlaneRun object.
 *   @return   none
**/
uint64_t val_realm_run_plane(uint64_t plane_index, val_realm_rsi_plane_run_ts *run_ptr)
{
    uint64_t ret;
    val_smc_param_ts cmd_ret;
    while (true)
    {
        cmd_ret = val_realm_rsi_plane_enter(plane_index, (uint64_t)(run_ptr));

        if (cmd_ret.x0) {
            LOG(ERROR, "Plane entry failed with : %d \n", cmd_ret.x0);
            return VAL_ERROR;
        }

        val_realm_preserve_plane_context(run_ptr);

        ret = val_realm_plane_handle_exception(run_ptr);
        if (ret != PSI_RETURN_TO_PN)
            return VAL_SUCCESS;
    }
}
/**
 *   @brief    Initialize access permissions for a plane.
 *   @param    plane_idx Plane index.
 *   @param    perm_idx  Permission index.
 *   @param    base      Base of the plane IPA range.
 *   @param    top       Top of the plane IPA range.
 *   @return   none
**/
uint64_t val_realm_plane_perm_init(uint64_t plane_idx, uint64_t perm_idx,
                                                       uint64_t base, uint64_t top)
{
    uint64_t cookie_value = 0;
    val_smc_param_ts cmd_ret;

    cmd_ret = val_realm_rsi_mem_set_perm_value(plane_idx, perm_idx, S2_AP_RW_upX);
    if (cmd_ret.x0) {
        LOG(ERROR, "MEM_SET_PERM_VALUE failed with : %d \n", cmd_ret.x0);
        return VAL_ERROR;
    }

    while (base != top) {
        cmd_ret = val_realm_rsi_mem_set_perm_index(base, top, perm_idx, cookie_value);
        if (cmd_ret.x0 == RSI_ERROR_INPUT || cmd_ret.x2 == RSI_REJECT)
        {
            LOG(ERROR, "MEM_SET_PERM_INDEX failed with : 0x%lx , Response %d \n",
                                                             cmd_ret.x0, cmd_ret.x2);
            return VAL_ERROR;
        }

        base = cmd_ret.x1;
        cookie_value = cmd_ret.x3;
    }

    return VAL_SUCCESS;
}

