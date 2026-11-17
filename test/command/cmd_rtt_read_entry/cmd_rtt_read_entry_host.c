/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_rtt_read_entry_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define IPA_ADDR_DATA  (4 * PAGE_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st <START_LEVEL> Table Entry)
 *  ------------------------------------
 * |        RTTE.state = TABLE          | -> mappings created up to leaf level
 *  ------------------------------------
 * ipa: SL_SIZE
 *
 */

#define IPA_ADDR_MAPPED 0x0
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))

#define NUM_REALMS 1
#define VALID_REALM 0

#define PROTECTED_DESC_ATTR_MASK \
    (0xFFUL << 2 | 1UL << ATTR_PI_INDEX_1_SHIFT | \
     1UL << ATTR_PI_INDEX_2_SHIFT | 1UL << ATTR_PI_INDEX_3_SHIFT | \
     0xFUL << 59)

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
    uint64_t data_pa;
    uint64_t rtt_addr[VAL_RTT_MAX_LEVEL + 1];
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t ipa;
    uint64_t level;
};

static bool output_mbz_valid(const val_host_rtt_entry_ts *rtte)
{
    return (VAL_EXTRACT_BITS(rtte->state, 8, 63) == 0) &&
           (VAL_EXTRACT_BITS(rtte->ripas, 8, 63) == 0);
}

static bool protected_desc_valid(uint64_t desc, uint64_t pa)
{
    return ((desc & PROTECTED_DESC_ATTR_MASK) == 0) && (OA(desc) == pa);
}

static uint64_t rtt_unprot_map(uint64_t rd, uint64_t ipa, uint64_t desc)
{
    val_host_rmi_rtt_unprot_map_flags_ts map_flags = {0};
    uint64_t flags = 0;
    uint64_t oaddr;

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    map_flags.memattr = VAL_EXTRACT_BITS(desc, ATTR_STAGE2_MEMATTR_SHIFT,
                                        ATTR_STAGE2_MEMATTR_SHIFT + 2);
    map_flags.s2ap =
        (VAL_EXTRACT_BITS(desc, ATTR_STAGE2_AP_SHIFT,
                          ATTR_STAGE2_AP_SHIFT) << 1) |
        VAL_EXTRACT_BITS(desc, (ATTR_STAGE2_AP_SHIFT + 1),
                         (ATTR_STAGE2_AP_SHIFT + 1));
    val_memcpy(&flags, &map_flags, sizeof(map_flags));

    oaddr = RMI_PAGE_L3 |
            (1UL << 2) |
            ADDR_ALIGN_DOWN(desc, PAGE_SIZE) |
            ((uint64_t)RMI_OP_MEM_UNDELEGATED << 62);

    return val_host_rmi_rtt_unprot_map(rd, ipa, ipa + PAGE_SIZE,
                                       flags, oaddr);
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

static uint64_t primary_rtt_path_prep_sequence(void)
{
    val_host_rtt_entry_ts rtte;
    uint64_t ipa_aligned;
    uint64_t level;
    uint64_t rtt;

    if (val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_MAPPED,
                                    VAL_RTT_MAX_LEVEL, &rtte))
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rtt_addr[0] = realm_test[VALID_REALM].rtt_l0_addr;
    level = rtte.walk_level;

    while (level < VAL_RTT_MAX_LEVEL)
    {
        rtt = g_delegated_prep_sequence();
        if (rtt == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;

        level++;
        ipa_aligned = ADDR_ALIGN_DOWN(IPA_ADDR_MAPPED,
                                      val_host_rtt_level_mapsize(level - 1));
        if (val_host_rmi_rtt_create(c_args.rd_valid, rtt, ipa_aligned, level))
            return VAL_TEST_PREP_SEQ_FAILED;

        c_args.rtt_addr[level] = rtt;
    }

    return VAL_SUCCESS;
}

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = VOID
       RTTE[ipa].ripas = EMPTY */

    if (primary_rtt_path_prep_sequence())
    {
        LOG(ERROR, "Couldn't create the protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_MAPPED;
}


static uint64_t level_valid_prep_sequence(void)
{
    return VAL_RTT_MAX_LEVEL;
}

static uint64_t level_invalid_oob_prep_sequence(void)
{
    return VAL_RTT_MAX_LEVEL + 1;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t rd)
{
    val_host_realm_ts realm = {0};
    val_host_rec_params_ts rec_params;

    realm.rd = rd;
    realm.rec_count = 1;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm, &rec_params))
    {
        LOG(ERROR, "REC Create Failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.level_valid = level_valid_prep_sequence();

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;


        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "The undelegated preparation sequence failed\n");
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "The delegated preparation sequence failed\n");
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "REC preparation sequence failed\n");
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DATA:
            c_args.data_pa = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (c_args.data_pa == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "DATA preparation sequence failed\n");
                return VAL_ERROR;
            }
            args->rd = c_args.data_pa;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_oob_prep_sequence();
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->level = c_args.level_valid;
            break;

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}


