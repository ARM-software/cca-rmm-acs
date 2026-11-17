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
#include "rmi_rtt_unprot_map_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAP_LEVEL 3
#define L3_SIZE PAGE_SIZE
#define IPA_ADDR_DATA  (4 * PAGE_SIZE)
#define IPA_ADDR_LIST  (IPA_ADDR_UNPROTECTED_UNASSIGNED + PAGE_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st protected L3 Entry)
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * ipa: 0x1000
 *
 *                  (...)
 *
 *
 * ipa: 0x1XXX000 (1st unprotected L3 Entry)
 *  ------------------------------------
 * |         HIPAS = VALID_NS           |
 *  ------------------------------------
 * |        HIPAS = UNASSIGNED          |
 *  ------------------------------------
 * ipa: 0x1XX2000
 *
 */

#define IPA_ADDR_UNPROTECTED_UNASSIGNED (1UL << (IPA_WIDTH - 1)) + L3_SIZE

#define NUM_REALMS 1
#define VALID_REALM 0

static val_host_realm_ts realm_test[NUM_REALMS];
static uint64_t input_list[PAGE_SIZE / sizeof(uint64_t)]
                           __attribute__((aligned(PAGE_SIZE)));

static struct argument_store {
    uint64_t rd_valid;
    uint64_t base_valid;
    uint64_t top_valid;
    uint64_t flags_valid;
    uint64_t oaddr_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
};

static uint64_t rtt_unprot_map(struct arguments *args)
{
    return val_host_rmi_rtt_unprot_map(args->rd, args->base, args->top,
                                       args->flags, args->oaddr);
}

