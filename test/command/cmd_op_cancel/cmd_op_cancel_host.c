/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"
#include "command_common_host.h"
#include "rmi_op_cancel_data.h"

#define RMI_OP_INVALID_HANDLE 0xFFFFFFFFFFFFFFFFULL
#define RMI_RESULT_CANCEL(result) VAL_EXTRACT_BITS((result), 10, 10)

struct arguments {
    uint64_t handle;
    val_smc_param_ts sro_args;
    uint64_t rec;
    uint64_t rd;
    val_host_realm_ts realm;
    uint64_t params_addr;
    bool rec_sro;
    bool rd_delegated;
    bool rtt_delegated;
    bool realm_created;
    bool realm_sro_active;
    bool cancel_started;
};

static struct argument_store {
    struct arguments valid;
} c_args;

static uint64_t rec_create_cannot_cancel_prep_sequence(struct arguments *args)
{
    val_host_realm_ts realm;
    val_host_rec_params_ts *params;

    val_memset(&realm, 0, sizeof(realm));
    realm.s2sz = 40;
    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;

    if (val_host_realm_create_common(&realm))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_ERROR;
    }

    args->rd = realm.rd;
    args->rec = g_delegated_prep_sequence();
    if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        return VAL_ERROR;

    val_memset(params, 0, PAGE_SIZE);
    params->pc = 0;
    params->flags = RMI_RUNNABLE;
    params->mpidr = 0;
    params->num_aux = 0;

    args->sro_args = val_host_rmi_rec_create_smc(args->rd, args->rec,
                                                 (uint64_t)params);
    val_host_mem_free(params);

    if (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "REC_CREATE did not return RMI_INCOMPLETE, ret=0x%x\n",
            args->sro_args.x0);
        return VAL_ERROR;
    }

    if (RMI_RESULT_CANCEL(args->sro_args.x0) != RMI_OP_CANNOT_CANCEL)
    {
        LOG(ERROR, "REC_CREATE SRO is cancelable, ret=0x%x\n",
            args->sro_args.x0);
        return VAL_ERROR;
    }

    args->handle = args->sro_args.x1;
    args->rec_sro = true;

    return VAL_SUCCESS;
}

