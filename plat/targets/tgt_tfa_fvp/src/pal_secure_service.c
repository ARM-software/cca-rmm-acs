/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_smc.h"
#include "pal_interfaces.h"

/* FFA 32 bit- function ids. */
#define FFA_MSG_WAIT_32              0x8400006B
#define FFA_MSG_SEND_DIRECT_REQ_32   0x8400006F
#define FFA_MSG_SEND_DIRECT_RESP_32  0x84000070
#define FFA_INTERRUPT                0x84000062
#define FFA_RUN                      0x8400006D
#define FFA_ERROR_32                 0x84000060

#define SENDER_ID(x)    (x >> 16) & 0xffff
#define RECEIVER_ID(x)  (x & 0xffff)

#define NS_HYP_ID 0x0
#define SP_ID 0x8001

static uint16_t target_id, my_id;

static uint32_t pal_ffa_msg_wait(void)
{
    pal_smc_param_t args;

    args = pal_smc_call(FFA_MSG_WAIT_32, 0, 0, 0, 0, 0, 0, 0);

     if (args.x0 != FFA_MSG_SEND_DIRECT_REQ_32)
     {
        pal_printf("\tInvalid fid received, fid=0x%x, error=0x%x\n", args.x0, args.x2);
        return PAL_ERROR;
     }

     target_id = SENDER_ID(args.x1);
     my_id = RECEIVER_ID(args.x1);
     return PAL_SUCCESS;
}

static uint32_t pal_ffa_msg_send_direct_resp_32(void)
{
    pal_smc_param_t args;

    args = pal_smc_call(FFA_MSG_SEND_DIRECT_RESP_32,
                        ((uint32_t)my_id << 16) | target_id, 0, 0, 0, 0, 0, 0);

     if ((args.x0 != FFA_MSG_SEND_DIRECT_REQ_32) && (args.x0 != FFA_INTERRUPT))
     {
          pal_printf("\tInvalid fid received, fid=0x%x, error=0x%x\n", args.x0, args.x2);
          return PAL_ERROR;
     }
     return PAL_SUCCESS;
}

static uint32_t pal_ffa_run(void)
{
    pal_smc_param_t args;

    args = pal_smc_call(FFA_RUN,
                        ((uint32_t)NS_HYP_ID << 16), 0, 0, 0, 0, 0, 0);
    if (args.x0 == FFA_ERROR_32)
    {
        pal_printf("\tFFA_RUN failed, error %lx\n", args.x2, 0);
        return PAL_ERROR;
     }

     return PAL_SUCCESS;
}

static uint32_t pal_ffa_msg_send_direct_req_32(void)
{
    pal_smc_param_t args;

    args = pal_smc_call(FFA_MSG_SEND_DIRECT_REQ_32,
                        ((uint32_t)NS_HYP_ID << 16) | SP_ID, 0, 0, 0, 0, 0, 0);

     if (args.x0 != FFA_MSG_SEND_DIRECT_RESP_32)
     {
          pal_printf("\tInvalid fid received, fid=0x%x, error=0x%x\n", args.x0, args.x2);
          return PAL_ERROR;
     }
     return PAL_SUCCESS;
}

uint32_t pal_register_acs_service(void)
{
    /* In FF-A, secure service registration happens through
     * partition manifest.
     * Port this function if communication between
     * normal world and secure world is not based
     * Firmware Framework for A Specification.
     * */
     return PAL_SUCCESS;
}

uint32_t pal_wait_for_sync_call(void)
{
    /* In FF-A, giving control to normal world happens through
     * FFA_MSG_WAIT_32 interface and normal world can send message
     * to secure world(SPMC/SP) through FFA_MSG_SEND_DIRECT_REQ_32 interface.
     * Port this function if communication between
     * normal world and secure world is not based
     * Firmware Framework for A Specification.
     * */
    return pal_ffa_msg_wait();
}

uint32_t pal_sync_resp_call_to_host(void)
{
    /* In FF-A, normal world can send message to Secure world through
     * FFA_MSG_SEND_DIRECT_REQ_32 and Secure world can respond and control
     * back using FFA_MSG_SEND_DIRECT_RESP_32 interface.
     * Port this function if communication between
     * normal world and secure world is not based
     * Firmware Framework for A Specification.
     * */
    return pal_ffa_msg_send_direct_resp_32();
}

uint32_t pal_sync_resp_call_to_preempted_host(void)
{
    /* In FF-A, giving control to preempted normal world happens through
     * FFA_RUN interface and normal world can send message
     * to secure world(SPMC/SP) through FFA_MSG_SEND_DIRECT_REQ_32 interface.
     * Port this function if communication between
     * normal world and secure world is not based
     * Firmware Framework for A Specification.
     * */
    return pal_ffa_run();
}

uint32_t pal_sync_req_call_to_secure(void)
{
    /* In FF-A, normal world can send message to secure world through
     * FFA_MSG_SEND_DIRECT_REQ_32 and secure world can respond and control
     * back using FFA_MSG_SEND_DIRECT_RESP_32 interface.
     * Port this function if communication between
     * normal world and secure world is not based
     * Firmware Framework for A Specification.
     * */
    return pal_ffa_msg_send_direct_req_32();
}
