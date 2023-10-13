/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_host_helpers.h"
#include "val_libc.h"
#include "val_host_realm.h"

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
