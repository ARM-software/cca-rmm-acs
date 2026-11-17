/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"
#include "val_host_rmi_sro.h"
#include "rmi_realm_create_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define NUM_REALMS 2
#define REALM_VALID 0
#define REALM_NEW 1

#define IPA_ADDR_DATA 0

static val_host_realm_ts realm[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t params_valid;
    val_host_tracking_test_region_ts track_region;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t params_ptr;
};

static uint64_t rd_track_prep_sequence(void)
{
    return val_host_tracking_coarse_prep_sequence(&c_args.track_region);
}

static uint32_t rd_track_cleanup(void)
{
    return val_host_tracking_coarse_cleanup(&c_args.track_region);
}

typedef enum {
    PARAMS_HASH_INVALID = 0x0,
    PARAMS_HASH_UNSUPPORTED,
    PARAMS_PMU_UNSUPPORTED,
    PARAMS_SVE_UNSUPPORTED,
    PARAMS_LPA2_UNSUPPORTED,
    PARAMS_BPS_UNSUPPORTED,
    PARAMS_WPS_UNSUPPORTED,
    PARAMS_RTT_UNALIGNED,
    PARAMS_S2SZ_UNSUPPORTED,
    PARAMS_INVALID_RTT_CONFIG,
    PARAMS_RTT_UNDELEGATED,
    PARAMS_BPS_ZERO,
    PARAMS_WPS_ZERO,
    PARAMS_DA_UNSUPPORTED,
    PARAMS_ATS_UNSUPPORTED,
    PARAMS_ATS_PLANE_INVALID
} params_prep_seq_type;

static uint64_t rd_valid_prep_sequence(void)
{
    uint64_t rd = g_delegated_prep_sequence();
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Track for reuse/destruction */
    realm[REALM_VALID].rd = rd;

    return rd;
}

