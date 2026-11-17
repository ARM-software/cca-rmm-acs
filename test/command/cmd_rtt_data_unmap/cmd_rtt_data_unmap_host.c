/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "val_host_sro.h"
#include "rmi_rtt_data_unmap_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)

#define IPA_ADDR_DATA 3 * PAGE_SIZE
#define IPA_ADDR_ASSIGNED PAGE_SIZE
#define IPA_ADDR_LIST (6 * PAGE_SIZE)
#define IPA_ADDR_BLOCK L2_SIZE

#define MAP_LEVEL 3
#define ADDR_RANGE_ADDR_MASK VAL_HOST_RMI_ADDR_DESC_ADDR_MASK

#define NUM_REALMS 1
#define VALID_REALM 0

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st protected L3 Entry)
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * |   HIPAS = ASSIGNED, RIPAS = RAM    |
 *  ------------------------------------
 * |   HIPAS = DESTROYED                |
  *  ------------------------------------
 * |  HIPAS = ASSIGNED, RIPAS = RAM     |
 *  ------------------------------------
 * |  HIPAS = ASSIGNED, RIPAS = EMPTY   |
 *  ------------------------------------
 * ipa: 0x5000
 *
 *                  (...)
 *
 *
 * ipa: 0x1XXX000 (1st unprotected L3 Entry)
 *  ------------------------------------
 * |         HIPAS = ASSIGNED_NS        |
 *  ------------------------------------
 * |        HIPAS = UNASSIGNED          |
 *  ------------------------------------
 * ipa: 0x1XX2000
 *
 */

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t flags_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
};

struct cmd_output {
    uint64_t top;
    uint64_t range;
    uint64_t count;
} c_exp_output;

static uint64_t block_data;
static uint64_t output_list[PAGE_SIZE / sizeof(uint64_t)]
                            __attribute__((aligned(PAGE_SIZE)));
static struct {
    uint64_t initial_state;
    uint64_t folded_rtt;
    uint64_t delegated_start;
    uint64_t delegated_top;
    bool region_changed;
    bool block_mapped;
    bool rtt_split;
} tracking;

static uint64_t rtt_data_map(uint64_t rd, uint64_t data, uint64_t ipa,
                             uint64_t map_size, uint64_t size)
{
    val_host_rmi_rtt_prot_map_flags_ts map_flags = {0};
    uint64_t flags = 0;

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &map_flags, sizeof(map_flags));

    return val_host_rmi_rtt_data_map(rd, ipa, ipa + map_size,
                                     flags, val_host_rmi_addr_range_desc(
                                         data, 1, size, RMI_OP_MEM_DELEGATED));
}

static uint32_t granule_range_transition_raw(uint64_t base, uint64_t top,
                                              bool delegate)
{
    uint64_t completed_top;

    return val_host_tracking_test_range_transition(base, top, delegate,
                                                    &completed_top);
}

static uint64_t rtt_data_unmap(uint64_t rd, uint64_t base, uint64_t top,
                               uint64_t flags, uint64_t oaddr,
                               struct cmd_output *output)
{
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    val_smc_param_ts args;
    uint64_t ret;

    args = val_host_rmi_rtt_data_unmap_smc(rd, base, top, flags, oaddr);
    ret = val_host_sro_complete(&args);
    if ((ret == RMI_SUCCESS) && (output != NULL)) {
        output->top = args.x1;
        output->range = args.x2;
        output->count = args.x3;
    }

    if (ret == RMI_SUCCESS) {
        val_memcpy(&unmap_flags, &flags, sizeof(unmap_flags));
        if (unmap_flags.oaddr_type == RMI_ADDR_TYPE_SINGLE)
            val_host_update_destroy_granule_state(
                rd, ADDR_ALIGN_DOWN(args.x2, PAGE_SIZE), base, 0,
                GRANULE_DELEGATED, GRANULE_DATA, 0);
    }

    return ret;
}

