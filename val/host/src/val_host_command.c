 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_host_realm.h"
#include "val_host_command.h"

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define L0_SIZE (512UL * L1_SIZE)
#define MAP_LEVEL 3
#define IPA_WIDTH 40
#define IPA_ADDR_PROTECTED_ASSIGNED PAGE_SIZE
#define IPA_ADDR_PROTECTED_UNASSIGNED 0
#define IPA_ADDR_DESTROYED (2 * PAGE_SIZE)
#define IPA_ADDR_RIPAS_EMPTY (3 * PAGE_SIZE)
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))
#define IPA_ADDR_UNPROTECTED_UNASSIGNED (IPA_ADDR_UNPROTECTED + PAGE_SIZE)

uint64_t create_mapping(uint64_t ipa, bool ripas_init, uint64_t rd1)
{
    /* Fetch the rd of the Realm */
    uint64_t rd = rd1;
    val_host_rtt_entry_ts rtte;

    /* Try a maximum walk and check where to create the next table entry */
    if (val_host_rmi_rtt_read_entry(rd, ipa, MAP_LEVEL, &rtte)) {
        LOG(ERROR, "\tReadEntry query failed!", 0, 0);
        return VAL_ERROR;
    }

    uint64_t level, rtt_l1, rtt_l2, rtt_l3, out_top;

    level = rtte.walk_level;

    /* Delegate granules for RTTs */
    if (level < 1) {
        rtt_l1 = val_host_delegate_granule();
        if (rtt_l1 == VAL_ERROR)
            return VAL_ERROR;

        uint64_t ipa_aligned = (ipa / L0_SIZE) * L0_SIZE;

        if (val_host_rmi_rtt_create(rd, rtt_l1, ipa_aligned, 1)) {
            LOG(ERROR, "\trtt_l1 creation failed\n", 0, 0);
            return VAL_ERROR;
        }
    }
    if (level < 2) {
        rtt_l2 = val_host_delegate_granule();
        if (rtt_l2 == VAL_ERROR)
            return VAL_ERROR;


        uint64_t ipa_aligned = (ipa / L1_SIZE) * L1_SIZE;

        if (val_host_rmi_rtt_create(rd, rtt_l2, ipa_aligned, 2)) {
            LOG(ERROR, "\trtt_l2 creation failed\n", 0, 0);
            return VAL_ERROR;
        }
    }
    if (level < 3) {
        rtt_l3 = val_host_delegate_granule();
        if (rtt_l3 == VAL_ERROR)
            return VAL_ERROR;


        uint64_t ipa_aligned = (ipa / L2_SIZE) * L2_SIZE;

        if (val_host_rmi_rtt_create(rd, rtt_l3, ipa_aligned, MAP_LEVEL)) {
            LOG(ERROR, "\trtt_l3 creation failed\n", 0, 0);
            return VAL_ERROR;
        }
    }
    if (ripas_init) {
        uint64_t ipa_aligned = (ipa / L3_SIZE) * L3_SIZE;

        if (val_host_rmi_rtt_init_ripas(rd, ipa_aligned, ipa_aligned + PAGE_SIZE, &out_top)) {
            LOG(ERROR, "\tRIPAS initialization failed\n", 0, 0);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

uint64_t val_host_delegate_granule(void)
{
    /* Allocate a src granule*/
    uint64_t *gran = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (gran != NULL) {
        if (val_host_rmi_granule_delegate((uint64_t)gran)) {
            LOG(ERROR, "\tError! granule couldn't be delegated!\n", 0, 0);
            return VAL_ERROR;
        } else {
            LOG(DBG, "\tallocation: granule @ address: %x\n",
                (uint64_t)gran, 0);
        }
    } else {
        LOG(ERROR, "\tError! granule couldn't be allocated!\n", 0, 0);
        return VAL_ERROR;
    }

    return (uint64_t)gran;
}

uint64_t val_host_undelegate_granule(void)
{
    /* Allocate a granule */
    uint64_t *gran = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (gran == NULL)
    {
        LOG(ERROR, "\t Error : Granule couldn't be allocated!\n", 0, 0);
        return VAL_ERROR;
    }
    return (uint64_t)gran;
}

