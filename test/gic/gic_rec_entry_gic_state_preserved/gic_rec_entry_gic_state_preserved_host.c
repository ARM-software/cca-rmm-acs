/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_irq.h"

#define GIC_ENTRY_HCR_TEST_VALUE     (1ULL << GICV3_HCR_EL2_UIE)
#define GIC_ENTRY_VMCR_VPMR_SHIFT    24U
#define GIC_ENTRY_VMCR_VPMR_VALUE    0xa0U
#define GIC_ENTRY_VMCR_TEST_VALUE    ((1ULL << 1) | \
                                      ((uint64_t)GIC_ENTRY_VMCR_VPMR_VALUE << \
                                       GIC_ENTRY_VMCR_VPMR_SHIFT))
#define GIC_ENTRY_APR_MAX_REGS      4U
#define GIC_ENTRY_APR_TEST_VALUE(i)  (1ULL << (i))
#define GIC_ENTRY_LR_INTID_BASE      SPI_vINTID

static uint32_t gic_check_reg(const char *name, uint64_t received,
                              uint64_t expected, uint32_t error_point)
{
    if (received != expected) {
        LOG(ERROR, "%s was not preserved, received=%lx expected=%lx\n",
            name, received, expected);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(error_point)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void gic_rec_entry_gic_state_preserved_host(void)
{
    val_host_realm_ts realm;
    uint64_t expected_lrs[VAL_REC_GIC_NUM_LRS];
    uint64_t expected_hcr;
    uint64_t expected_vmcr;
    uint64_t expected_ap0r[GIC_ENTRY_APR_MAX_REGS];
    uint64_t expected_ap1r[GIC_ENTRY_APR_MAX_REGS];
    uint64_t ret;
    uint32_t num_lrs;
    uint32_t num_aprs;
    uint32_t i;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(expected_lrs, 0, sizeof(expected_lrs));
    val_memset(expected_ap0r, 0, sizeof(expected_ap0r));
    val_memset(expected_ap1r, 0, sizeof(expected_ap1r));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    num_lrs = gic_host_ich_num_lrs();
    num_aprs = gic_host_ich_num_aprs();
    if (num_lrs > (sizeof(expected_lrs) / sizeof(expected_lrs[0]))) {
        LOG(ERROR, "Implemented LR count %u exceeds expected_lrs capacity %lu\n",
            num_lrs, sizeof(expected_lrs) / sizeof(expected_lrs[0]));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        return;
    }
    if (num_aprs > (sizeof(expected_ap0r) / sizeof(expected_ap0r[0]))) {
        LOG(ERROR, "Implemented APR count %u exceeds expected_ap0r capacity %lu\n",
            num_aprs, sizeof(expected_ap0r) / sizeof(expected_ap0r[0]));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        return;
    }

    gic_host_ich_clear();

    gic_host_ich_write_hcr(GIC_ENTRY_HCR_TEST_VALUE);
    gic_host_ich_write_vmcr(GIC_ENTRY_VMCR_TEST_VALUE);
    for (i = 0U; i < num_aprs; i++) {
        gic_host_ich_write_ap0r(i, GIC_ENTRY_APR_TEST_VALUE(i));
        gic_host_ich_write_ap1r(i, GIC_ENTRY_APR_TEST_VALUE(i + 1U));
        expected_ap0r[i] = gic_host_ich_read_ap0r(i);
        expected_ap1r[i] = gic_host_ich_read_ap1r(i);
    }

    expected_hcr = gic_host_ich_read_hcr();
    expected_vmcr = gic_host_ich_read_vmcr();

    for (i = 0U; i < num_lrs; i++) {
        gic_host_ich_write_lr(i, gic_lr(GIC_ENTRY_LR_INTID_BASE + i,
                                        GICV3_LR_STATE_INACTIVE));
        expected_lrs[i] = gic_host_ich_read_lr(i);
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);

    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        gic_host_ich_clear();
        return;
    }

    if (gic_check_reg("ICH_HCR_EL2", gic_host_ich_read_hcr(),
                      expected_hcr, 3U))
        goto cleanup;

    if (gic_check_reg("ICH_VMCR_EL2", gic_host_ich_read_vmcr(),
                      expected_vmcr, 4U))
        goto cleanup;

    for (i = 0U; i < num_aprs; i++) {
        if (gic_host_ich_read_ap0r(i) != expected_ap0r[i]) {
            LOG(ERROR, "ICH_AP0R%u_EL2 was not preserved, received=%lx expected=%lx\n",
                i, gic_host_ich_read_ap0r(i), expected_ap0r[i]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto cleanup;
        }

        if (gic_host_ich_read_ap1r(i) != expected_ap1r[i]) {
            LOG(ERROR, "ICH_AP1R%u_EL2 was not preserved, received=%lx expected=%lx\n",
                i, gic_host_ich_read_ap1r(i), expected_ap1r[i]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto cleanup;
        }
    }

    for (i = 0U; i < num_lrs; i++) {
        if (gic_host_ich_read_lr(i) != expected_lrs[i]) {
            LOG(ERROR, "ICH_LR%u_EL2 was not preserved, received=%lx expected=%lx\n",
                i, gic_host_ich_read_lr(i), expected_lrs[i]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto cleanup;
        }
    }

    LOG(TEST, "Validated ICH_HCR_EL2, ICH_VMCR_EL2, "
              "%u ICH_AP0R<n>_EL2 registers\n", num_aprs);
    LOG(TEST, "Validated %u ICH_AP1R<n>_EL2 and "
              "%u ICH_LR<n>_EL2 registers\n", num_aprs, num_lrs);
    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    gic_host_ich_clear();
}
