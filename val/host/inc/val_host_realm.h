/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_REALM_H_
#define _VAL_HOST_REALM_H_

#include "val_host_rmi.h"
#include "val_libc.h"

#define VAL_MAX_REC_COUNT 8

#define VAL_RTT_BLOCK_LEVEL    2
#define VAL_RTT_MAX_LEVEL    3

#define ADDR_ALIGN_DOWN(a, b) ((unsigned long)(a) & ~(((unsigned long)(b)) - 1))
#define ADDR_IS_ALIGNED(a, b)        (((a) & ((typeof(a))(b) - 1)) == 0)

#define VAL_PAGE_SHIFT        12
#define VAL_RTT_LEVEL_SHIFT(level)    ((VAL_PAGE_SHIFT - 3) * (4 - (level)) + 3)
#define VAL_RTT_L2_BLOCK_SIZE    (1UL << VAL_RTT_LEVEL_SHIFT(2))


#define VAL_MAX_RTT_GRANULES 25
#define VAL_MAX_GRANULES_MAP 25
#define VAL_MAX_DEV_GRANULES_MAP 64

#define VAL_HOST_MAX_REALMS 10

#define REALM_FLAG_PMU_ENABLE (1UL << 2)


typedef enum {
    REALM_STATE_NULL,
    REALM_STATE_NEW,
    REALM_STATE_ACTIVE,
    REALM_STATE_SYSTEM_OFF
} val_host_realm_state_te;

typedef enum {
    GRANULE_UNDELEGATED,
    GRANULE_DELEGATED,
    GRANULE_RD,
    GRANULE_RTT,
    GRANULE_RTT_AUX,
    GRANULE_DATA,
    GRANULE_REC,
    GRANULE_UNPROTECTED
} val_host_memory_state_te;

typedef struct {
    uint64_t rtt_addr;
    uint64_t ipa;
} val_host_rtt_db_ts;

typedef struct {
    uint64_t ipa;
    uint64_t size;
    uint64_t level;
    uint64_t pa;
} val_host_granules_mapped_ts;

typedef struct val_host_realm {
    /* Test Input start */
    uint64_t flags;
    uint64_t flags1;
    uint64_t max_num_vdevs;
    uint8_t s2sz;
    uint8_t sve_vl;
    uint8_t num_bps;
    uint8_t num_wps;
    uint8_t pmu_num_ctrs;
    uint8_t hash_algo;
    uint8_t rpv[64];
    uint16_t vmid;
    uint64_t s2_starting_level;
    uint32_t num_s2_sl_rtts;
    uint64_t rec_count;
    uint8_t num_aux_planes;
    uint64_t mecid;

    /* Test Input end */
    uint64_t image_pa_base;
    uint64_t aux_image_pa_base[VAL_MAX_AUX_PLANES];
    uint64_t image_pa_size;
    uint64_t rd;
    uint64_t rtt_l0_addr;
    uint64_t rtt_aux_l0_addr[VAL_MAX_AUX_PLANES];
    uint64_t granules_mapped_count;
    val_host_granules_mapped_ts granules[VAL_MAX_GRANULES_MAP];
    uint64_t dev_granules_mapped_count;
    val_host_granules_mapped_ts dev_granules[VAL_MAX_DEV_GRANULES_MAP];
    uint64_t rec[VAL_MAX_REC_COUNT];
    uint64_t run[VAL_MAX_REC_COUNT];
    uint64_t aux_count;
    uint64_t rec_aux_granules[VAL_MAX_REC_AUX_GRANULES * VAL_MAX_REC_COUNT];
    val_host_realm_state_te state;
} val_host_realm_ts;

typedef struct val_host_granule_ts {
    uint64_t rd;
    uint32_t state;
    uint64_t PA;
    uint64_t ipa;
    uint64_t level;
    uint64_t rtt_tree_idx;
    uint8_t  is_granule_sliced;
    uint8_t has_auxiliary[VAL_MAX_AUX_PLANES];
    struct val_host_granule_ts *next;
} val_host_granule_ts;

typedef struct {
    uint64_t src_pa;
    uint64_t target_pa;
    uint64_t ipa;
    uint64_t rtt_alignment;
    uint64_t size;
} val_data_create_ts;

typedef val_host_granule_ts NS_LL;
typedef val_host_granule_ts RD_LL;
typedef val_host_granule_ts RTT_LL;
typedef val_host_granule_ts RTT_AUX_LL;
typedef val_host_granule_ts REC_LL;
typedef val_host_granule_ts DATA_LL;
typedef val_host_granule_ts VALID_NS_LL;

typedef struct {
    NS_LL *ns;
    RD_LL *rd;
    RTT_LL *rtt;
    RTT_AUX_LL *rtt_aux;
    REC_LL *rec;
    DATA_LL *data;
    VALID_NS_LL *valid_ns;
} val_host_granule_type_ts;

typedef struct mem_track {
    uint64_t rd;
    val_host_granule_type_ts gran_type;
} val_host_memory_track_ts;

extern val_host_memory_track_ts mem_track[];

uint32_t val_host_map_protected_data(val_host_realm_ts *realm,
                uint64_t target_pa,
                uint64_t ipa,
                uint64_t map_size,
                uint64_t src_pa);
