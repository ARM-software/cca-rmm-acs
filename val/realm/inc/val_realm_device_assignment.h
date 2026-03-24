/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_realm_rsi.h"

#define MAX_MMIO_RANGE_COUNT        10

typedef struct {
    uint16_t interface_info;
    uint16_t reserved;
    uint16_t msi_x_message_control;
    uint16_t lnr_control;
    uint32_t tph_control;
    uint32_t mmio_range_count;
    /* pci_tdisp_mmio_range_t mmio_range[mmio_range_count];
    * uint32_t device_specific_info_len;
    * uint8_t device_specific_info[device_specific_info_len]; */
} val_host_pci_tdisp_device_interface_report_struct_t;

typedef struct {
        uint64_t first_page;
        uint32_t number_of_pages;
        uint16_t range_attributes;
        uint16_t range_id;
} pci_tdisp_mmio_range_t;

enum INTERFACE_REPORT_OFFSETS {
    INTERFACE_INFO = 0,
    RESERVED = 2,
    MSI_X_MESSGAE_CONTROL = 4,
    LNR_CONTROL = 6,
    TPH_CONTROL = 8,
    MMIO_RANGE_COUNT = 12
};

enum INTERFACE_REPORT_SIZE_IN_BYTES {
    INTERFACE_INFO_SIZE = 2,
    RESERVED_SIZE = 2,
    MSI_X_MESSGAE_CONTROL_SIZE = 2,
    LNR_CONTROL_SIZE = 2,
    TPH_CONTROL_SIZE = 4,
    MMIO_RANGE_COUNT_SIZE = 4,
};

enum MMIO_RANGE_SIZE_IN_BYTES {
    FIRST_PAGE_SIZE = 8,
    NUM_PAGES = 4,
    RANGE_ATTRIBUTES = 2,
    RID_RANGE = 2
};

typedef struct {
    uint8_t  *digest_report;        /* data to hash */
    uint64_t  digest_report_size;
    const uint8_t *digests;         /* expected digest */
} val_realm_verify_digests_ts;

uint64_t interface_report_decoding(uint8_t *report_buff,
    val_host_pci_tdisp_device_interface_report_struct_t *interface_report,
    pci_tdisp_mmio_range_t mmio_range_struct[]);

