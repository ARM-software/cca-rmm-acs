/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_HOST_DA_H
#define VAL_HOST_DA_H

#include "val_host_realm.h"

/* Up to 6 PCIe memory BARs */
#define NCOH_ADDR_RANGE_NUM    6U

/* SPDM_MAX_CERTIFICATE_CHAIN_SIZE is 64KB */
#define VAL_HOST_PDEV_CERT_LEN_MAX        (64 * 1024)

/* A single page for storing VCA should be enough */
#define VAL_HOST_PDEV_VCA_LEN_MAX        (4 * 1024)

/*
 * Measurement max supported is 4KB.
 * todo: This will be increased if device supports returning more measurements
 */
#define VAL_HOST_PDEV_MEAS_LEN_MAX        (4 * 1024)
#define VAL_HOST_VDEV_MEAS_LEN_MAX        (4 * 1024)
#define VAL_HOST_VDEV_IFC_REPORT_LEN_MAX    (8 * 1024)

typedef struct{
    /* PDEV related fields */
    uint64_t pdev;
    unsigned long pdev_flags;
    void *pdev_aux[PDEV_PARAM_AUX_GRANULES_MAX];
    uint32_t pdev_aux_num;
    val_host_dev_comm_data_ts *dev_comm_data;

    /* Algorithm used to generate device digests */
    uint8_t pdev_hash_algo;

    /* Certificate, public key fields */
    uint8_t cert_slot_id;
    uint8_t *cert_chain;
    size_t cert_chain_len;
    uint8_t *vca;
    size_t vca_len;
    void *public_key;
    size_t public_key_len;
    void *public_key_metadata;
    size_t public_key_metadata_len;
    unsigned char public_key_sig_algo;
    uint32_t bdf;
    uint32_t doe_cap_base;
    uint16_t root_id;
    unsigned long ncoh_num_addr_range;
    val_host_address_range_ts ncoh_addr_range[NCOH_ADDR_RANGE_NUM];
} val_host_pdev_ts;

typedef struct {
    uint64_t vdev;
    uint64_t pdev;
    uint64_t vdev_id;

    /*
     * The TEE device interface ID. Currently NS host assigns the whole
     * device, this value is same as PDEV's bdf.
     */
    uint64_t tdi_id;

    uint64_t flags;

    void *vdev_aux[VDEV_PARAM_AUX_GRANULES_MAX];
    uint32_t vdev_aux_num;

    val_host_dev_comm_data_ts *dev_comm_data;

    /* Fields related to cached device measurements. */
    uint8_t *meas;
    size_t meas_len;

    /* Fields related to cached interface report. */
    uint8_t *ifc_report;
    size_t ifc_report_len;
    uint16_t vmid;
    uint64_t rec_count;
    uint64_t realm_rd;
} val_host_vdev_ts;

uint64_t val_host_dev_communicate(val_host_realm_ts *realm,
                val_host_pdev_ts *pdev_obj,
                val_host_vdev_ts *vdev_obj,
                unsigned char target_state);

void val_host_get_addr_range(val_host_pdev_ts *pdev_obj);

uint32_t val_host_vdev_teardown(val_host_realm_ts *realm,
                val_host_pdev_ts *pdev_obj,
                val_host_vdev_ts *vdev_obj);

uint32_t val_host_pdev_teardown(val_host_pdev_ts *pdev_obj,
                uint64_t pdev_ptr);

#endif /* VAL_HOST_DA_H */
