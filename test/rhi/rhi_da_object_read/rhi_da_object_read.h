/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef RHI_DA_OBJECT_READ_H
#define RHI_DA_OBJECT_READ_H

struct read_step {
    const char *desc;
    uint64_t vdev_id;
    uint64_t obj_type;
    uint64_t buffer_ipa;
    uint64_t max_len;
    uint64_t offset;
    uint64_t expected_status;
    bool expect_nonzero_len;
};

#define RHI_DA_OBJECT_READ_STEPS(vdev_id, buffer_ipa, invalid_vdev_id,          \
                                 invalid_object_type, invalid_buffer_ipa)       \
    {                                                                            \
        {"invalid vdev id", (invalid_vdev_id), RHI_DA_DEV_VCA,                   \
            (buffer_ipa), PAGE_SIZE, 0, RHI_DA_ERROR_INVALID_VDEV_ID, false},    \
        {"invalid object type", (vdev_id), (invalid_object_type),                \
            (buffer_ipa), PAGE_SIZE, 0, RHI_DA_ERROR_INVALID_OBJECT, false},     \
        {"unsupported object type", (vdev_id), RHI_DA_DEV_EXTENSION_EVIDENCE,    \
            (buffer_ipa), PAGE_SIZE, 0, RHI_DA_ERROR_OBJECT_UNSUPPORTED, false}, \
        {"invalid buffer IPA", (vdev_id), RHI_DA_DEV_VCA,                        \
            (invalid_buffer_ipa), PAGE_SIZE, 0, RHI_DA_ERROR_ACCESS_FAILED,      \
            false},                                                              \
        {"invalid offset", (vdev_id), RHI_DA_DEV_VCA,                            \
            (buffer_ipa), 0x10, 0x20, RHI_DA_ERROR_INVALID_OFFSET, false},       \
        {"valid object read", (vdev_id), RHI_DA_DEV_VCA,                         \
            (buffer_ipa), PAGE_SIZE, 0, RHI_DA_SUCCESS, true},                   \
    }

#define RHI_DA_OBJECT_READ_STEP_COUNT 6U

#endif /* RHI_DA_OBJECT_READ_H */
