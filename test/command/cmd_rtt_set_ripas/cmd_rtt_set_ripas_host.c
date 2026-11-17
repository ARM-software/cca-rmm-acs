/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"
#include "rmi_rtt_set_ripas_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define MAP_LEVEL 3

#define NUM_REALMS 2
#define VALID_REALM 0
#define INVALID_REALM 1

static val_host_realm_ts realm[NUM_REALMS];
static val_host_rec_exit_ts *rec_exit;

static struct da_ripas_context {
    val_host_realm_ts realm;
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_pdev_stream_ts *pdev_stream;
    uint64_t ram_base;
    uint64_t ram_top;
    uint64_t empty_base;
    uint64_t empty_top;
    uint64_t dev_pa;
    bool prepared;
} da_ripas;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t rec_ptr_valid;
    uint64_t base_valid;
    uint64_t top_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t rec_ptr;
    uint64_t base;
    uint64_t top;
};

static uint64_t addr_range_desc_4k(uint64_t addr, uint64_t count)
{
    return val_host_rmi_addr_range_desc(addr, count, RMI_PAGE_L3,
                                    RMI_OP_MEM_DELEGATED);
}

static uint64_t rtt_dev_map_flags(void)
{
    return (uint64_t)RMI_ADDR_TYPE_SINGLE;
}