uint32_t val_host_map_protected_data_unknown(val_host_realm_ts *realm,
                        uint64_t target_pa,
                        uint64_t ipa,
                        uint64_t map_size);
uint32_t val_host_map_unprotected(val_host_realm_ts *realm,
                        uint64_t ns_pa,
                        uint64_t ipa,
                        uint64_t map_size,
                        uint64_t rtt_alignment);
uint32_t val_host_map_unprotected_attr(val_host_realm_ts *realm,
                        uint64_t ns_pa,
                        uint64_t ipa,
                        uint64_t map_size,
                        uint64_t rtt_alignment,
                        uint64_t mem_attr);

uint32_t val_host_rmm_activate(void);
uint32_t val_host_realm_create(val_host_realm_ts *realm);
uint32_t val_host_realm_rtt_map(val_host_realm_ts *realm);
uint32_t val_host_rec_create(val_host_realm_ts *realm);
uint32_t val_host_realm_activate(val_host_realm_ts *realm);
uint32_t val_host_realm_destroy(uint64_t rd);
uint32_t val_host_realm_setup(val_host_realm_ts *realm, bool activate);
uint32_t val_host_check_realm_exit_host_call(val_host_rec_run_ts *run);
uint32_t val_host_check_realm_exit_rsi_host_call(val_host_rec_run_ts *run);
uint32_t val_host_check_realm_exit_ripas_change(val_host_rec_run_ts *run);
uint32_t val_host_check_realm_exit_psci(val_host_rec_run_ts *run, uint32_t psci_fid);
void val_host_add_granule(uint32_t state, uint64_t PA, val_host_granule_ts *node);
val_host_granule_ts *val_host_find_granule(uint64_t PA);
void val_host_update_granule_state(uint64_t rd,
                        uint32_t state,
                        uint64_t PA,
                        uint64_t ipa,
                        uint64_t level,
                        uint64_t rtt_tree_idx);
uint64_t val_host_postamble(void);
val_host_granule_ts *val_host_remove_granule(val_host_granule_ts **current, uint64_t PA);
val_host_granule_ts *val_host_remove_data_granule(val_host_granule_ts **current, uint64_t ipa);
val_host_granule_ts *val_host_remove_rtt_granule(val_host_granule_ts **gran_list_head,
                                                        uint64_t ipa, uint64_t level);
val_host_granule_ts *val_host_remove_aux_rtt_granule(val_host_granule_ts **gran_list_head,
                                                   uint64_t ipa, uint64_t level, uint64_t index);
int val_host_get_curr_realm(uint64_t rd);
void val_host_update_destroy_granule_state(uint64_t rd,
                        uint64_t PA,
                        uint64_t ipa,
                        uint64_t level,
                        uint32_t state,
                        uint32_t gran_list_state,
                        uint64_t rtt_tree_idx);
uint64_t val_host_destroy_rtt_levels(uint64_t level, int current_realm);
uint32_t val_host_map_ns_shared_region(val_host_realm_ts *realm, uint64_t size, uint64_t mem_attr);
int val_host_ripas_init(val_host_realm_ts *realm, uint64_t base,
                uint64_t top, uint64_t rtt_level, uint64_t rtt_alignment);
uint32_t val_host_create_rtt_levels(val_host_realm_ts *realm,
                   uint64_t ipa,
                   uint64_t level,
                   uint64_t max_level,
                   uint64_t rtt_alignment);

uint32_t val_host_map_protected_data_to_realm(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create);

void val_host_realm_params(val_host_realm_ts *realm);
void val_host_reset_mem_tack(void);
uint64_t val_host_update_aux_rtt_info(uint64_t gran_state, uint64_t rd, uint64_t rtt_index,
                                                                        uint64_t ipa, bool val);
uint32_t val_host_aux_map_unprotected(val_host_realm_ts *realm,
                        uint64_t ipa,
                        uint64_t rtt_map_size,
                        uint64_t rtt_alignment,
                        uint64_t index);
uint32_t val_host_aux_map_protected_data(val_host_realm_ts *realm, uint64_t ipai, uint64_t index);
uint32_t val_host_create_aux_rtt_levels(val_host_realm_ts *realm,
                   uint64_t ipa,
                   uint64_t rtt_level,
                   uint64_t rtt_max_level,
                   uint64_t rtt_alignmenti,
                   uint64_t index);
uint64_t val_host_destroy_aux_rtt_levels(uint64_t rtt_level, int current_realm, uint64_t index);
uint64_t val_host_rtt_level_mapsize(uint64_t rtt_level);
bool val_host_rmm_supports_planes(void);
bool val_host_rmm_supports_rtt_tree_single(void);
bool val_host_rmm_supports_rtt_tree_per_plane(void);
uint32_t val_host_map_protected_dev_mem(val_host_realm_ts *realm,
                uint64_t target_pa,
                uint64_t ipa,
                uint64_t rtt_map_size,
                uint64_t ripas_flag);
uint32_t val_host_dev_mem_map(val_host_realm_ts *realm, uint64_t vdev, uint64_t dev_mem_base,
                                                  uint64_t dev_mem_top, uint64_t dev_mem_pa);
bool val_host_rmm_supports_mec(void);

uint64_t val_host_check_pdev_state(uint64_t pdev, uint8_t pdev_state);
bool val_host_rmm_supports_da(void);

#endif /* _VAL_HOST_REALM_H_ */
