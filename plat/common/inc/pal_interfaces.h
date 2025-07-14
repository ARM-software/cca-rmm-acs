/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_INTERFACES_H_
#define _PAL_INTERFACES_H_

#include "pal.h"
#include "pal_config_def.h"
#include "pal_mmio.h"
#include "pal_libc.h"
#include "pal_irq.h"
#include "pal_shemaphore.h"
#include "pal_arch.h"
#include "pal_arch_helpers.h"
#include "pal_common_val_intf.h"

/**
 *   @brief    - This function prints the given string and data onto the uart
 *   @param    - str      : Input String
 *   @param    - data1    : Value for first format specifier
 *   @param    - data2    : Value for second format specifier
 *   @return   - SUCCESS(0)/FAILURE(Any positive number)
**/
uint32_t pal_printf(const char *msg, uint64_t data1, uint64_t data2);

/**
 *   @brief    - Terminates the simulation at the end of all tests completion.
 *   @param    - void
 *   @return   - SUCCESS(0)/FAILURE
**/
uint32_t pal_terminate_simulation(void);

/**
 *   @brief    - Returns number of cpu in the system.
 *   @param    - void
 *   @return   - Number of cpu
**/
uint32_t pal_get_cpu_count(void);

/**
 *   @brief    - Return base of phy_mpidr_array
 *   @param    - void
 *   @return   - Base of phy_mpidr_array
**/
uint64_t *pal_get_phy_mpidr_list_base(void);

/**
 *   @brief    - Registers ACS secure service with EL3/EL2 software
 *   @param    - void
 *   @return   - SUCCESS(0)/FAILURE
**/
uint32_t pal_register_acs_service(void);

/**
 *   @brief    - Send control back to normal world during boot phase and
 *                waits for message from normal world
 *   @param    - void
 *   @return   - SUCCESS(0)/FAILURE
**/
uint32_t pal_wait_for_sync_call(void);

/**
 *   @brief    - Send control back to normal world during test execution phase
 *                and waits for message from normal world
 *   @param    - void
 *   @return   - SUCCESS(0)/FAILURE
**/
uint32_t pal_sync_resp_call_to_host(void);
uint32_t pal_sync_resp_call_to_preempted_host(void);

/**
 *   @brief    - Normal sends message to secure world and wait for the respond
 *               from secure world
 *   @param    - void
 *   @return   - SUCCESS(0)/FAILURE
**/
uint32_t pal_sync_req_call_to_secure(void);

void pal_ns_wdog_enable(uint32_t ms);
void pal_ns_wdog_disable(void);

/**
 *   @brief    - Initializes and enable the hardware watchdog timer
 *   @param    - ms   : Number of cycles(Milliseconds)
 *   @return   - SUCCESS/FAILURE
**/
uint32_t pal_twdog_enable(uint32_t ms);

/**
 *   @brief    - Disables the hardware trusted watchdog timer
 *   @param    - void
 *   @return   - SUCCESS/FAILURE
**/
uint32_t pal_twdog_disable(void);

uint32_t pal_verify_signature(uint64_t *token);

#endif /* _PAL_INTERFACES_H_ */
