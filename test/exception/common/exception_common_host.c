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
    uint64_t status;
    uint64_t out_top;

    switch (rec_exit_exception.exception_ripas_exit_intent)
    {
        case EXCEPTION_RIPAS_EXIT_ACCEPT:
        case EXCEPTION_RIPAS_EXIT_PARTIAL:
        {
            if (!(rec_exit->ripas_base == rec_exit_exception.ripas_base) ||\
                !(rec_exit->ripas_top == rec_exit_exception.ripas_top))
            {
                LOG(ERROR, "\tRipas return params mismatch\n", 0, 0);
                return VAL_ERROR;
            }

            /* check the rec exit dueto RIPAS changes*/
            if (rec_exit_exception.ripas_value != rec_exit->ripas_value)
            {
                LOG(ERROR, "\tRipas value mismatch\n", 0, 0);
                return VAL_ERROR;
            }

            if (rec_exit_exception.exception_ripas_exit_intent ==\
                EXCEPTION_RIPAS_EXIT_PARTIAL)
            {
                status = val_host_rmi_rtt_set_ripas(realm->rd, realm->rec[0], rec_exit->ripas_base,
                                        (rec_exit->ripas_base + 0x1000), &out_top);
            } else {
                status = val_host_rmi_rtt_set_ripas(realm->rd, realm->rec[0], rec_exit->ripas_base,
                                        rec_exit->ripas_top, &out_top);
            }

            if (status)
            {
                LOG(ERROR, "\tRMI_RTT_SET_RIPAS failed, status=%x\n", status, 0);
                return VAL_ERROR;
            }

        }
        break;

        case EXCEPTION_RIPAS_EXIT_REJECT:
        break;
    }

    return VAL_SUCCESS;
}