static uint64_t g_params_prep_sequence(params_prep_seq_type type)
{
    /*  Allocate a granule for RealmParams */
    val_host_realm_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (params == NULL) {
        LOG(ERROR, "Failed to allocate RealmParams\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(params, 0, PAGE_SIZE);

    /* Allocate and delegate a granule for the L0 RTT */
    uint64_t rtt_base = realm[REALM_VALID].rtt_l0_addr;

    uint64_t featreg0;
    uint64_t featreg1;
    uint64_t featreg2;

    params->hash_algo = RMI_SHA256;
    params->s2sz = IPA_WIDTH;
    params->rtt_level_start = 0;
    params->rtt_num_start = 1;
    params->rtt_base = rtt_base;
    params->num_bps = 1;
    params->num_wps = 1;

    switch (type) {
    /* Generate a Invalid encoding for Hash algorithm */
    case PARAMS_HASH_INVALID:
        params->hash_algo = 3;
        break;

    /* Select a hash algorithm not supported by the implementation */
    case PARAMS_HASH_UNSUPPORTED:

        val_host_rmi_features(1, &featreg1);

        if (!VAL_EXTRACT_BITS(featreg1, 3, 3))
            params->hash_algo = RMI_HASH_SHA_256;
        else if (!VAL_EXTRACT_BITS(featreg1, 5, 5))
            params->hash_algo = RMI_HASH_SHA_512;
        else if (!VAL_EXTRACT_BITS(featreg1, 4, 4))
            params->hash_algo = RMI_HASH_SHA_384;
        else {
            LOG(TEST, "No Invalid combitation\n");
            return VAL_SKIP_CHECK;
        }
        break;

    case PARAMS_PMU_UNSUPPORTED:

        val_host_rmi_features(0, &featreg0);

        params->flags0 = REALM_FLAG_PMU_ENABLE;
        if (VAL_EXTRACT_BITS(featreg0, 26, 26) != 0)
            params->pmu_num_ctrs =
                (uint8_t)(VAL_EXTRACT_BITS(featreg0, 27, 31) + 1);

        break;

    case PARAMS_SVE_UNSUPPORTED:

        val_host_rmi_features(0, &featreg0);

        if (VAL_EXTRACT_BITS(featreg0, 9, 9)) {
            if (VAL_EXTRACT_BITS(featreg0, 10, 13) == 0xF) {
                LOG(TEST, "No Invalid combitation\n");
                return VAL_SKIP_CHECK;
            }
            params->sve_vl |= (uint8_t)(VAL_EXTRACT_BITS(featreg0, 10, 13) + 1);
        }
        else
            params->flags0 |= 0x2;

        break;

    case PARAMS_LPA2_UNSUPPORTED:

        val_host_rmi_features(0, &featreg0);

        if (!VAL_EXTRACT_BITS(featreg0, 8, 8))
        {
            params->flags0 = 0x1;
            params->s2sz = 52;
        }
        else {
            LOG(TEST, "No Invalid combitation\n");
            return VAL_SKIP_CHECK;
        }

        break;

    case PARAMS_BPS_UNSUPPORTED:

        val_host_rmi_features(0, &featreg0);

        if (VAL_EXTRACT_BITS(featreg0, 14, 19) == 0x0) {
            LOG(TEST, "No Invalid combitation\n");
            return VAL_SKIP_CHECK;
        }
        params->num_bps = (uint8_t)(VAL_EXTRACT_BITS(featreg0, 14, 19) + 1);
        break;

    case PARAMS_WPS_UNSUPPORTED:

        val_host_rmi_features(0, &featreg0);

        params->num_wps = (uint8_t)(VAL_EXTRACT_BITS(featreg0, 20, 25) + 1);
        break;

    case PARAMS_BPS_ZERO:
        params->num_bps = 0;
        break;

    case PARAMS_WPS_ZERO:
        params->num_wps = 0;
        break;

    case PARAMS_DA_UNSUPPORTED:
        val_host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX, &featreg2);
        if (VAL_EXTRACT_BITS(featreg2, RMI_FEATURE_REGISTER_2_DA,
                             RMI_FEATURE_REGISTER_2_DA))
            return VAL_SKIP_CHECK;
        params->flags0 = 1UL << 3;
        break;

    case PARAMS_ATS_UNSUPPORTED:
        val_host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX, &featreg2);
        if (VAL_EXTRACT_BITS(featreg2, RMI_FEATURE_REGISTER_2_ATS,
                             RMI_FEATURE_REGISTER_2_ATS))
            return VAL_SKIP_CHECK;
        params->flags1 = 1UL << 2;
        break;

    /* Select a unalligned address for rtt */
    case PARAMS_RTT_UNALIGNED:
        params->rtt_base = g_unaligned_prep_sequence(rtt_base);
        break;

    /* Select an IPA width above the advertised maximum. */
    case PARAMS_S2SZ_UNSUPPORTED:
        val_host_rmi_features(0, &featreg0);
        if (VAL_EXTRACT_BITS(featreg0, 0, 7) == UINT8_MAX)
            return VAL_SKIP_CHECK;
        params->s2sz = (uint8_t)(VAL_EXTRACT_BITS(featreg0, 0, 7) + 1U);
        break;

    /* Select an invalid RTT starting level. */
    case PARAMS_INVALID_RTT_CONFIG:
        params->rtt_num_start++;
        break;

    /* Select a undelegated granule for rtt */
    case PARAMS_RTT_UNDELEGATED:
        params->rtt_base = g_undelegated_prep_sequence();
        if (params->rtt_base == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;
        break;

    case PARAMS_ATS_PLANE_INVALID:
        params->ats_plane = 1;
        break;

    default:
        LOG(ERROR, " INVALID PREP_SEQUENCE \n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return (uint64_t)params;
}

static uint64_t params_valid_prep_sequence(void)
{
    /* Allocate a granule for RealmParams */
    val_host_realm_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    val_memset(params, 0, PAGE_SIZE);

    /* Allocate and delegate a granule for the L0 RTT */
    uint64_t rtt0 = g_delegated_prep_sequence();

    realm[REALM_VALID].rtt_l0_addr = rtt0;

    params->hash_algo = RMI_SHA256;
    params->s2sz = IPA_WIDTH;
    params->rtt_num_start = 1;
    params->rtt_level_start = 0;
    params->rtt_base = rtt0;
    params->num_bps = 1;
    params->num_wps = 1;

    return (uint64_t)params;
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
        LOG(ERROR, "Couldn't destroy the Realm\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
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
    realm[vmid].rd = realm_init.rd;
    realm[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;
    return realm_init.rd;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.params_valid = params_valid_prep_sequence();
    if (c_args.params_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PARAMS_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_valid);
            break;

        case PARAMS_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case HASH_ALGO_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_HASH_INVALID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case S2SZ_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_S2SZ_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case HASH_ALGO_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_HASH_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case PMU_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_PMU_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case SVE_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_SVE_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case LPA2_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_LPA2_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case BPS_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_BPS_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case WPS_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_WPS_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case BPS_ZERO:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_BPS_ZERO);
            break;

        case WPS_ZERO:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_WPS_ZERO);
            break;

        case DA_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_DA_UNSUPPORTED);
            if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case ATS_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_ATS_UNSUPPORTED);
            if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;


        case RTT_BASE_RD_ALIASED:
            args->rd = realm[REALM_VALID].rtt_l0_addr;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->params_ptr = c_args.params_valid;
            break;

        case RD_TRACK_COARSE:
            args->rd = rd_track_prep_sequence();
            if (args->rd == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_RD:
            args->rd = g_rd_new_prep_sequence(REALM_NEW);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(realm[REALM_NEW].rd);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[REALM_NEW].rtt_l0_addr;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(realm[REALM_NEW].rd, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->params_ptr = c_args.params_valid;
            break;

        case RTT_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_RTT_UNALIGNED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

         case RTT_CONFIG_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_INVALID_RTT_CONFIG);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RTT_BASE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_RTT_UNDELEGATED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case ATS_PLANE_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_ATS_PLANE_INVALID);
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}


void cmd_realm_create_host(void)
{
    uint64_t ret;
    val_smc_param_ts cmd_ret;
    struct arguments args = {0, 0};
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

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Skipping Check %d\n", (int)(i + 1));
            continue;
        }
        else if (ret == VAL_ERROR) {
            LOG(ERROR, " Intent to sequence failed \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_realm_create_smc(args.rd, args.params_ptr);
        ret = val_host_sro_complete(&cmd_ret);

        if (test_data[i].label == RD_TRACK_COARSE)
        {
            if ((RMI_STATUS(ret) != RMI_ERROR_TRACKING) ||
                (RMI_TRACKING_LEVEL(ret) != RMI_TRACKING_LEVEL_COARSE) ||
                (VAL_EXTRACT_BITS(ret, 16, 55) != (args.rd >> PAGE_BITS_4K)))
            {
                LOG(ERROR, "RD tracking check returned 0x%lx, expected "
                    "TRACKING at 0x%lx\n", ret, args.rd);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto exit;
            }
            continue;
        }

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index))
        {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                            ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    ret = val_host_rmi_realm_create(c_args.rd_valid, c_args.params_valid);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0) {
        LOG(ERROR, "Positive Observability Check Failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    LOG(TEST, "Check %2d : Negative Observability\n", ++i);
    cmd_ret = val_host_rmi_realm_create_smc(c_args.rd_valid, c_args.params_valid);
    ret = val_host_sro_complete(&cmd_ret);
    /* Now the command should fail with the same parameters (footprint has changed) */
    if (ret == 0) {
        LOG(ERROR, "Negative Observability Check Failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (rd_track_cleanup() != VAL_SUCCESS)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
    return;
}
