/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_host_helpers.h"
#include "val_libc.h"
#include "val_host_realm.h"
#include "val_host_command.h"

/* validates the Rec Exit due to DA*/
uint32_t validate_rec_exit_da(val_host_rec_exit_ts *rec_exit, uint64_t hpfar,
                            uint32_t dfsc, exception_da_abort_type_te abort_type, uint32_t wnr)
{
    val_host_rec_exit_ts tmp_exit;

    val_memset(&tmp_exit, 0, sizeof(tmp_exit));

    /* Validate exit_reason */
    if (rec_exit->exit_reason != RMI_EXIT_SYNC)
    {
        LOG(ERROR, "\tExit_reason mismatch: %x\n", rec_exit->exit_reason, 0);
        return VAL_ERROR;
    }


    /* Validate exit HPFAR field */
    if (VAL_EXTRACT_BITS(hpfar, 8, 63) != rec_exit->hpfar)
    {
        LOG(ERROR, "\tREC exit HPFAR mismatch: %x\n", rec_exit->hpfar, 0);
        return VAL_ERROR;
    }

    /* Validate exit ESR params */
    switch (abort_type)
    {
        case EMULATABLE_DA:
               if (!((VAL_EXTRACT_BITS(rec_exit->esr, 11, 12) == ESR_ISS_SET_UER) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 9, 9) == ESR_ISS_EA) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 0, 5) == dfsc) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_EC_LOWER_EL) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 24, 24) == ESR_ISV_VALID) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 22, 23) == ESR_SAS_WORD) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 6, 6) == wnr) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 10, 10) == ESR_FnV)))
                {
                    LOG(ERROR, "\tREC exit params mismatch\n", 0, 0);
                    return VAL_ERROR;
                }

               /* On REC exit due to Emulatable Data Abort, All other exit.esr fields are zero. */
                if (VAL_EXTRACT_BITS(rec_exit->esr, 7, 7) ||    //S1PTW
                    VAL_EXTRACT_BITS(rec_exit->esr, 8, 8) ||    //CM
                    VAL_EXTRACT_BITS(rec_exit->esr, 13, 13) ||  //VNCR
                    VAL_EXTRACT_BITS(rec_exit->esr, 14, 14) ||  //AR
                    VAL_EXTRACT_BITS(rec_exit->esr, 15, 15) ||  //SF
                    VAL_EXTRACT_BITS(rec_exit->esr, 16, 20) ||  //SRT
                    VAL_EXTRACT_BITS(rec_exit->esr, 21, 21) ||  //SSE
                    VAL_EXTRACT_BITS(rec_exit->esr, 25, 25))    //IL
                {
                    LOG(ERROR, "\tREC exit ESR MBZ fail: esr %lx\n", rec_exit->esr, 0);
                    return VAL_ERROR;
                }
                break;

        case NON_EMULATABLE_DA:
                if (!((VAL_EXTRACT_BITS(rec_exit->esr, 11, 12) == ESR_ISS_SET_UER) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 9, 9) == ESR_ISS_EA) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 10, 10) == ESR_FnV) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 0, 5) == dfsc) &&
                    (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_EC_LOWER_EL)))
                {
                    LOG(ERROR, "\tREC exit ESR params mismatch: %lx\n", rec_exit->esr, 0);
                    return VAL_ERROR;
                }

                /* On REC exit due to Non-Emulatable Data Abort,
                 * All other exit.esr fields are zero.
                 */
                if (VAL_EXTRACT_BITS(rec_exit->esr, 7, 7) ||    //S1PTW
                    VAL_EXTRACT_BITS(rec_exit->esr, 8, 8) ||    //CM
                    VAL_EXTRACT_BITS(rec_exit->esr, 13, 13) ||  //VNCR
                    VAL_EXTRACT_BITS(rec_exit->esr, 14, 14) ||  //AR
                    VAL_EXTRACT_BITS(rec_exit->esr, 15, 15) ||  //SF
                    VAL_EXTRACT_BITS(rec_exit->esr, 16, 20) ||  //SRT
                    VAL_EXTRACT_BITS(rec_exit->esr, 21, 21) ||  //SSE
                    VAL_EXTRACT_BITS(rec_exit->esr, 24, 24) ||  //ISV
                    VAL_EXTRACT_BITS(rec_exit->esr, 22, 23) ||  //SAS
                    VAL_EXTRACT_BITS(rec_exit->esr, 6, 6) ||   //WnR
                    VAL_EXTRACT_BITS(rec_exit->esr, 25, 25))    //IL
                {
                    LOG(ERROR, "\tREC exit ESR MBZ fail: esr %lx\n", rec_exit->esr, 0);
                    return VAL_ERROR;
                }
    }

    /* All other exit fields are zero */
    if (val_memcmp(((char *)&tmp_exit + 1), (void *)((char *)rec_exit + 1), 0xFF))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x108), (void *)((char *)rec_exit + 0x108), 0x8))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x118), (void *)((char *)rec_exit + 0x118), 0xE8))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x208), (void *)((char *)rec_exit + 0x208), 0xF8))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x398), (void *)((char *)rec_exit + 0x398), 0x68))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x420), (void *)((char *)rec_exit + 0x420), 0x2E0))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x701), (void *)((char *)rec_exit + 0x701), 0xFF))
    {
        LOG(ERROR, "\tDA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/* validates the Rec Exit due to IA*/
uint32_t validate_rec_exit_ia(val_host_rec_exit_ts *rec_exit, uint64_t hpfar)
{
    val_host_rec_exit_ts tmp_exit;

    val_memset(&tmp_exit, 0, sizeof(tmp_exit));

    /* Validate exit_reason */
    if (rec_exit->exit_reason != RMI_EXIT_SYNC)
    {
        LOG(ERROR, "\tExit_reason mismatch: %x\n", rec_exit->exit_reason, 0);
        return VAL_ERROR;
    }

    /* Validate exit ESR params */
    if (!((VAL_EXTRACT_BITS(rec_exit->esr, 11, 12) == ESR_ISS_SET_UER) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 9, 9) == ESR_ISS_EA) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 0, 5) == ESR_ISS_IFSC_TTF_L3) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_IA_EC_LOWER_EL)))
    {
        LOG(ERROR, "\tREC exit ESR params mismatch: %lx\n", rec_exit->esr, 0);
        return VAL_ERROR;
    }

    /* Validate exit ESR MBZ params */
    if (VAL_EXTRACT_BITS(rec_exit->esr, 7, 7) ||    //S1PTW
        VAL_EXTRACT_BITS(rec_exit->esr, 10, 10))    //FnV
    {
        LOG(ERROR, "\tREC exit ESR MBZ params mismatch: %lx\n", rec_exit->esr, 0);
        return VAL_ERROR;
    }

    /* Validate exit HPFAR field */
    if (VAL_EXTRACT_BITS(hpfar, 8, 63) != rec_exit->hpfar)
    {
        LOG(ERROR, "\tREC exit HPFAR mismatch: %x\n", rec_exit->hpfar, 0);
        return VAL_ERROR;
    }

    /* All other exit fields are zero */
    if (val_memcmp(((char *)&tmp_exit + 1), (void *)((char *)rec_exit + 1), 0xFF))
    {
        LOG(ERROR, "\tIA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x108), (void *)((char *)rec_exit + 0x108), 0x8))
    {
        LOG(ERROR, "\tIA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x118), (void *)((char *)rec_exit + 0x118), 0x1E7))
    {
        LOG(ERROR, "\tIA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x398), (void *)((char *)rec_exit + 0x398), 0x68))
    {
        LOG(ERROR, "\tIA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x420), (void *)((char *)rec_exit + 0x420), 0x2E0))
    {
        LOG(ERROR, "\tIA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    if (val_memcmp(((char *)&tmp_exit + 0x701), (void *)((char *)rec_exit + 0x701), 0xFF))
    {
        LOG(ERROR, "\tIA: REC exit fields MBZ fail\n", 0, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/* Aligned address to given level */
uint64_t val_host_addr_align_to_level(uint64_t addr, uint64_t level)
{
    uint64_t levels = VAL_RTT_MAX_LEVEL - level;
    uint64_t lsb = levels * S2TTE_STRIDE + GRANULE_SHIFT;
    uint64_t msb = IPA_WIDTH_DEFAULT - 1;

    return (addr & VAL_BIT_MASK_ULL(msb, lsb));
}

/**
 *   @brief    Perform S2AP change for requested memory rage.
 *   @param    realm         -  pointer to realm structure
 *   @param    rec_exit      -  pointer to rec_exit structure
 *   @param    rec_enter     -  pointer to rec_enter structure
 *   @return   VAL_SUCCESS / VAL_ERROR
**/

uint64_t val_host_set_s2ap(val_host_realm_ts *realm)
{
    uint64_t s2ap_ipa_base, s2ap_ipa_top, ret;
    val_smc_param_ts cmd_ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    while (rec_exit->exit_reason == RMI_EXIT_S2AP_CHANGE)
    {
        s2ap_ipa_base = rec_exit->s2ap_base;
        s2ap_ipa_top =  rec_exit->s2ap_top;

        while (s2ap_ipa_base != s2ap_ipa_top) {
            cmd_ret = val_host_rmi_rtt_set_s2ap(realm->rd, realm->rec[0],
                                                       s2ap_ipa_base, s2ap_ipa_top);

            /* RTT_SET_S2AP requires all RTTs to be create when running in RTT per plane
             * configuration */
            if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_RTT)
            {
                    if (create_mapping(s2ap_ipa_base, false, realm->rd))
                    {
                        LOG(ERROR, "\tRTT_AUX_CREATE failed\n", 0, 0);
                        return VAL_ERROR;
                    }
                continue;
            }
            else if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_RTT_AUX)
            {
                for (uint64_t i = 0; i < realm->num_aux_planes; i++)
                {
                    if (val_host_create_aux_mapping(realm->rd, s2ap_ipa_base, i + 1))
                    {
                        LOG(ERROR, "\tRTT_AUX_CREATE failed\n", 0, 0);
                        return VAL_ERROR;
                    }
                }

                continue;
            }
            else if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_INPUT) {
                LOG(ERROR, "\nRMI_SET_S2AP failed with ret= 0x%x\n", cmd_ret.x0, 0);
                return VAL_ERROR;
            }

            s2ap_ipa_base = cmd_ret.x1;
        }

        rec_enter->flags = 0x0;

        /* Enter REC[0]  */
        ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
        if (ret)
        {
            LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}