static uint64_t check_single_output(const struct cmd_output *output,
                                    uint64_t data, uint64_t top,
                                    uint64_t size)
{
    if ((output->top != top) ||
        (output->range != val_host_rmi_addr_range_desc(data, 1, size,
                                                   RMI_OP_MEM_DELEGATED)) ||
        (output->count != 0UL)) {
        LOG(ERROR, "Unexpected output: top=0x%x range=0x%x count=0x%x\n",
            output->top, output->range, output->count);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t check_rtte(uint64_t ipa, uint64_t level, uint64_t ripas)
{
    val_host_rtt_entry_ts rtte = {0};
    uint64_t ret;

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, ipa, level, &rtte);
    if (ret || (rtte.state != RMI_UNASSIGNED) || (rtte.ripas != ripas)) {
        LOG(ERROR, "Unexpected RTTE: ret=0x%x state=%d ripas=%d\n",
            ret, rtte.state, rtte.ripas);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t check_granules_delegated(uint64_t data, uint64_t size)
{
    uint64_t ret;

    ret = val_host_rmi_granule_range_undelegate(data, data + size);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Unmapped granules were not DELEGATED, ret=0x%x\n", ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t mapped_l2_prep_sequence(void)
{
    uint64_t ret;

    if (block_data != 0UL)
        return IPA_ADDR_BLOCK;

    block_data = (uint64_t)val_host_mem_alloc(L2_SIZE, L2_SIZE);
    if (block_data == 0UL)
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = granule_range_transition_raw(block_data, block_data + L2_SIZE,
                                        true);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Unable to delegate DATA range, ret=0x%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = rtt_data_map(c_args.rd_valid, block_data, IPA_ADDR_BLOCK,
                       L2_SIZE, RMI_BLOCK_L2);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Unable to create L2 DATA mapping, ret=0x%x\n", ret);
        (void)granule_range_transition_raw(block_data,
                                            block_data + L2_SIZE, false);
        val_host_mem_free((void *)block_data);
        block_data = 0UL;
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_BLOCK;
}

static uint64_t check_and_remove_l2_mapping(void)
{
    struct cmd_output output = {0};
    uint64_t ret;

    ret = rtt_data_unmap(c_args.rd_valid, IPA_ADDR_BLOCK,
                         IPA_ADDR_BLOCK + L2_SIZE,
                         c_args.flags_valid, 0, &output);
    if ((ret != RMI_SUCCESS) ||
        check_single_output(&output, block_data,
                            IPA_ADDR_BLOCK + L2_SIZE, RMI_BLOCK_L2))
        return VAL_ERROR;

    ret = granule_range_transition_raw(block_data, block_data + L2_SIZE,
                                        false);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Unmapped DATA range was not DELEGATED, ret=0x%x\n", ret);
        return VAL_ERROR;
    }

    val_host_update_destroy_granule_state(0, block_data, 0, 0,
                                           GRANULE_UNDELEGATED, 0, 0);
    block_data = 0UL;

    return VAL_SUCCESS;
}

static uint32_t tracking_cleanup(void)
{
    const uint64_t base = PLATFORM_TRACKING_TEST_REGION_BASE;
    const uint64_t top = IPA_ADDR_BLOCK + TRACKING_TEST_REGION_SIZE_2MB;
    uint64_t ret;
    uint64_t completed_top;
    val_smc_param_ts cmd_ret;

    if (tracking.rtt_split)
    {
        ret = val_host_rmi_rtt_fold(c_args.rd_valid, IPA_ADDR_BLOCK,
                                    MAP_LEVEL, &tracking.folded_rtt);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not fold tracking test RTT, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }
        tracking.rtt_split = false;
    }

    if (tracking.folded_rtt != 0U)
    {
        ret = val_host_rmi_granule_range_undelegate(
                  tracking.folded_rtt, tracking.folded_rtt + PAGE_SIZE);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not undelegate folded RTT, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }
        val_host_mem_free((void *)tracking.folded_rtt);
        tracking.folded_rtt = 0U;
    }

    if (tracking.block_mapped)
    {
        cmd_ret = val_host_rmi_rtt_data_unmap_smc(c_args.rd_valid,
                    IPA_ADDR_BLOCK, top, c_args.flags_valid, 0U);
        ret = val_host_sro_complete(&cmd_ret);
        if ((ret != RMI_SUCCESS) || (cmd_ret.x1 != top))
        {
            LOG(ERROR, "Could not remove tracking test block, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }
        tracking.block_mapped = false;
    }

    if (tracking.delegated_top > tracking.delegated_start)
    {
        ret = val_host_tracking_test_range_transition(tracking.delegated_start,
                                                      tracking.delegated_top,
                                                      false, &completed_top);
        if (ret != VAL_SUCCESS)
        {
            tracking.delegated_start = completed_top;
            LOG(ERROR, "Could not undelegate tracking test region at 0x%lx\n",
                completed_top);
            return VAL_ERROR;
        }
        tracking.delegated_start = base;
        tracking.delegated_top = base;
    }

    if (tracking.region_changed)
    {
        ret = val_host_rmi_granule_tracking_set(base,
                RMI_MEM_CATEGORY_CONVENTIONAL, tracking.initial_state);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not restore tracking test region state, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }
        tracking.region_changed = false;
    }

    return VAL_SUCCESS;
}

static uint64_t tracking_prep_sequence(void)
{
    const uint64_t base = PLATFORM_TRACKING_TEST_REGION_BASE;
    const uint64_t top = base + TRACKING_TEST_REGION_SIZE_2MB;
    val_host_rmi_rtt_prot_map_flags_ts map_flags = {0};
    uint64_t flags = 0;
    uint64_t ret;
    val_smc_param_ts cmd_ret;

    if (PLATFORM_RMM_TRACKING_REGION_SIZE == 0U)
    {
        LOG(TEST, "Skipping DATA_UNMAP tracking check in 1 GB mode\n");
        return VAL_SKIP_CHECK;
    }

    tracking.delegated_start = base;
    tracking.delegated_top = base;

    cmd_ret = val_host_rmi_granule_tracking_get(base, top);
    if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_INPUT)
    {
        LOG(TEST, "Skipping DATA_UNMAP tracking check: DRAM2 region is unavailable\n");
        return VAL_SKIP_CHECK;
    }
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "DATA_UNMAP TRACKING_GET failed, ret=0x%lx\n", cmd_ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    if ((cmd_ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (cmd_ret.x3 != top))
    {
        LOG(TEST, "Skipping DATA_UNMAP tracking check: DRAM2 region is unavailable\n");
        return VAL_SKIP_CHECK;
    }

    if ((cmd_ret.x2 != RMI_TRACKING_NONE) &&
        (cmd_ret.x2 != RMI_TRACKING_COARSE) &&
        (cmd_ret.x2 != RMI_TRACKING_FINE))
    {
        LOG(TEST, "Skipping DATA_UNMAP tracking check: unsupported initial state\n");
        return VAL_SKIP_CHECK;
    }

    tracking.initial_state = cmd_ret.x2;
    if (tracking.initial_state != RMI_TRACKING_COARSE)
    {
        ret = val_host_rmi_granule_tracking_set(base,
                RMI_MEM_CATEGORY_CONVENTIONAL, RMI_TRACKING_COARSE);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not set tracking test region to COARSE, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }
        tracking.region_changed = true;
    }

    cmd_ret = val_host_rmi_granule_tracking_get(base, top);
    if ((cmd_ret.x0 != RMI_SUCCESS) ||
        (cmd_ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (cmd_ret.x2 != RMI_TRACKING_COARSE) || (cmd_ret.x3 != top))
    {
        LOG(ERROR, "Tracking test region is not COARSE after setup\n");
        return VAL_ERROR;
    }

    ret = val_host_tracking_test_range_transition(base, top, true,
                                                  &tracking.delegated_top);
    if (ret != VAL_SUCCESS)
    {
        LOG(ERROR, "Could not delegate tracking test region, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &map_flags, sizeof(flags));

    cmd_ret = val_host_rmi_rtt_data_map_smc(c_args.rd_valid, IPA_ADDR_BLOCK,
                IPA_ADDR_BLOCK + TRACKING_TEST_REGION_SIZE_2MB, flags,
                val_host_rmi_addr_range_desc(base, 1U, RMI_BLOCK_L2,
                                             RMI_OP_MEM_DELEGATED));
    ret = val_host_sro_complete(&cmd_ret);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Could not map tracking test block, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }
    tracking.block_mapped = true;

    if (create_mapping(IPA_ADDR_BLOCK, false, c_args.rd_valid) != VAL_SUCCESS)
    {
        LOG(ERROR, "Could not split tracking test block into L3 entries\n");
        return VAL_ERROR;
    }
    tracking.rtt_split = true;

    return VAL_SUCCESS;
}

static uint64_t check_list_unmap(void)
{
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    val_smc_param_ts smc_args;
    struct cmd_output output = {0};
    uint64_t *list = output_list;
    uint64_t flags = 0;
    uint64_t data;
    uint64_t ret;

    if (create_mapping(IPA_ADDR_LIST, true, c_args.rd_valid))
        return VAL_ERROR;

    data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    ret = rtt_data_map(c_args.rd_valid, data, IPA_ADDR_LIST,
                       PAGE_SIZE, RMI_PAGE_L3);
    if (ret != RMI_SUCCESS)
        return VAL_ERROR;

    val_memset(list, 0, PAGE_SIZE);

    unmap_flags.oaddr_type = RMI_ADDR_TYPE_LIST;
    unmap_flags.list_count = 1;
    val_memcpy(&flags, &unmap_flags, sizeof(flags));

    smc_args = val_host_rmi_rtt_data_unmap_smc(
        c_args.rd_valid, IPA_ADDR_LIST, IPA_ADDR_LIST + PAGE_SIZE,
        flags, (uint64_t)list);
    if (RMI_STATUS(smc_args.x0) == RMI_INCOMPLETE)
        ret = val_host_sro_complete(&smc_args);
    else
        ret = smc_args.x0;

    if (ret != RMI_SUCCESS) {
        ret = val_host_rmi_rtt_data_unmap(c_args.rd_valid, IPA_ADDR_LIST,
                                          IPA_ADDR_LIST + PAGE_SIZE,
                                          c_args.flags_valid, 0);
        if (ret == RMI_SUCCESS)
            (void)check_granules_delegated(data, PAGE_SIZE);
        return VAL_ERROR;
    }

    output.top = smc_args.x1;
    output.range = smc_args.x2;
    output.count = smc_args.x3;

    val_host_update_destroy_granule_state(c_args.rd_valid, data,
                                           IPA_ADDR_LIST, 0,
                                           GRANULE_DELEGATED,
                                           GRANULE_DATA, 0);

    if ((output.top != IPA_ADDR_LIST + PAGE_SIZE) ||
        (output.range != 0UL) ||
        (output.count != 1UL) ||
        (list[0] != val_host_rmi_addr_range_desc(data, 1, RMI_PAGE_L3,
                                             RMI_OP_MEM_DELEGATED)) ||
        check_rtte(IPA_ADDR_LIST, MAP_LEVEL, RMI_DESTROYED) ||
        check_granules_delegated(data, PAGE_SIZE)) {
        LOG(ERROR, "LIST output: top=0x%x range=0x%x count=0x%x list[0]=0x%x\n",
            output.top, output.range, output.count, list[0]);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t rd)
{
    val_host_realm_ts realm = {0};
    val_host_rec_params_ts rec_params;

    realm.rec_count = 1;
    realm.rd = rd;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm, &rec_params))
    {
        LOG(ERROR, "Rec Create Failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
}

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = ASSIGNED
       RTTE[ipa].ripas = RAM/EMPTY */

    if (create_mapping(IPA_ADDR_ASSIGNED, true, c_args.rd_valid))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t src = g_undelegated_prep_sequence();
    if (src == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_rtt_data_map_init(c_args.rd_valid, data,
                                       IPA_ADDR_ASSIGNED, src, flags))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    /* save the PA before returning*/
    c_exp_output.range = data;
    return IPA_ADDR_ASSIGNED;
}

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    val_host_realm_ts realm_init;

    val_memset(&realm_init, 0, sizeof(realm_init));

    realm_init.s2sz = 40;
    realm_init.hash_algo = RMI_HASH_SHA_256;
    realm_init.s2_starting_level = 0;
    realm_init.num_s2_sl_rtts = 1;
    realm_init.vmid = vmid;

    if (val_host_realm_create_common(&realm_init))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    realm_test[vmid].rd = realm_init.rd;
    realm_test[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;

    return realm_init.rd;
}

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&c_args.flags_valid, &unmap_flags, sizeof(unmap_flags));

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->rd = c_args.rd_valid;
    args->base = c_args.ipa_valid;
    args->top = c_args.ipa_valid + PAGE_SIZE;
    args->flags = c_args.flags_valid;
    args->oaddr = 0;

    switch (label)
    {
         case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            break;


        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case BASE_UNALIGNED:
            args->base++;
            break;

        case TOP_UNALIGNED:
            args->top++;
            break;

        case SIZE_INVALID:
            args->top = args->base;
            break;

        case IPA_UNPROTECTED:
            args->base = ipa_unprotected_unmapped_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;

        case OADDR_LIST_UNALIGNED:
            args->flags = RMI_ADDR_TYPE_LIST;
            args->oaddr = 1;
            break;

        case OADDR_LIST_INVALID_PAS:
            args->flags = RMI_ADDR_TYPE_LIST;
            args->oaddr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case RTTE_SIZE:
            args->base = mapped_l2_prep_sequence();
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        case TRACKING_COARSE:
            args->base = IPA_ADDR_BLOCK;
            args->top = args->base + PAGE_SIZE;
            return tracking_prep_sequence();

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

void cmd_rtt_data_unmap_host(void)
{
    uint64_t ret = 0, i, data, ipa;
    struct arguments args;
    struct cmd_output output_val;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Skipping Check %d\n", (int)(i + 1));
            continue;
        }
        if (ret != VAL_SUCCESS) {
            LOG(ERROR, " Intent to sequence failed \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = rtt_data_unmap(args.rd, args.base, args.top,
                             args.flags, args.oaddr, &output_val);
        if (test_data[i].label == TRACKING_COARSE)
        {
            if ((RMI_STATUS(ret) != RMI_ERROR_TRACKING) ||
                (RMI_TRACKING_LEVEL(ret) != RMI_TRACKING_LEVEL_COARSE) ||
                (VAL_EXTRACT_BITS(ret, 16, 55) !=
                 (PLATFORM_TRACKING_TEST_REGION_BASE >> PAGE_BITS_4K)))
            {
                LOG(ERROR, "DATA_UNMAP tracking check returned 0x%lx, expected "
                    "TRACKING at 0x%lx\n", ret, PLATFORM_TRACKING_TEST_REGION_BASE);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto exit;
            }

            if (tracking_cleanup() != VAL_SUCCESS)
            {
                LOG(ERROR, "DATA_UNMAP tracking setup cleanup failed\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
                goto exit;
            }
            continue;
        }

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        if ((test_data[i].label == RD_STATE_REC) &&
            (val_host_rmi_rec_destroy(args.rd) != RMI_SUCCESS)) {
            LOG(ERROR, "Temporary REC cleanup failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if ((test_data[i].label == RTTE_SIZE) &&
            check_and_remove_l2_mapping()) {
            LOG(ERROR, "L2 DATA mapping cleanup/observability failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

    }

    LOG(TEST, "Check %2d : Single output and RAM RIPAS\n", ++i);

    ret = rtt_data_unmap(c_args.rd_valid, c_args.ipa_valid,
                         c_args.ipa_valid + PAGE_SIZE,
                         c_args.flags_valid, 0, &output_val);
    if ((ret != RMI_SUCCESS) ||
        check_single_output(&output_val, c_exp_output.range,
                            c_args.ipa_valid + PAGE_SIZE, RMI_PAGE_L3) ||
        check_rtte(c_args.ipa_valid, MAP_LEVEL, RMI_DESTROYED) ||
        check_granules_delegated(c_exp_output.range, PAGE_SIZE)) {
        LOG(ERROR, "RMI_RTT_DATA_UNMAP single/RAM check failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    LOG(TEST, "Check %2d : DESTROYED RIPAS remains DESTROYED\n", ++i);
    data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    ret = rtt_data_map(c_args.rd_valid, data, c_args.ipa_valid,
                       PAGE_SIZE, RMI_PAGE_L3);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "RMI_RTT_DATA_MAP failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    ret = rtt_data_unmap(c_args.rd_valid, c_args.ipa_valid,
                         c_args.ipa_valid + PAGE_SIZE,
                         c_args.flags_valid, 0, &output_val);
    if ((ret != RMI_SUCCESS) ||
        check_single_output(&output_val, data,
                            c_args.ipa_valid + PAGE_SIZE, RMI_PAGE_L3) ||
        check_rtte(c_args.ipa_valid, MAP_LEVEL, RMI_DESTROYED) ||
        check_granules_delegated(data, PAGE_SIZE)) {
        LOG(ERROR, "RMI_RTT_DATA_UNMAP DESTROYED check failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    LOG(TEST, "Check %2d : EMPTY RIPAS remains EMPTY\n", ++i);
    ipa = ipa_protected_assigned_empty_prep_sequence(c_args.rd_valid);
    if (ipa == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    ret = rtt_data_unmap(c_args.rd_valid, ipa, ipa + PAGE_SIZE,
                         c_args.flags_valid, 0, &output_val);
    data = output_val.range & ADDR_RANGE_ADDR_MASK;
    if ((ret != RMI_SUCCESS) ||
        check_single_output(&output_val, data, ipa + PAGE_SIZE,
                            RMI_PAGE_L3) ||
        check_rtte(ipa, MAP_LEVEL, RMI_EMPTY) ||
        check_granules_delegated(data, PAGE_SIZE)) {
        LOG(ERROR, "RMI_RTT_DATA_UNMAP EMPTY check failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    LOG(TEST, "Check %2d : LIST output descriptor\n", ++i);
    ret = check_list_unmap();
    if (ret) {
        LOG(ERROR, "LIST-mode RMI_RTT_DATA_UNMAP is not supported\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (tracking_cleanup() != VAL_SUCCESS)
    {
        LOG(ERROR, "DATA_UNMAP tracking setup cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
    }
    return;
}
