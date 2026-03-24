/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_RHI_H_
#define _PAL_RHI_H_

/* RHI HOSTCONF capability bit definitions */
#define RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT_SUPPORTED  (1ULL << 0)

/* Combined RHI HOSTCONF supported capability mask */
#define RHI_HOSTCONF_SUPPORTED_MASK  RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT_SUPPORTED

/* RHI FAL capability bit definitions */
#define RHI_FAL_GET_SIZE_SUPPORTED    (1ULL << 0)
#define RHI_FAL_READ_SUPPORTED        (1ULL << 1)

/* Combined RHI FAL supported capability mask */
#define RHI_FAL_SUPPORTED_MASK  ( \
    RHI_FAL_GET_SIZE_SUPPORTED | \
    RHI_FAL_READ_SUPPORTED \
)

/* RHI DA capability bit definitions */
#define RHI_DA_OBJECT_SIZE_SUPPORTED                 (1ULL << 0)
#define RHI_DA_OBJECT_READ_SUPPORTED                 (1ULL << 1)
#define RHI_DA_VDEV_CONTINUE_SUPPORTED               (1ULL << 2)
#define RHI_DA_VDEV_GET_MEASUREMENTS_SUPPORTED       (1ULL << 3)
#define RHI_DA_VDEV_GET_INTERFACE_REPORT_SUPPORTED   (1ULL << 4)
#define RHI_DA_VDEV_SET_TDI_STATE_SUPPORTED          (1ULL << 5)
#define RHI_DA_VDEV_P2P_UNBIND_SUPPORTED              (1ULL << 6)
#define RHI_DA_VDEV_ABORT_SUPPORTED                  (1ULL << 7)

/* Combined RHI DA supported capability mask */
#define RHI_DA_SUPPORTED_MASK  ( \
    RHI_DA_OBJECT_SIZE_SUPPORTED | \
    RHI_DA_OBJECT_READ_SUPPORTED | \
    RHI_DA_VDEV_CONTINUE_SUPPORTED | \
    RHI_DA_VDEV_GET_MEASUREMENTS_SUPPORTED | \
    RHI_DA_VDEV_GET_INTERFACE_REPORT_SUPPORTED | \
    RHI_DA_VDEV_SET_TDI_STATE_SUPPORTED | \
    RHI_DA_VDEV_P2P_UNBIND_SUPPORTED | \
    RHI_DA_VDEV_ABORT_SUPPORTED \
)

/* Session call capability bit definitions */
#define RHI_SESSION_OPEN_SUPPORTED     (1ULL << 0)
#define RHI_SESSION_CLOSE_SUPPORTED    (1ULL << 1)
#define RHI_SESSION_SEND_SUPPORTED     (1ULL << 2)
#define RHI_SESSION_RECEIVE_SUPPORTED  (1ULL << 3)

/* Combined supported call mask */
#define RHI_SESSION_SUPPORTED_MASK  ( \
    RHI_SESSION_OPEN_SUPPORTED | \
    RHI_SESSION_CLOSE_SUPPORTED | \
    RHI_SESSION_SEND_SUPPORTED | \
    RHI_SESSION_RECEIVE_SUPPORTED \
)

/* Connection mode capability bit definitions */
#define RHI_CONN_MODE_BLOCKING       (1ULL << 0)
#define RHI_CONN_MODE_NON_BLOCKING   (1ULL << 1)

/* Combined supported connection mode mask */
#define RHI_CONN_MODE_SUPPORTED_MASK  ( \
    RHI_CONN_MODE_BLOCKING | \
    RHI_CONN_MODE_NON_BLOCKING \
)

uint64_t pal_rhi_alloc_session_id(void);
#endif
