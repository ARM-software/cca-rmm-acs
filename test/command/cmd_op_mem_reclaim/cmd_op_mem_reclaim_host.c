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
#include "rmi_op_mem_reclaim_data.h"

#define RMI_OP_INVALID_HANDLE 0xFFFFFFFFFFFFFFFFULL

enum cleanup_mode {
    CLEANUP_NONE = 0,
    CLEANUP_REC_CREATE,
    CLEANUP_REC_DESTROY
};

struct arguments {
    uint64_t handle;
    uint64_t list_addr;
    uint64_t list_alloc;
    uint64_t list_count;
    val_smc_param_ts sro_args;
    uint64_t rec;
    uint64_t rd;
    enum cleanup_mode cleanup;
};

static struct argument_store {
    struct arguments valid;
} c_args;

static void cleanup_prepared_args(struct arguments *args);

static uint64_t addr_range_desc(uint64_t addr, uint64_t count,
                                uint64_t state, uint64_t size)
{
    return size | (count << 2) | ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           (state << 62);
}

static uint64_t delegate_untracked(uint64_t granule)
{
    val_smc_param_ts args;

    args = val_host_rmi_granule_range_delegate_smc(granule, granule + PAGE_SIZE);
    if (RMI_STATUS(args.x0) == RMI_INCOMPLETE)
        args.x0 = val_host_sro_complete(&args);

    if (RMI_STATUS(args.x0) != RMI_SUCCESS)
    {
        LOG(ERROR, "Granule delegate failed, granule=0x%x ret=0x%x\n",
            granule, args.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t undelegate_untracked(uint64_t granule)
{
    val_smc_param_ts args;

    args = val_host_rmi_granule_range_undelegate_smc(granule, granule + PAGE_SIZE);
    if (RMI_STATUS(args.x0) == RMI_INCOMPLETE)
        args.x0 = val_host_sro_complete(&args);

    if (RMI_STATUS(args.x0) != RMI_SUCCESS)
    {
        LOG(ERROR, "Granule undelegate failed, granule=0x%x ret=0x%x\n",
            granule, args.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t destroy_created_rec(uint64_t rec)
{
    val_smc_param_ts args;
    uint64_t ret;

    args = val_host_rmi_rec_destroy_smc(rec);
    ret = val_host_sro_complete(&args);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "REC destroy failed, rec=0x%x ret=0x%x\n", rec, ret);
        return VAL_ERROR;
    }

    if (undelegate_untracked(rec))
        return VAL_ERROR;

    val_host_mem_free((void *)rec);

    return VAL_SUCCESS;
}

static uint64_t prepare_donation_list(struct arguments *args)
{
    uint64_t donate_req = args->sro_args.x2;
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t donated_base;
    uint64_t donated_size;
    uint64_t *list;

    donated_size = count * PAGE_SIZE;
    if ((count == 0) || ((donated_size / PAGE_SIZE) != count))
        return VAL_ERROR;

    args->list_alloc = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    args->list_addr = args->list_alloc;
    donated_base = (uint64_t)val_host_mem_alloc(PAGE_SIZE, donated_size);
    if ((args->list_alloc == 0) || (donated_base == 0))
    {
        if (args->list_alloc != 0)
            val_host_mem_free((void *)args->list_alloc);
        if (donated_base != 0)
            val_host_mem_free((void *)donated_base);
        args->list_alloc = 0;
        args->list_addr = 0;
        return VAL_ERROR;
    }

    if ((state == RMI_OP_MEM_DELEGATED) &&
        val_host_rmi_granule_range_delegate(donated_base, donated_base + donated_size))
    {
        val_host_mem_free((void *)args->list_alloc);
        args->list_alloc = 0;
        args->list_addr = 0;
        return VAL_ERROR;
    }

    list = (uint64_t *)args->list_addr;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = addr_range_desc(donated_base, count, state, size);
    args->list_count = 1;

    return VAL_SUCCESS;
}

static uint64_t start_rec_create_sro(struct arguments *args)
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
    args->rec = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((args->rec == 0) || delegate_untracked(args->rec))
        return VAL_ERROR;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        return VAL_ERROR;

    val_memset(params, 0, PAGE_SIZE);
    params->flags = RMI_RUNNABLE;

    args->sro_args = val_host_rmi_rec_create_smc(args->rd, args->rec,
                                                 (uint64_t)params);
    val_host_mem_free(params);

    if (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "REC_CREATE did not return RMI_INCOMPLETE, ret=0x%x\n",
            args->sro_args.x0);
        return VAL_ERROR;
    }

    args->handle = args->sro_args.x1;
    args->cleanup = CLEANUP_REC_CREATE;

    if (VAL_HOST_SRO_MEM_REQ(args->sro_args.x0) != RMI_OP_MEM_REQ_DONATE)
    {
        LOG(ERROR, "REC_CREATE SRO does not require donation, ret=0x%x\n",
            args->sro_args.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static void free_address_list(struct arguments *args)
{
    if (args->list_alloc != 0)
        val_host_mem_free((void *)args->list_alloc);

    args->list_alloc = 0;
    args->list_addr = 0;
}

static uint64_t complete_rec_create_sro(struct arguments *args)
{
    val_smc_param_ts smc_ret;
    uint64_t ret;

    if (prepare_donation_list(args))
        return VAL_ERROR;

    smc_ret = val_host_rmi_op_mem_donate_smc(args->handle, args->list_addr,
                                             args->list_count);
    free_address_list(args);
    args->sro_args = smc_ret;
    if ((RMI_STATUS(smc_ret.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(smc_ret.x0) != RMI_OP_MEM_REQ_NONE))
    {
        LOG(ERROR, "MEM_DONATE did not progress SRO, ret=0x%x\n", smc_ret.x0);
        return VAL_ERROR;
    }

    args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                  RMI_CONTINUE_KEEP_GOING);
    ret = val_host_sro_complete(&args->sro_args);
    if (ret != RMI_SUCCESS)
        return ret;

    args->cleanup = CLEANUP_NONE;

    return VAL_SUCCESS;
}

static uint64_t start_rec_destroy_reclaim_sro(struct arguments *args)
{
    if (start_rec_create_sro(args))
    {
        cleanup_prepared_args(args);
        return VAL_ERROR;
    }

    if (complete_rec_create_sro(args))
    {
        cleanup_prepared_args(args);
        return VAL_ERROR;
    }

    args->sro_args = val_host_rmi_rec_destroy_smc(args->rec);
    if (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "REC_DESTROY did not return RMI_INCOMPLETE, ret=0x%x\n",
            args->sro_args.x0);
        if (RMI_STATUS(args->sro_args.x0) == RMI_SUCCESS)
        {
            if (undelegate_untracked(args->rec) == VAL_SUCCESS)
                val_host_mem_free((void *)args->rec);
        }
        else
            destroy_created_rec(args->rec);
        return VAL_ERROR;
    }

    args->handle = args->sro_args.x1;
    args->cleanup = CLEANUP_REC_DESTROY;

    if (VAL_HOST_SRO_MEM_REQ(args->sro_args.x0) != RMI_OP_MEM_REQ_RECLAIM)
    {
        LOG(ERROR, "REC_DESTROY SRO does not require reclaim, ret=0x%x\n",
            args->sro_args.x0);
        cleanup_prepared_args(args);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t prepare_reclaim_list(struct arguments *args)
{
    args->list_alloc = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (args->list_alloc == 0)
        return VAL_ERROR;

    args->list_addr = args->list_alloc;
    val_memset((void *)args->list_addr, 0, PAGE_SIZE);
    args->list_count = VAL_HOST_SRO_LIST_MAX_ENTRIES;

    return VAL_SUCCESS;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_memset(&c_args, 0, sizeof(c_args));

    if (start_rec_destroy_reclaim_sro(&c_args.valid))
        return VAL_TEST_PREP_SEQ_FAILED;

    if (prepare_reclaim_list(&c_args.valid))
    {
        cleanup_prepared_args(&c_args.valid);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

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
            args->list_addr = c_args.valid.list_addr;
            args->list_count = c_args.valid.list_count;
            break;

        case LIST_ALIGN:
            args->handle = c_args.valid.handle;
            args->list_addr = c_args.valid.list_addr + 1;
            args->list_count = c_args.valid.list_count;
            break;

        case LIST_PAS:
            args->handle = c_args.valid.handle;
            args->list_addr = g_secure_prep_sequence();
            args->list_count = VAL_HOST_SRO_LIST_MAX_ENTRIES;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t complete_sro(struct arguments *args, val_smc_param_ts cmd_ret)
{
    uint64_t ret;

    switch (args->cleanup)
    {
        case CLEANUP_NONE:
            return VAL_SUCCESS;

        case CLEANUP_REC_CREATE:
            ret = val_host_sro_complete(&args->sro_args);
            if (ret != RMI_SUCCESS)
                return ret;

            ret = destroy_created_rec(args->rec);
            if (ret == VAL_SUCCESS)
                args->cleanup = CLEANUP_NONE;
            return ret;

        case CLEANUP_REC_DESTROY:
            if (RMI_STATUS(cmd_ret.x0) == RMI_INCOMPLETE)
                args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                              RMI_CONTINUE_KEEP_GOING);

            ret = val_host_sro_complete(&args->sro_args);
            if (ret != RMI_SUCCESS)
                return ret;

            if (undelegate_untracked(args->rec))
                return VAL_ERROR;

            val_host_mem_free((void *)args->rec);
            args->cleanup = CLEANUP_NONE;
            return VAL_SUCCESS;

        default:
            return VAL_ERROR;
    }
}

static void cleanup_prepared_args(struct arguments *args)
{
    uint64_t ret;

    free_address_list(args);

    if (args->cleanup == CLEANUP_NONE)
        return;

    if ((RMI_STATUS(args->sro_args.x0) != RMI_SUCCESS) &&
        (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE) &&
        (RMI_STATUS(args->sro_args.x0) != RMI_BUSY))
        args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                      RMI_CONTINUE_KEEP_GOING);

    ret = val_host_sro_complete(&args->sro_args);
    if (ret != RMI_SUCCESS)
        return;

    if (args->cleanup == CLEANUP_REC_CREATE)
        ret = destroy_created_rec(args->rec);
    else
    {
        ret = undelegate_untracked(args->rec);
        if (ret == VAL_SUCCESS)
            val_host_mem_free((void *)args->rec);
    }

    if (ret == VAL_SUCCESS)
        args->cleanup = CLEANUP_NONE;
}

static void cleanup_valid_input_args(void)
{
    cleanup_prepared_args(&c_args.valid);
}

void cmd_op_mem_reclaim_host(void)
{
    uint64_t i, ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            cleanup_prepared_args(&args);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_op_mem_reclaim_smc(args.handle, args.list_addr,
                                                  args.list_count);
        free_address_list(&args);
        ret = cmd_ret.x0;
        if (RMI_STATUS(ret) != test_data[i].status) {
            complete_sro(&args, cmd_ret);
            LOG(ERROR, "Test Failure!The ABI call returned: %x Expected status: %x\n",
                ret, test_data[i].status);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        if (complete_sro(&args, cmd_ret)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_host_rmi_op_mem_reclaim_smc(c_args.valid.handle,
                                              c_args.valid.list_addr,
                                              c_args.valid.list_count);
    c_args.valid.sro_args = cmd_ret;
    ret = cmd_ret.x0;
    if ((RMI_STATUS(ret) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(ret) != RMI_OP_MEM_REQ_NONE)) {
        LOG(ERROR, "RMI_OP_MEM_RECLAIM failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (cmd_ret.x1 > c_args.valid.list_count) {
        LOG(ERROR, "Reclaim output count exceeds list capacity: 0x%lx\n",
            cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if ((cmd_ret.x1 != 0U) &&
        val_host_sro_release_reclaimed(c_args.valid.list_addr, cmd_ret.x1)) {
        LOG(ERROR, "Invalid or unreleasable reclaim descriptors\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    free_address_list(&c_args.valid);

    if (complete_sro(&c_args.valid, cmd_ret)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    cleanup_valid_input_args();
    free_address_list(&c_args.valid);
    return;
}
