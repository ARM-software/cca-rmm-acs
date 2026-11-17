/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DA_PDEV_STREAM_INVARIANT_HOST_H
#define DA_PDEV_STREAM_INVARIANT_HOST_H

#include "val_host_da.h"

typedef struct {
    val_host_pdev_ts endpoint[2];
    val_host_pdev_stream_ts stream;
    val_host_rmi_pdev_stream_params_ts *params;
    uint64_t second_handle;
    uint32_t root_bdf;
} da_pdev_stream_context_ts;

bool da_pdev_stream_ranges_are_ncoh(const val_host_pdev_ts *pdev);
uint32_t da_pdev_stream_init(da_pdev_stream_context_ts *context);
void da_pdev_stream_set_params(da_pdev_stream_context_ts *context,
                               uint32_t endpoint_index, uint32_t ide_sid);
uint32_t da_pdev_stream_complete(da_pdev_stream_context_ts *context,
                                  uint32_t endpoint_index, uint64_t handle);
uint32_t da_pdev_stream_disconnect_second(da_pdev_stream_context_ts *context);
uint32_t da_pdev_stream_cleanup_endpoint(val_host_pdev_ts *pdev);
uint32_t da_pdev_stream_cleanup(da_pdev_stream_context_ts *context);

#endif /* DA_PDEV_STREAM_INVARIANT_HOST_H */