static uint64_t check_list_positive(void)
{
    val_host_rmi_rtt_unprot_map_flags_ts map_flags = {0};
    val_host_rtt_entry_ts rtte;
    val_smc_param_ts cmd;
    uint64_t flags;
    uint64_t ns;
    uint64_t ret;
    uint64_t observed_s2ap;

    if (create_mapping(IPA_ADDR_LIST, false, c_args.rd_valid))
        return VAL_ERROR;

    ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    val_memset(input_list, 0, PAGE_SIZE);
    input_list[0] = val_host_rmi_addr_range_desc(ns, 1, RMI_PAGE_L3,
                                             RMI_OP_MEM_UNDELEGATED);

    val_memcpy(&map_flags, &c_args.flags_valid, sizeof(map_flags));
    map_flags.oaddr_type = RMI_ADDR_TYPE_LIST;
    map_flags.list_count = 1;
    val_memcpy(&flags, &map_flags, sizeof(flags));

    cmd = val_host_rmi_rtt_unprot_map_smc(c_args.rd_valid, IPA_ADDR_LIST,
                                           IPA_ADDR_LIST + PAGE_SIZE, flags,
                                           (uint64_t)input_list);
    ret = val_host_sro_complete(&cmd);
    if (ret != RMI_SUCCESS || cmd.x1 != IPA_ADDR_LIST + PAGE_SIZE ||
        val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_LIST,
                                     MAP_LEVEL, &rtte) != RMI_SUCCESS)
        return VAL_ERROR;

    observed_s2ap = (VAL_EXTRACT_BITS(rtte.desc, 6, 6) << 1) |
                    VAL_EXTRACT_BITS(rtte.desc, 7, 7);
    if (rtte.state != RMI_VALIDN_NS ||
        (rtte.desc & VAL_HOST_RMI_ADDR_DESC_ADDR_MASK) !=
        (ns & VAL_HOST_RMI_ADDR_DESC_ADDR_MASK) ||
        VAL_EXTRACT_BITS(rtte.desc, 2, 4) != map_flags.memattr ||
        observed_s2ap != map_flags.s2ap)
        return VAL_ERROR;

    return VAL_SUCCESS;
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

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = UNASSIGNED */

    if (create_mapping(IPA_ADDR_UNPROTECTED_UNASSIGNED, false, c_args.rd_valid))
    {
        LOG(ERROR, "Couldn't create mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNPROTECTED_UNASSIGNED;
}


static uint64_t oaddr_valid_prep_sequence(void)
{
    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return val_host_rmi_addr_range_desc(ns, 1, RMI_PAGE_L3,
                                   RMI_OP_MEM_UNDELEGATED);
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
        LOG(ERROR, "REC create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.base_valid = ipa_valid_prep_sequence();
    if (c_args.base_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;
    c_args.top_valid = c_args.base_valid + PAGE_SIZE;

    val_host_rmi_rtt_unprot_map_flags_ts map_flags = {0};

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    map_flags.memattr = 0;
    map_flags.s2ap = 3;
    val_memcpy(&c_args.flags_valid, &map_flags, sizeof(map_flags));

    c_args.oaddr_valid = oaddr_valid_prep_sequence();
    if (c_args.oaddr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->rd = c_args.rd_valid;
    args->base = c_args.base_valid;
    args->top = c_args.top_valid;
    args->flags = c_args.flags_valid;
    args->oaddr = c_args.oaddr_valid;

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
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
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

        case IPA_PROTECTED:
            args->base = ipa_protected_unassigned_empty_prep_sequence(c_args.rd_valid);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        case OADDR_TYPE_NONE:
            args->flags = (args->flags & ~0x3UL) | RMI_ADDR_TYPE_NONE;
            break;

        case OADDR_TYPE_RESERVED:
            args->flags = (args->flags & ~0x3UL) | 0x3UL;
            break;

        case OADDR_LIST_UNALIGNED:
            args->flags = (args->flags & ~0x3UL) | RMI_ADDR_TYPE_LIST;
            args->oaddr = g_unaligned_prep_sequence(c_args.oaddr_valid);
            break;

        case OADDR_LIST_INVALID_PAS:
            args->flags = (args->flags & ~0x3UL) | RMI_ADDR_TYPE_LIST;
            args->oaddr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case RTTE_STATE_ASSIGNED_NS:
            args->base = ipa_unprotected_assinged_prep_sequence(c_args.rd_valid);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        case RTTE_SIZE:
            args->base = ipa_unprotected_unmapped_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}


void cmd_rtt_unprot_map_host(void)
{
    uint64_t ret;
    uint64_t observed_s2ap;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_host_rmi_rtt_unprot_map_flags_ts expected_flags = {0};
    val_smc_param_ts cmd;
    uint64_t i;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = rtt_unprot_map(&args);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    /* Check that rtte.addr and rtte.state have not changed */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.base_valid,
                                      MAP_LEVEL, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    } else {
        if (rtte.state != RMI_UNASSIGNED) {
            LOG(ERROR, "Unexpected RTT entry.\n");
            LOG(ERROR, "State is: %d & desc is: %x\n", rtte.state, rtte.desc);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    args.rd = c_args.rd_valid;
    args.base = c_args.base_valid;
    args.top = c_args.top_valid;
    args.flags = c_args.flags_valid;
    args.oaddr = c_args.oaddr_valid;
    cmd = val_host_rmi_rtt_unprot_map_smc(args.rd, args.base, args.top,
                                          args.flags, args.oaddr);
    ret = val_host_sro_complete(&cmd);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0 || cmd.x1 != c_args.top_valid) {
        LOG(ERROR, "Positive Observability Check failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Check all observable SINGLE mapping success postconditions. */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.base_valid,
                                      MAP_LEVEL, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    } else {
        val_memcpy(&expected_flags, &c_args.flags_valid,
                   sizeof(expected_flags));
        observed_s2ap =
            (VAL_EXTRACT_BITS(rtte.desc, 6, 6) << 1) |
            VAL_EXTRACT_BITS(rtte.desc, 7, 7);

        if ((rtte.state != RMI_VALIDN_NS) ||
            ((rtte.desc & VAL_HOST_RMI_ADDR_DESC_ADDR_MASK) !=
             (c_args.oaddr_valid & VAL_HOST_RMI_ADDR_DESC_ADDR_MASK)) ||
            (VAL_EXTRACT_BITS(rtte.desc, 2, 4) != expected_flags.memattr) ||
            (observed_s2ap != expected_flags.s2ap)) {
            LOG(ERROR, "Unexpected RTT entry.\n");
            LOG(ERROR, "State: %d desc: 0x%lx memattr: 0x%lx s2ap: 0x%lx\n",
                rtte.state, rtte.desc,
                VAL_EXTRACT_BITS(rtte.desc, 2, 4), observed_s2ap);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : LIST mapping success\n", ++i);
    if (check_list_positive()) {
        LOG(ERROR, "RMI_RTT_UNPROT_MAP LIST success check failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
