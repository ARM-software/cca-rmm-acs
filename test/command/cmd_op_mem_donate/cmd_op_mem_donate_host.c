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
#include "rmi_op_mem_donate_data.h"

#define RMI_OP_INVALID_HANDLE 0xFFFFFFFFFFFFFFFFULL
#define RMI_PAGE_L2 1
#define TEST_ADDR_LIST_MAX_ENTRIES 2U

struct arguments {
    uint64_t handle;
    uint64_t list_addr;
    uint64_t list_alloc;
    uint64_t list_count;
    uint64_t donate_req;
    uint64_t donated_base;
    uint64_t donated_count;
    uint64_t donated_state;
    uint64_t list_check_addr;
    uint64_t list_check_count;
    uint64_t list_snapshot[TEST_ADDR_LIST_MAX_ENTRIES];
    val_smc_param_ts sro_args;
    uint64_t rec;
    uint64_t rd;
    bool sro_started;
};

static struct argument_store {
    struct arguments valid;
} c_args;

static void free_address_list(struct arguments *args);

static uint64_t save_address_list(struct arguments *args)
{
    if ((args->list_count == 0U) ||
        (args->list_count > TEST_ADDR_LIST_MAX_ENTRIES))
        return VAL_ERROR;

    args->list_check_addr = args->list_addr;
    args->list_check_count = args->list_count;
    val_memcpy(args->list_snapshot, (void *)args->list_check_addr,
               args->list_check_count * sizeof(uint64_t));

    return VAL_SUCCESS;
}

static void copy_address_list_check(struct arguments *args,
                                    const struct arguments *source)
{
    args->list_check_addr = source->list_check_addr;
    args->list_check_count = source->list_check_count;
    val_memcpy(args->list_snapshot, source->list_snapshot,
               source->list_check_count * sizeof(uint64_t));
}