void cmd_rtt_read_entry_host(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;
    val_host_rtt_entry_ts rtte;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    // Iterate over the input
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rtt_read_entry(args.rd, args.ipa, args.level, &rtte);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    /* Check the walk level and TABLE descriptor at each non-leaf level. */
    for (i = 0; i < VAL_RTT_MAX_LEVEL; i++) {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid, i, &rtte);
        if (ret) {
            LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if (rtte.walk_level != i) {
            LOG(ERROR, "Walk level did not match \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }

        if ((rtte.state != RMI_RTTE_TABLE) ||
            !output_mbz_valid(&rtte) ||
            !protected_desc_valid(rtte.desc, c_args.rtt_addr[i + 1])) {
            LOG(ERROR, "Unexpected TABLE descriptor at level %d\n", i);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
    }

    /* For the valid IPA (VOID and EMPTY), check X2(state), X3(desc) and X4(ripas). */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                       c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if ((rtte.walk_level != c_args.level_valid) ||
        (rtte.state != RMI_RTTE_VOID) ||
        (rtte.ripas != RMI_RIPAS_EMPTY) ||
        (rtte.desc != 0) || !output_mbz_valid(&rtte)) {
        LOG(ERROR, " Unexpected RTT entry.State is: %d, desc is: 0x%x\n",
                                                             rtte.state, rtte.desc);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    /* For a protected DATA entry, check attributes, address and RIPAS. */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_DATA,
                                                       c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if ((rtte.walk_level != c_args.level_valid) ||
        (rtte.state != RMI_RTTE_DATA) ||
        !protected_desc_valid(rtte.desc, c_args.data_pa) ||
        (rtte.ripas != RMI_RIPAS_RAM) || !output_mbz_valid(&rtte)) {
        LOG(ERROR, "Unexpected protected DATA descriptor or RIPAS\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    /* Check unprotected UNMAPPED_NS and MAPPED_NS outputs. */
    if (create_mapping(IPA_ADDR_UNPROTECTED, false, c_args.rd_valid))
    {
        LOG(ERROR, "Couldn't create the unprotected mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_UNPROTECTED,
                                      c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed for UNMAPPED_NS, ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto exit;
    }

    if ((rtte.walk_level != c_args.level_valid) ||
        (rtte.state != RMI_RTTE_VOID) ||
        (rtte.desc != 0) ||
        (rtte.ripas != RMI_RIPAS_EMPTY) || !output_mbz_valid(&rtte)) {
        LOG(ERROR, "Unexpected UNMAPPED_NS output\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto exit;
    }

    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED) {
        LOG(ERROR, "Undelegated preparation sequence failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto exit;
    }

    uint64_t desc = (ns | ATTR_NORMAL_WB_WA_RA | ATTR_STAGE2_AP_RW);
    if (rtt_unprot_map(c_args.rd_valid, IPA_ADDR_UNPROTECTED, desc))
    {
        LOG(ERROR, "Couldn't complete the unprotected mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_UNPROTECTED,
                                                       c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto exit;
    }

    if ((rtte.walk_level != c_args.level_valid) ||
        (rtte.state != RMI_RTTE_DATA) ||
        (rtte.desc != desc) || (rtte.ripas != RMI_RIPAS_EMPTY) ||
        !output_mbz_valid(&rtte)) {
        LOG(ERROR, " Unexpected RTT entry State is: %d\n", rtte.state);
        LOG(ERROR, " ,desc is: 0x%x & RIPAS is: %d\n", rtte.desc, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
