 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "exception_common_host.h"

/* get the ec and the imm values passed from the realm to the host */
void exception_get_ec_imm(uint64_t esr, uint64_t *ec, uint64_t *imm)
{
    *ec = esr & ESR_EL2_EC_MASK;
    *imm = esr & ESR_EL2_xVC_IMM_MASK;
}

/* copy the common parameters from exit to entry structure.*/
void exception_copy_exit_to_entry(val_host_rec_entry_ts *rec_entry,
                                           val_host_rec_exit_ts *rec_exit)
{
    rec_entry->gicv3_hcr = rec_exit->gicv3_hcr;
    val_memcpy(rec_entry->gprs, rec_exit->gprs, VAL_REC_HVC_NUM_GPRS);
    val_memcpy(rec_entry->gicv3_lrs, rec_exit->gicv3_lrs, VAL_REC_GIC_NUM_LRS);
}

uint64_t exception_validate_rec_exit_ripas(exception_rec_exit_ts rec_exit_exception)
{
    val_host_realm_ts *realm = rec_exit_exception.realm;
    val_host_rec_exit_ts *rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);
    uint64_t end = 0;
    uint64_t status;

    switch (rec_exit_exception.exception_ripas_exit_intent)
    {
        case EXCEPTION_RIPAS_EXIT_ACCEPT:
        case EXCEPTION_RIPAS_EXIT_PARTIAL:
        {
            if (!(rec_exit->base == rec_exit_exception.ripas_base) ||\
                !(rec_exit->size == rec_exit_exception.ripas_size))
            {
                LOG(ERROR, "\tRipas return params mismatch\n", 0, 0);
                return VAL_ERROR;
            }
            end = rec_exit->base;

            /* check the rec exit dueto RIPAS changes*/
            if (rec_exit_exception.ripas_value != rec_exit->ripas_value)
            {
                LOG(ERROR, "\tRipas value mismatch\n", 0, 0);
                return VAL_ERROR;
            }

            while (end < (rec_exit->base + rec_exit->size))
            {
                status = val_host_rmi_rtt_set_ripas(realm->rd, realm->rec[0], end,
                                                VAL_RTT_MAX_LEVEL, rec_exit->ripas_value);
                if (status)
                {
                    LOG(ERROR, "\tRMI_RTT_SET_RIPAS failed, status=%x\n", status, 0);
                    return VAL_ERROR;
                }
                end += PAGE_SIZE;
                if (rec_exit_exception.exception_ripas_exit_intent ==\
                    EXCEPTION_RIPAS_EXIT_PARTIAL)
                    break;
            }
        }
        break;
        case EXCEPTION_RIPAS_EXIT_REJECT:
        break;
    }

    return VAL_SUCCESS;
}

/* validates the Rec Exit dueto IA*/
uint64_t exception_validate_rec_exit_ia(exception_rec_exit_ts *exception_rec_exit)
{
    val_host_realm_ts *realm = exception_rec_exit->realm;
    val_host_rec_exit_ts *rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);
    uint64_t ec, imm;

    LOG(TEST, "\tIA Abort Validation: ESR=%x\n", rec_exit->esr, 0);

    if (rec_exit->exit_reason != RMI_EXIT_SYNC)
    {
        LOG(ERROR, "\tIA Abort Validation: Exit Reason Not RMI_EXIT_SYNC\n", 0, 0);
        return VAL_ERROR;
    }

    exception_get_ec_imm(rec_exit->esr, &ec, &imm);
    if (ec != ESR_EL2_EC_INST_ABORT)
    {
        LOG(ERROR, "\tIA Abort Validation: EC is not instruction abort\n", 0, 0);
        return VAL_ERROR;
    }

    uint64_t received_iss = (rec_exit->esr & ESR_EL2_ISS_EXCEPTION_MASK) >>\
                             ESR_EL2_ISS_EXCEPTION_SHIFT;
    uint64_t iss_set = ESR_EL2_ISS_SET(received_iss);
    if (iss_set != ESR_EL2_ISS_SET_UER)
    {
        LOG(ERROR, "\tIA Abort Validation: Not a Recoverable instruction abort\n", 0, 0);
        return VAL_ERROR;
    }

    uint64_t iss_ea = ESR_EL2_ISS_EA(received_iss);
    if (iss_ea != 0)
    {
        LOG(ERROR, "\tIA Abort Validation: EA is not 0\n", 0, 0);
        return VAL_ERROR;
    }

    uint64_t iss_ifsc = ESR_EL2_ISS_IFSC(received_iss);
    if (iss_ifsc != ESR_EL2_ISS_IA_TFSC_L3_TRANS_FAULT)
    {
        LOG(ERROR, "\tIA Abort Validation: Not a translation fault in level 3\n", 0, 0);
        return VAL_ERROR;
    }

    /* Validate exit ESR MBZ params */
    if (VAL_EXTRACT_BITS(rec_exit->esr, 7, 7) ||    //S1PTW
        VAL_EXTRACT_BITS(rec_exit->esr, 10, 10))    //FnV
    {
        LOG(ERROR, "\tREC exit ESR MBZ params mismatch\n", 0, 0);
        return VAL_ERROR;
    }

    uint64_t iss_hpfar = rec_exit->hpfar;
    uint64_t fipa = ((iss_hpfar & ESR_EL2_HPFAR_MASK) << (ESR_EL2_HPFAR_OFFSET));
    LOG(TEST, "\tIA Abort Validation: HPFAR, FIPA : %x, %x\n", iss_hpfar, fipa);
    if (iss_hpfar == (exception_rec_exit->test_fault_addr >> 8))
    {
        LOG(ERROR, "\tIA Abort Validation: HPFAR FIPA lower 12 bits are not zero, hpfar: %x\n",\
            iss_hpfar, 0);
        return VAL_ERROR;
    }
    LOG(TEST, "\tIA Abort Validation: FIPA Lower 12bits masked? : Yes\n", 0, 0);

    uint64_t aligned = ADDR_IS_ALIGNED(fipa, PAGE_SIZE);
    LOG(TEST, "\tIA Abort Validation: HPFAR fipa Aligned? : %x\n", aligned, 0);
    if (!aligned)
    {
        LOG(ERROR, "\tIA Abort Validation: HPFAR fipa is not Granule Aligned.\n", 0, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/* validates the Rec Exit ESR MBZ fileds due to DA*/
uint64_t check_esr_mbz_feilds(val_host_rec_exit_ts *rec_exit,
                              exception_da_abort_type_te abort_type)
{
    switch (abort_type)
    {
        case EMULATABLE_DA:
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

    return VAL_SUCCESS;
}