static uint64_t address_list_unchanged(struct arguments *args)
{
    if (args->list_check_count == 0U)
        return VAL_SUCCESS;

    if (val_memcmp(args->list_snapshot, (void *)args->list_check_addr,
                   args->list_check_count * sizeof(uint64_t)))
    {
        LOG(ERROR, "RMI_OP_MEM_DONATE modified the RMI Address Range List\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t addr_range_desc(uint64_t addr, uint64_t count,
                                uint64_t state, uint64_t size)
{
    return size | (count << 2) | ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           (state << 62);
}

static uint64_t delegate_rec(uint64_t granule)
{
    uint64_t ret;

    ret = val_host_rmi_granule_range_delegate(granule, granule + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Granule delegate failed, granule=0x%x ret=0x%x\n",
            granule, ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t track_created_rec(uint64_t rd, uint64_t rec)
{
    val_host_update_granule_state(rd, GRANULE_REC, rec, 0, 0, 0);

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
    if ((args->rec == 0) || delegate_rec(args->rec))
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

    if (VAL_HOST_SRO_MEM_REQ(args->sro_args.x0) != RMI_OP_MEM_REQ_DONATE)
    {
        LOG(ERROR, "REC_CREATE SRO does not require donation, ret=0x%x\n",
            args->sro_args.x0);
        return VAL_ERROR;
    }

    args->handle = args->sro_args.x1;
    args->donate_req = args->sro_args.x2;
    args->sro_started = true;

    return VAL_SUCCESS;
}

static uint64_t prepare_donation_list(struct arguments *args, uint64_t count,
                                      uint64_t state, bool delegate,
                                      uint64_t size)
{
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

    if (delegate && val_host_rmi_granule_range_delegate(donated_base,
                                                        donated_base + donated_size))
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
    args->donated_base = donated_base;
    args->donated_count = count;
    args->donated_state = state;

    return save_address_list(args);
}

static uint64_t prepare_valid_donation_list(struct arguments *args)
{
    uint64_t donate_req = args->donate_req;
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);

    return prepare_donation_list(args, count, state,
                                 (state == RMI_OP_MEM_DELEGATED),
                                 VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req));
}

static uint64_t prepare_noncontiguous_donation_list(struct arguments *args)
{
    uint64_t donate_req = args->sro_args.x2;
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t donated_base;
    uint64_t donated_size;
    uint64_t second_base;
    uint64_t *list;

    donated_size = (count + 1) * PAGE_SIZE;
    if ((count < 2) || ((donated_size / PAGE_SIZE) != (count + 1)))
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

    second_base = donated_base + (2 * PAGE_SIZE);
    if ((state == RMI_OP_MEM_DELEGATED) &&
        (val_host_rmi_granule_range_delegate(donated_base,
                                             donated_base + PAGE_SIZE) ||
         val_host_rmi_granule_range_delegate(second_base,
                                             second_base + ((count - 1) * PAGE_SIZE))))
    {
        val_host_mem_free((void *)args->list_alloc);
        args->list_alloc = 0;
        args->list_addr = 0;
        return VAL_ERROR;
    }

    list = (uint64_t *)args->list_addr;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = addr_range_desc(donated_base, 1, state, size);
    list[1] = addr_range_desc(second_base, count - 1, state, size);
    args->list_count = 2;

    return save_address_list(args);
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_memset(&c_args, 0, sizeof(c_args));

    if (start_rec_create_sro(&c_args.valid) ||
        prepare_valid_donation_list(&c_args.valid))
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static void valid_sro_args_prep(struct arguments *args)
{
    args->handle = c_args.valid.handle;
    args->donate_req = c_args.valid.donate_req;
    args->sro_args = c_args.valid.sro_args;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    uint64_t donate_req, count, state;
    enum test_intent label = test_data->label;

    val_memset(args, 0, sizeof(*args));

    switch (label)
    {
        case HANDLE_INVALID:
            args->handle = RMI_OP_INVALID_HANDLE;
            args->list_addr = c_args.valid.list_addr;
            args->list_count = c_args.valid.list_count;
            copy_address_list_check(args, &c_args.valid);
            break;

        case LIST_ALIGN:
            valid_sro_args_prep(args);
            args->list_addr = c_args.valid.list_addr + 1;
            args->list_count = c_args.valid.list_count;
            copy_address_list_check(args, &c_args.valid);
            break;

        case LIST_PAS:
            valid_sro_args_prep(args);
            args->list_addr = g_secure_prep_sequence();
            args->list_count = 1;
            break;

        case MEM_GT:
            valid_sro_args_prep(args);
            donate_req = args->donate_req;
            count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req) + 1;
            state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
            if (prepare_donation_list(args, count, state,
                                      (state == RMI_OP_MEM_DELEGATED),
                                      VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req)))
                return VAL_ERROR;
            break;

        case MEM_CONTIG:
            donate_req = c_args.valid.donate_req;
            count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
            if ((VAL_HOST_SRO_DONATE_REQ_CONTIG(donate_req) != RMI_OP_MEM_CONTIG) ||
                (count < 2))
                return VAL_SKIP_CHECK;

            valid_sro_args_prep(args);
            if (prepare_noncontiguous_donation_list(args))
                return VAL_ERROR;
            break;

        case MEM_ALIGN:
            valid_sro_args_prep(args);
            donate_req = args->donate_req;
            count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
            state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
            if (prepare_donation_list(args, count, state,
                                      (state == RMI_OP_MEM_DELEGATED), RMI_PAGE_L2))
                return VAL_ERROR;
            break;

        case MEM_STATE:
            valid_sro_args_prep(args);
            donate_req = args->donate_req;
            count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
            state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req) ^ 1;
            if (prepare_donation_list(args, count, state, false,
                                      VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req)))
                return VAL_ERROR;
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

    if (!args->sro_started)
        return VAL_SUCCESS;

    if ((RMI_STATUS(cmd_ret.x0) != RMI_SUCCESS) &&
        (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE) &&
        (RMI_STATUS(cmd_ret.x0) != RMI_BUSY))
    {
        if (prepare_valid_donation_list(args))
            return VAL_ERROR;
        args->sro_args = val_host_rmi_op_mem_donate_smc(args->handle,
                                                        args->list_addr,
                                                        args->list_count);
        free_address_list(args);
        if (RMI_STATUS(args->sro_args.x0) == RMI_INCOMPLETE)
            args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                          RMI_CONTINUE_KEEP_GOING);
    }
    else if (RMI_STATUS(cmd_ret.x0) == RMI_INCOMPLETE)
        args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                      RMI_CONTINUE_KEEP_GOING);
    else
        args->sro_args = cmd_ret;

    ret = val_host_sro_complete(&args->sro_args);
    if (ret != RMI_SUCCESS)
        return ret;

    ret = track_created_rec(args->rd, args->rec);
    if (ret == VAL_SUCCESS)
        args->sro_started = false;

    return ret;
}

