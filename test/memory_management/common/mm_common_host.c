/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"

uint32_t val_host_init_ripas_delegate(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create)
{
    uint32_t i = 0;
    uint64_t phys = data_create->target_pa;

    /* MAP image regions */
    while (i < (data_create->size/PAGE_SIZE))
    {
        if (val_host_ripas_init(realm,
                data_create->ipa + i * PAGE_SIZE,
                VAL_RTT_MAX_LEVEL, data_create->rtt_alignment))
        {
            LOG(ERROR, "\tval_host_ripas_init failed, ipa=0x%x\n",
                    data_create->ipa + i * PAGE_SIZE, 0);
            return VAL_ERROR;
        }

        if (val_host_rmi_granule_delegate(phys))
        {
            LOG(ERROR, "\tGranule delegation failed, PA=0x%x\n", phys, 0);
            return VAL_ERROR;
        }
        phys += PAGE_SIZE;
        i++;
    }

    return VAL_SUCCESS;
}

/* validates the Rec Exit due to DA*/
uint32_t validate_rec_exit_da(val_host_rec_exit_ts *rec_exit, uint64_t hpfar, uint32_t dfsc)
{
    /* Validate exit_reason */
    if (rec_exit->exit_reason != RMI_EXIT_SYNC)
    {
        LOG(ERROR, "\tExit_reason mismatch: %x\n", rec_exit->exit_reason, 0);
        return VAL_ERROR;
    }

    /* Validate exit ESR params */
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

    /* Validate exit HPFAR field */
    if (VAL_EXTRACT_BITS(hpfar, 8, 63) != rec_exit->hpfar)
    {
        LOG(ERROR, "\tREC exit HPFAR mismatch: %x\n", rec_exit->hpfar, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/* validates the Rec Exit due to IA*/
uint32_t validate_rec_exit_ia(val_host_rec_exit_ts *rec_exit, uint64_t hpfar)
{
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
        (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_EC_LOWER_EL)))
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

    return VAL_SUCCESS;
}