static uint64_t realm_create_cancel_prep_sequence(struct arguments *args)
{
    val_host_realm_params_ts *params;

    val_host_realm_params(&args->realm);
    args->realm.rec_count = 0;

    args->realm.rd = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    args->realm.rtt_l0_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    args->params_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((args->realm.rd == 0) || (args->realm.rtt_l0_addr == 0) ||
        (args->params_addr == 0))
    {
        LOG(ERROR, "Failed to allocate Realm create inputs\n");
        return VAL_ERROR;
    }

    if (val_host_rmi_granule_range_delegate(args->realm.rd,
                                            args->realm.rd + PAGE_SIZE))
        return VAL_ERROR;
    args->rd_delegated = true;

    if (val_host_rmi_granule_range_delegate(args->realm.rtt_l0_addr,
                                            args->realm.rtt_l0_addr + PAGE_SIZE))
        return VAL_ERROR;
    args->rtt_delegated = true;

    params = (val_host_realm_params_ts *)args->params_addr;
    val_memset(params, 0, PAGE_SIZE);
    params->flags0 = args->realm.flags;
    params->flags1 = args->realm.flags1;
    params->pmu_num_ctrs = args->realm.pmu_num_ctrs;
    params->s2sz = args->realm.s2sz;
    params->rtt_base = args->realm.rtt_l0_addr;
    params->hash_algo = args->realm.hash_algo;
    params->rtt_level_start = (int64_t)args->realm.s2_starting_level;
    params->rtt_num_start = args->realm.num_s2_sl_rtts;
    params->num_aux_planes = args->realm.num_aux_planes;
    params->num_bps = args->realm.num_bps + 1;
    params->num_wps = args->realm.num_wps + 1;
    val_memcpy(&params->rpv, &args->realm.rpv, sizeof(args->realm.rpv));

    args->sro_args = val_host_rmi_realm_create_smc(args->realm.rd,
                                                   args->params_addr);
    if (RMI_STATUS(args->sro_args.x0) == RMI_SUCCESS)
    {
        args->realm_created = true;
        LOG(TEST, "RMI_REALM_CREATE completed without an SRO\n");
        return VAL_SKIP_CHECK;
    }

    if (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_REALM_CREATE failed, ret=0x%lx\n", args->sro_args.x0);
        return VAL_ERROR;
    }

    args->handle = args->sro_args.x1;
    args->realm_sro_active = true;
    if ((VAL_HOST_SRO_MEM_REQ(args->sro_args.x0) != RMI_OP_MEM_REQ_DONATE) ||
        (RMI_RESULT_CANCEL(args->sro_args.x0) != RMI_OP_CAN_CANCEL))
    {
        LOG(TEST, "RMI_REALM_CREATE did not start a cancelable donation SRO, "
            "ret=0x%lx\n", args->sro_args.x0);
        return VAL_SKIP_CHECK;
    }

    return VAL_SUCCESS;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t ret;

    val_memset(&c_args, 0, sizeof(c_args));

    ret = realm_create_cancel_prep_sequence(&c_args.valid);
    if (ret == VAL_SKIP_CHECK)
        return VAL_SKIP_CHECK;

    if (ret != VAL_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    val_memset(args, 0, sizeof(*args));

    switch (label)
    {
        case HANDLE_INVALID:
            args->handle = RMI_OP_INVALID_HANDLE;
            break;

        case HANDLE_CANNOT_CANCEL:
            if (rec_create_cannot_cancel_prep_sequence(args))
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t complete_sro(struct arguments *args)
{
    uint64_t ret;

    if (!args->rec_sro)
        return VAL_SUCCESS;

    ret = val_host_sro_complete(&args->sro_args);
    if (ret != RMI_SUCCESS)
        return ret;

    val_host_update_granule_state(args->rd, GRANULE_REC, args->rec, 0, 0, 0);

    return VAL_SUCCESS;
}

static uint64_t cleanup_cancel_realm(struct arguments *args)
{
    val_smc_param_ts cmd_ret;
    uint64_t ret;

    if (args->realm_sro_active)
    {
        ret = val_host_sro_complete_with_handle(&args->sro_args, args->handle);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Realm create SRO cleanup failed, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }

        args->realm_sro_active = false;
        if (!args->cancel_started)
            args->realm_created = true;
    }

    if (args->realm_created)
    {
        (void)val_host_rmi_realm_terminate(args->realm.rd);
        cmd_ret = val_host_rmi_realm_destroy_smc(args->realm.rd);
        ret = val_host_sro_complete(&cmd_ret);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Realm destroy cleanup failed, ret=0x%lx\n", ret);
            return VAL_ERROR;
        }
    }

    if (args->rtt_delegated &&
        val_host_rmi_granule_range_undelegate(args->realm.rtt_l0_addr,
                                              args->realm.rtt_l0_addr + PAGE_SIZE))
        return VAL_ERROR;

    if (args->rd_delegated &&
        val_host_rmi_granule_range_undelegate(args->realm.rd,
                                              args->realm.rd + PAGE_SIZE))
        return VAL_ERROR;

    if (args->params_addr != 0)
        val_host_mem_free((void *)args->params_addr);
    if (args->realm.rtt_l0_addr != 0)
        val_host_mem_free((void *)args->realm.rtt_l0_addr);
    if (args->realm.rd != 0)
        val_host_mem_free((void *)args->realm.rd);

    return VAL_SUCCESS;
}

void cmd_op_cancel_host(void)
{
    uint64_t i, ret, mem_req;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    val_memset(&c_args, 0, sizeof(c_args));

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        cmd_ret = val_host_rmi_op_cancel_smc(args.handle);
        ret = cmd_ret.x0;
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            complete_sro(&args);
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        if (complete_sro(&args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    ret = valid_input_args_prep_sequence();
    if (ret == VAL_SKIP_CHECK) {
        LOG(TEST, "No valid cancelable SRO generated, skipping command success check\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }
    else if (ret == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_host_rmi_op_cancel_smc(c_args.valid.handle);
    ret = RMI_STATUS(cmd_ret.x0);
    if (ret == RMI_INCOMPLETE) {
        c_args.valid.sro_args = cmd_ret;
        c_args.valid.cancel_started = true;
    }

    if (ret != RMI_INCOMPLETE) {
        LOG(ERROR, "RMI_OP_CANCEL failed, ret=0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    mem_req = VAL_HOST_SRO_MEM_REQ(cmd_ret.x0);
    if ((mem_req != RMI_OP_MEM_REQ_NONE) &&
        (mem_req != RMI_OP_MEM_REQ_RECLAIM)) {
        LOG(ERROR, "RMI_OP_CANCEL returned invalid memory request, ret=0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    cmd_ret.x0 = val_host_sro_complete_with_handle(&cmd_ret,
                                                    c_args.valid.handle);

    if (RMI_STATUS(cmd_ret.x0) != RMI_SUCCESS) {
        LOG(ERROR, "RMI_OP_CANCEL completion failed, ret=0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }
    c_args.valid.realm_sro_active = false;

    cmd_ret = val_host_rmi_op_cancel_smc(c_args.valid.handle);
    if (cmd_ret.x0 != RMI_ERROR_INPUT) {
        LOG(ERROR, "Cancelled SRO handle is still valid, ret=0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (cleanup_cancel_realm(&c_args.valid))
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
    return;
}
