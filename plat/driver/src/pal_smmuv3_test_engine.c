/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include "pal_smmuv3_test_engine.h"
#include "pal_mmio.h"

int pal_smmu_configure_testengine(uintptr_t bar, uintptr_t source_pa, uintptr_t dest_pa)
{
    engine_pair_t *pairs = (engine_pair_t *)bar;
    uint32_t cur_cnt, trans_cnt, cmd;

    /* Configure DMA engine */
    privileged_frame_t *pframe = &pairs->privileged[0];
    user_frame_t *uframe = &pairs->user[0];

    /* Non-secure */
    pal_mmio_write32((uintptr_t)&pframe->pctrl, 1U);

    /* Not used with PCI */
    pal_mmio_write32((uintptr_t)&pframe->downstream_port_index, 0U);
    pal_mmio_write32((uintptr_t)&pframe->streamid, 0U);
    pal_mmio_write32((uintptr_t)&pframe->substreamid, NO_SUBSTREAMID);

    pal_mmio_write32((uintptr_t)&uframe->cmd, ENGINE_HALTED);
    pal_mmio_write32((uintptr_t)&uframe->uctrl, 0U);

    /* Configure source */
    pal_mmio_write64((uintptr_t)&uframe->begin, source_pa);
    pal_mmio_write64((uintptr_t)&uframe->end_incl, source_pa + SZ_4K - 1UL);

    /*
     * Configure attributes for source and destination:
     * rawWB, inner shareability, non-secure
     */
    pal_mmio_write32((uintptr_t)&uframe->attributes, 0x42ff42ff);

    /* Copy from start to end */
    pal_mmio_write32((uintptr_t)&uframe->seed, 0U);

    /* Don't send MSI-X */
    pal_mmio_write64((uintptr_t)&uframe->msiaddress, 0UL);
    pal_mmio_write32((uintptr_t)&uframe->msidata, 0U);
    pal_mmio_write32((uintptr_t)&uframe->msiattr, 0U);

    /* Configure destination */
    pal_mmio_write64((uintptr_t)&uframe->udata, dest_pa);

    /* Copy everything */
    pal_mmio_write64((uintptr_t)&uframe->stride, 1UL);

    /* Read the current number of transactions */
    cur_cnt = pal_mmio_read32((uintptr_t)&uframe->count_of_transactions_returned);

    /* Start memcpy */
    pal_mmio_write32((uintptr_t)&uframe->cmd, ENGINE_MEMCPY);

    /* Wait for completion */
    do {
        trans_cnt =
            pal_mmio_read32((uintptr_t)&uframe->count_of_transactions_returned);
        cmd = pal_mmio_read32((uintptr_t)&uframe->cmd);
    } while ((cmd != ENGINE_NO_FRAME) && (trans_cnt == cur_cnt));

    cmd = pal_mmio_read32((uintptr_t)&uframe->cmd);

    return (int)cmd;
}