static uint64_t prepare_da_pdev(void)
{
    val_host_pdev_flags_ts pdev_flags = {0};
    uint32_t num_bdf;
    uint32_t rp_bdf;

    num_bdf = val_pcie_get_num_bdf();
    if ((num_bdf == VAL_ERROR) ||
        val_pcie_find_doe_capability(&num_bdf, &da_ripas.pdev.bdf,
                                     &da_ripas.pdev.doe_cap_base) ||
        val_pcie_get_rootport(da_ripas.pdev.bdf, &rp_bdf))
        return VAL_TEST_PREP_SEQ_FAILED;

    da_ripas.pdev.root_id = (uint16_t)rp_bdf;
    val_host_set_pdev_flags(&pdev_flags);
    da_ripas.pdev.pdev_flags = pdev_flags;

    if (g_pdev_ready_prep_sequence(&da_ripas.pdev) ==
        VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if ((da_ripas.pdev.ncoh_num_addr_range == 0U) ||
        ((da_ripas.pdev.ncoh_addr_range[0].top -
          da_ripas.pdev.ncoh_addr_range[0].base) < (2U * PAGE_SIZE)))
    {
        LOG(ERROR, "PDEV needs two pages of non-coherent device memory\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    da_ripas.dev_pa = da_ripas.pdev.ncoh_addr_range[0].base;
    return VAL_SUCCESS;
}

static uint64_t prepare_da_realm(void)
{
    val_host_realm_flags_ts flags0 = {0};
    val_smc_param_ts args;
    uint64_t ret;

    val_host_realm_params(&da_ripas.realm);
    da_ripas.realm.vmid = 3;
    da_ripas.realm.rec_count = 2;
    da_ripas.realm.max_num_vdevs = 1;
    flags0.da = RMI_FEATURE_TRUE;
    val_memcpy(&da_ripas.realm.flags, &flags0, sizeof(flags0));

    da_ripas.pdev_stream = val_host_mem_alloc(
        PAGE_SIZE, sizeof(*da_ripas.pdev_stream));
    if ((da_ripas.pdev_stream == NULL) ||
        val_host_pdev_stream_setup(&da_ripas.pdev, da_ripas.pdev.root_id,
                                   PLATFORM_OVERRIDE_SMMUV3_BASE,
                                   da_ripas.pdev_stream))
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_realm_setup(&da_ripas.realm, true))
        return VAL_TEST_PREP_SEQ_FAILED;

    if (da_create_vdev(&da_ripas.realm, &da_ripas.pdev, &da_ripas.vdev,
                       0, RMI_VDEV_LOCKED) != VAL_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    da_ripas.vdev.realm_rd = da_ripas.realm.rd;
    da_ripas.vdev.pdev_stream = da_ripas.pdev_stream;
    args = val_host_rmi_vdev_start(da_ripas.realm.rd, da_ripas.pdev.pdev,
                                   da_ripas.vdev.vdev);
    if ((args.x0 != RMI_SUCCESS) ||
        val_host_dev_communicate(&da_ripas.realm, &da_ripas.pdev,
                                 &da_ripas.vdev, RMI_VDEV_STARTED))
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_rec_enter(da_ripas.realm.rec[0],
                                 da_ripas.realm.run[0]);
    if (ret || val_host_check_realm_exit_psci(
                    (val_host_rec_run_ts *)da_ripas.realm.run[0],
                    PSCI_CPU_ON_AARCH64))
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_psci_complete(da_ripas.realm.rec[0], PSCI_E_SUCCESS);
    if (ret)
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_rec_enter(da_ripas.realm.rec[0],
                                 da_ripas.realm.run[0]);
    if (ret || val_host_check_realm_exit_ripas_change(
                    (val_host_rec_run_ts *)da_ripas.realm.run[0]))
        return VAL_TEST_PREP_SEQ_FAILED;

    rec_exit = &((val_host_rec_run_ts *)da_ripas.realm.run[0])->exit;
    da_ripas.empty_base = rec_exit->ripas_base;
    da_ripas.empty_top = rec_exit->ripas_top;

    ret = val_host_rmi_rec_enter(da_ripas.realm.rec[1],
                                 da_ripas.realm.run[1]);
    if (ret || val_host_check_realm_exit_ripas_change(
                    (val_host_rec_run_ts *)da_ripas.realm.run[1]))
        return VAL_TEST_PREP_SEQ_FAILED;

    rec_exit = &((val_host_rec_run_ts *)da_ripas.realm.run[1])->exit;
    da_ripas.ram_base = rec_exit->ripas_base;
    da_ripas.ram_top = rec_exit->ripas_top;

    return VAL_SUCCESS;
}

static uint64_t map_da_test_page(uint64_t ipa, uint64_t pa)
{
    uint64_t map_idx;
    uint64_t ret;

    if (create_mapping(ipa, false, da_ripas.realm.rd))
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_rtt_dev_map(da_ripas.realm.rd, da_ripas.vdev.vdev,
                                   ipa, ipa + PAGE_SIZE,
                                   rtt_dev_map_flags(),
                                   addr_range_desc_4k(pa, 1U));
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RTT_DEV_MAP failed, ret=0x%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    map_idx = da_ripas.realm.dev_granules_mapped_count++;
    da_ripas.realm.dev_granules[map_idx].ipa = ipa;
    da_ripas.realm.dev_granules[map_idx].size = PAGE_SIZE;
    da_ripas.realm.dev_granules[map_idx].level = MAP_LEVEL;
    da_ripas.realm.dev_granules[map_idx].pa = pa;
    return VAL_SUCCESS;
}

static uint64_t da_ripas_prep_sequence(void)
{
    uint64_t ret;

    if (da_ripas.prepared)
        return VAL_SUCCESS;

    val_memset(&da_ripas, 0, sizeof(da_ripas));
    if ((prepare_da_pdev() != VAL_SUCCESS) ||
        (prepare_da_realm() != VAL_SUCCESS))
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_granule_range_delegate(da_ripas.dev_pa,
                                               da_ripas.dev_pa +
                                               (2U * PAGE_SIZE));
    if (ret != RMI_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    if ((map_da_test_page(da_ripas.ram_base, da_ripas.dev_pa) != VAL_SUCCESS) ||
        (map_da_test_page(da_ripas.empty_base,
                          da_ripas.dev_pa + PAGE_SIZE) != VAL_SUCCESS))
        return VAL_TEST_PREP_SEQ_FAILED;

    da_ripas.prepared = true;
    return VAL_SUCCESS;
}

static uint64_t da_ripas_teardown(void)
{
    uint64_t status = VAL_SUCCESS;

    if (da_ripas.vdev.vdev != 0U)
    {
        if (val_host_vdev_teardown(&da_ripas.realm, &da_ripas.pdev,
                                   &da_ripas.vdev))
            status = VAL_ERROR;
        da_ripas.pdev_stream = NULL;
    }
    else if (da_ripas.pdev_stream != NULL)
    {
        if (val_host_pdev_stream_teardown(&da_ripas.pdev,
                                          da_ripas.pdev_stream))
            status = VAL_ERROR;
        val_host_mem_free(da_ripas.pdev_stream);
        da_ripas.pdev_stream = NULL;
    }

    if (da_ripas.prepared &&
        val_host_rmi_granule_range_undelegate(da_ripas.dev_pa,
                                               da_ripas.dev_pa +
                                               (2U * PAGE_SIZE)))
        status = VAL_ERROR;

    if ((da_ripas.pdev.pdev != 0U) &&
        val_host_pdev_teardown(&da_ripas.pdev, da_ripas.pdev.pdev))
        status = VAL_ERROR;

    if ((da_ripas.realm.rd != 0U) &&
        val_host_realm_destroy(da_ripas.realm.rd))
        status = VAL_ERROR;

    return status;
}

static uint64_t g_rec_other_owner_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    val_memset(&realm[INVALID_REALM], 0, sizeof(realm[INVALID_REALM]));
    val_memset(&rec_params, 0, sizeof(rec_params));

    realm[INVALID_REALM].s2sz = 40;
    realm[INVALID_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[INVALID_REALM].s2_starting_level = 0;
    realm[INVALID_REALM].num_s2_sl_rtts = 1;
    realm[INVALID_REALM].vmid = 1;
#if defined(RMM_V_1_1)
    realm[INVALID_REALM].vmid = 2;
#endif
    realm[INVALID_REALM].rec_count = 1;

    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;


    if (val_host_realm_create_common(&realm[INVALID_REALM]))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Populate realm with two RECs*/
    if (val_host_rec_create_common(&realm[INVALID_REALM], &rec_params))
    {
        LOG(ERROR, "REC Create Failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[INVALID_REALM].rec[0];
}

static uint64_t rd_valid_prep_sequence(void)
{
    uint64_t ret, ipa_base;
    val_host_realm_flags1_ts realm_flags;

    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_realm_params(&realm[VALID_REALM]);

    realm[VALID_REALM].vmid = VALID_REALM;
    realm[VALID_REALM].rec_count = 2;

#if defined(RMM_V_1_1)
    realm[VALID_REALM].num_aux_planes = 1;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;
    val_memcpy(&realm[VALID_REALM].flags1, &realm_flags, sizeof(realm[VALID_REALM].flags1));
#endif

    /* Populate realm with two RECs*/
    if (val_host_realm_setup(&realm[VALID_REALM], false))
    {
        LOG(ERROR, "Realm setup failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

#if defined(RMM_V_1_1)
    if (ipa_protected_aux_assigned_prep_sequence(realm[VALID_REALM].rd, 1)
                                                     == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;
#endif

    if (val_host_realm_activate(&realm[VALID_REALM]))
    {
        LOG(ERROR, "Realm Activate Failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Power on REC 1 for future execution */
    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[0], realm[VALID_REALM].run[0]);
    if (ret)
    {
        LOG(ERROR, "REC_ENTER failed with ret value: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    } else if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm[VALID_REALM].run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Complete pending PSCI by returning a RUNNABLE target REC*/
    ret = val_host_rmi_psci_complete(realm[VALID_REALM].rec[0], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[0] to continue execution */
    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[0], realm[VALID_REALM].run[0]);
    if (ret)
    {
        LOG(ERROR, "REC_ENTER failed with ret value: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    } else if (val_host_check_realm_exit_ripas_change(
                                    (val_host_rec_run_ts *)realm[VALID_REALM].run[0]))
    {
        LOG(ERROR, "Ripas change req failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm[VALID_REALM].run[0])->exit);
    ipa_base = rec_exit->ripas_base;

    /* Create RTT mappings for requested IPA range in case it doesn't exist */
    while (ipa_base < rec_exit->ripas_top) {
        if (create_mapping(ipa_base, false, realm[VALID_REALM].rd))
            return VAL_TEST_PREP_SEQ_FAILED;
        ipa_base += PAGE_SIZE;
    }

    return realm[VALID_REALM].rd;
}

static uint64_t base_unaligned_prep_sequence(void)
{
    uint64_t ret;

    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[1], realm[VALID_REALM].run[1]);
    if (ret)
    {
        LOG(ERROR, "REC_ENTER failed with ret value: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    } else if (val_host_check_realm_exit_ripas_change(
                                    (val_host_rec_run_ts *)realm[VALID_REALM].run[1]))
    {
        LOG(ERROR, "Ripas change req failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm[VALID_REALM].run[1])->exit);

    return rec_exit->ripas_base;
}

static uint64_t top_rtt_unaligned_prep_sequence(void)
{
    uint64_t ret;

    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[1], realm[VALID_REALM].run[1]);
    if (ret)
    {
        LOG(ERROR, "REC_ENTER failed with ret value: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    } else if (val_host_check_realm_exit_ripas_change(
                                    (val_host_rec_run_ts *)realm[VALID_REALM].run[1]))
    {
        LOG(ERROR, "Ripas change req failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm[VALID_REALM].run[1])->exit);

    return rec_exit->ripas_base;
}

static uint64_t ipa_aux_live_prep_sequence(void)
{
    uint64_t ret;

    if (!val_host_rmm_supports_rtt_tree_per_plane() ||
        !val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "No support for RTT tree per plane\n");
        return VAL_SKIP_CHECK;
    }

    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[1],
                                 realm[VALID_REALM].run[1]);
    if (ret)
    {
        LOG(ERROR, "REC_ENTER failed with ret value: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    else if (val_host_check_realm_exit_ripas_change(
                    (val_host_rec_run_ts *)realm[VALID_REALM].run[1]))
    {
        LOG(ERROR, "Ripas change req failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_exit = &((val_host_rec_run_ts *)realm[VALID_REALM].run[1])->exit;

    return rec_exit->ripas_base;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rec_ptr_valid = realm[VALID_REALM].rec[0];

    c_args.base_valid = rec_exit->ripas_base;

    c_args.top_valid = rec_exit->ripas_top;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;


        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_REC:
            args->rd = realm[VALID_REALM].rec[0];
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[VALID_REALM].rtt_l0_addr;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_DATA:
            args->rd = realm[VALID_REALM].image_pa_base;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_unaligned_prep_sequence(c_args.rec_ptr_valid);
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;


        case REC_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_undelegated_prep_sequence();
            if (args->rec_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_DELEGATED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_delegated_prep_sequence();
            if (args->rec_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_RD:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rd_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_RTT:
            args->rd = c_args.rd_valid;
            args->rec_ptr = realm[VALID_REALM].rtt_l0_addr;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REc_GRAN_STATE_DATA:
            args->rd = c_args.rd_valid;
            args->rec_ptr = realm[VALID_REALM].image_pa_base;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_OTHER_OWNER:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_rec_other_owner_prep_sequence();
            if (args->rec_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case INVALID_SIZE:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.base_valid;
            break;

        case BASE_MISMATCH:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid + 1;
            args->top = c_args.top_valid;
            break;

        case TOP_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid + PAGE_SIZE;
            break;

        case BASE_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = realm[VALID_REALM].rec[1];
            args->base = base_unaligned_prep_sequence();
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = 2 * L2_SIZE;
            break;

        case TOP_GRAN_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid - PAGE_SIZE / 2;
            break;

        case TOP_LEVEL_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = realm[VALID_REALM].rec[1];
            args->base = top_rtt_unaligned_prep_sequence();
            args->top = args->base + L3_SIZE;
            break;

        case IPA_AUX_LIVE:
            args->rd = c_args.rd_valid;
            args->rec_ptr = realm[VALID_REALM].rec[1];
            args->base = ipa_aux_live_prep_sequence();
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            if (args->base == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            args->top = args->base + PAGE_SIZE;
            break;

        case RAM_INVALID:
            if (!val_host_rmm_supports_da())
                return VAL_SKIP_CHECK;

            if (da_ripas_prep_sequence() != VAL_SUCCESS)
                return VAL_ERROR;
            args->rd = da_ripas.realm.rd;
            args->rec_ptr = da_ripas.realm.rec[1];
            args->base = da_ripas.ram_base;
            args->top = da_ripas.ram_base + PAGE_SIZE;
            break;

        case BASE_MISMATCH_BASE_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid + PAGE_SIZE / 2;
            args->top = c_args.top_valid;
            break;

        case TOP_GRAN_UNALIGNED_TOP_LEVEL_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = realm[VALID_REALM].rec[1];
            args->base = top_rtt_unaligned_prep_sequence();
            args->top = args->base + (L3_SIZE / 2);
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_set_ripas_host(void)
{
    uint64_t ret, i, ipa_base, out_top, progress_top;
    struct arguments args;
    val_host_rtt_entry_ts rtte;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK) {
            LOG(TEST, "Skipping Check %d: DA is not supported\n", i + 1);
            continue;
        } else if (ret == VAL_ERROR) {
            LOG(ERROR, " Intent to sequence failed \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rtt_set_ripas_smc(args.rd, args.rec_ptr, args.base,
                                             args.top, &out_top).x0;

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Negative Observability\n", ++i);

    ipa_base = c_args.base_valid;
    while (ipa_base < c_args.top_valid) {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                      ipa_base, MAP_LEVEL, &rtte);
        if (ret) {
            LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if (rtte.walk_level == MAP_LEVEL && rtte.ripas != RMI_EMPTY) {
            LOG(ERROR, " Unexpected RTT entry. walk level: %d, RIPAS : %d\n",
                                                             rtte.walk_level, rtte.ripas);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
        ipa_base += PAGE_SIZE;
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    progress_top = c_args.base_valid + PAGE_SIZE;
    ret = val_host_rmi_rtt_set_ripas(c_args.rd_valid, c_args.rec_ptr_valid,
                                     c_args.base_valid, progress_top, &out_top);
    if (ret != 0)
    {
        LOG(ERROR, " RTT_SET_RIPAS failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (out_top != progress_top)
    {
        LOG(ERROR, "Unexpected out_top, expected=0x%lx returned=0x%lx\n",
            progress_top, out_top);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (out_top < c_args.top_valid)
    {
        /* A successful continuation proves rec.ripas_addr advanced to out_top. */
        ret = val_host_rmi_rtt_set_ripas(c_args.rd_valid, c_args.rec_ptr_valid,
                                         out_top, c_args.top_valid, &out_top);
        if (ret != 0)
        {
            LOG(ERROR, "RTT_SET_RIPAS continuation failed with ret value: %d\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
    }

    if (out_top != c_args.top_valid)
    {
        LOG(ERROR, "Unexpected final out_top, expected=0x%lx returned=0x%lx\n",
            c_args.top_valid, out_top);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    ipa_base = c_args.base_valid;
    while (ipa_base < c_args.top_valid) {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                      ipa_base, MAP_LEVEL, &rtte);
        if (ret) {
            LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
            goto exit;
        }

        if (rtte.walk_level == MAP_LEVEL && rtte.ripas != RMI_RAM) {
            LOG(ERROR, " Unexpected RTT entry. walk level: %d, RIPAS : %d\n",
                                                             rtte.walk_level, rtte.ripas);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
            goto exit;
        }
        ipa_base += PAGE_SIZE;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (da_ripas_teardown() != VAL_SUCCESS)
    {
        LOG(ERROR, "DA test resource teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
    }
    return;
}