static void free_address_list(struct arguments *args)
{
    if (args->list_alloc != 0)
        val_host_mem_free((void *)args->list_alloc);

    args->list_alloc = 0;
    args->list_addr = 0;
}

static void cleanup_started_sro(struct arguments *args)
{
    uint64_t ret;

    if (!args->sro_started)
        return;

    if ((RMI_STATUS(args->sro_args.x0) != RMI_SUCCESS) &&
        (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE) &&
        (RMI_STATUS(args->sro_args.x0) != RMI_BUSY))
    {
        if (prepare_valid_donation_list(args))
            return;
        args->sro_args = val_host_rmi_op_mem_donate_smc(args->handle,
                                                        args->list_addr,
                                                        args->list_count);
        free_address_list(args);
        if (RMI_STATUS(args->sro_args.x0) == RMI_INCOMPLETE)
            args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                          RMI_CONTINUE_KEEP_GOING);
    }

    ret = val_host_sro_complete(&args->sro_args);
    if (ret == RMI_SUCCESS)
        track_created_rec(args->rd, args->rec);

    if ((RMI_STATUS(ret) != RMI_INCOMPLETE) &&
        (RMI_STATUS(ret) != RMI_BUSY))
        args->sro_started = false;
}

static void cleanup_valid_input_args(void)
{
    cleanup_started_sro(&c_args.valid);
}

static uint64_t check_donated_granules_internal(const struct arguments *args,
                                                uint64_t donated_count)
{
    uint64_t donated_top = args->donated_base + (donated_count * PAGE_SIZE);
    uint64_t ret;

    if ((donated_count == 0U) || (donated_top <= args->donated_base))
        return VAL_ERROR;

    if (args->donated_state == RMI_OP_MEM_DELEGATED)
        ret = val_host_rmi_granule_range_delegate(args->donated_base,
                                                  donated_top);
    else if (args->donated_state == RMI_OP_MEM_UNDELEGATED)
        ret = val_host_rmi_granule_range_undelegate(args->donated_base,
                                                    donated_top);
    else
        return VAL_ERROR;

    if (ret == RMI_SUCCESS)
    {
        LOG(ERROR, "Donated Granules remain in their original Host state\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_op_mem_donate_host(void)
{
    uint64_t i, ret, prep_ret;
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

        prep_ret = intent_to_seq(&test_data[i], &args);
        if (prep_ret == VAL_SKIP_CHECK) {
            LOG(TEST, "Skipping check: SRO request cannot exercise noncontiguous donation\n");
            continue;
        }

        if (prep_ret != VAL_SUCCESS) {
            free_address_list(&args);
            cleanup_started_sro(&args);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_op_mem_donate_smc(args.handle, args.list_addr,
                                                 args.list_count);
        args.sro_args = cmd_ret;
        if (address_list_unchanged(&args))
        {
            free_address_list(&args);
            cleanup_started_sro(&args);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
        free_address_list(&args);
        ret = cmd_ret.x0;
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            complete_sro(&args, cmd_ret);
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if ((cmd_ret.x1 != 0U) || (cmd_ret.x2 != 0U))
        {
            LOG(ERROR, "Failed donation returned unexpected outputs: X1=0x%lx X2=0x%lx\n",
                cmd_ret.x1, cmd_ret.x2);
            cleanup_started_sro(&args);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }

        if (complete_sro(&args, cmd_ret)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_host_rmi_op_mem_donate_smc(c_args.valid.handle,
                                             c_args.valid.list_addr,
                                             c_args.valid.list_count);
    c_args.valid.sro_args = cmd_ret;
    if (address_list_unchanged(&c_args.valid))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }
    free_address_list(&c_args.valid);
    ret = cmd_ret.x0;
    if ((RMI_STATUS(ret) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(ret) != RMI_OP_MEM_REQ_NONE) ||
        (cmd_ret.x1 != c_args.valid.donated_count) ||
        (cmd_ret.x2 != 0U))
    {
        LOG(ERROR, "Unexpected donation result: X0=0x%lx X1=0x%lx X2=0x%lx\n",
            cmd_ret.x0, cmd_ret.x1, cmd_ret.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (check_donated_granules_internal(&c_args.valid, cmd_ret.x1))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (complete_sro(&c_args.valid, cmd_ret)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    cleanup_valid_input_args();
    free_address_list(&c_args.valid);
    return;
}
