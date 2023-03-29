/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#define VAL_REC_NUM_GPRS                      8
#define VAL_REC_HVC_NUM_GPRS                 31
#define VAL_REC_GIC_NUM_LRS                  16

#define VAL_MAX_REC_AUX_GRANULES 16
#define VAL_MAX_RTT_GRANULES 25
#define VAL_MAX_GRANULES_MAP 25

#define VAL_HOST_MAX_REALMS 10

/*
 * Defines member of structure and reserves space
 * for the next member with specified offset.
 */
#define SET_MEMBER(member, start, end)  \
    union {             \
        member;         \
        unsigned char reserved##end[end - start]; \
    }

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

typedef struct {
    /* Test Input start */
    uint64_t realm_feat_0;
    uint8_t hash_algo;
    uint8_t rpv[64];
    uint16_t vmid;
    uint64_t s2_starting_level;
    uint32_t num_s2_sl_rtts;
    uint64_t rec_count;

    /* Test Input end */
    uint64_t image_pa_base;
    uint64_t image_pa_size;
    uint64_t rd;
    uint64_t rtt_l0_addr;
    uint64_t rtt_l1_count;
    val_host_rtt_db_ts rtt_l1[VAL_MAX_RTT_GRANULES];
    uint64_t rtt_l2_count;
    val_host_rtt_db_ts rtt_l2[VAL_MAX_RTT_GRANULES];
    uint64_t rtt_l3_count;
    val_host_rtt_db_ts rtt_l3[VAL_MAX_RTT_GRANULES];
    uint64_t granules_mapped_count;
    val_host_granules_mapped_ts granules[VAL_MAX_GRANULES_MAP];
    uint64_t rec[VAL_MAX_REC_COUNT];
    uint64_t run[VAL_MAX_REC_COUNT];
    uint64_t aux_count;
    uint64_t rec_aux_granules[VAL_MAX_REC_AUX_GRANULES * VAL_MAX_REC_COUNT];
    val_host_realm_state_te state;
} val_host_realm_ts;

typedef struct __attribute__((packed)) {
    uint64_t realm_feat_0; /* Realm feature register 0 */
    uint8_t unused[248];
    uint8_t hash_algo; /* Hash algorithm */
    uint8_t unused1[767];
    uint8_t rpv[64];
    uint8_t unused2[960];
    uint16_t vmid;
    uint8_t unused3[6];
    uint64_t rtt_addr; /* RTT Base Granule address */
    uint64_t s2_starting_level; /* stage 2 starting level */
    uint32_t num_s2_sl_rtts; /* number of concatenated RTTs */
    uint8_t unused4[2020];
} val_host_realm_params_ts;

typedef struct __attribute__((packed)) {
    uint8_t s2sz;
    uint8_t lpa2:1;
    uint8_t sve_en:1;
    uint8_t sve_vl:4;
    uint8_t num_bps:4;
    uint8_t num_wps:4;
    uint8_t pmu_en:1;
    uint8_t pmu_num_ctrs:5;
    uint8_t hash_sha_256:1;
    uint8_t hash_sha_512:1;
    uint64_t unused:34;
} val_host_rmifeatureregister0_ts;

typedef struct {
  uint64_t flags;
  uint8_t unused[248];
  uint64_t mpidr;
  uint8_t unused1[248];
  uint64_t pc;
  uint8_t unused2[248];
  uint64_t gprs[VAL_REC_NUM_GPRS];
  uint8_t unused3[1216];
  uint64_t num_rec_aux;
  uint64_t rec_aux_granules[VAL_MAX_REC_AUX_GRANULES];
  uint8_t unused4[1912];
} val_host_rec_params_ts;

typedef struct __attribute__((packed)) {
    uint8_t runnable:1;
    uint64_t unused:63;
} val_host_rec_create_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t aff0:4;
    uint8_t unused:4;
    uint8_t aff1;
    uint8_t aff2;
    uint8_t aff3;
    uint64_t unused1:32;
} val_host_rec_mpidr_ts;

typedef struct {
    uint64_t flags;
    uint8_t unused[504];
    uint64_t gprs[VAL_REC_HVC_NUM_GPRS];
    uint8_t unused1;
    uint64_t gicv3_hcr;
    uint64_t gicv3_lrs[VAL_REC_GIC_NUM_LRS];
    uint8_t unused2[1144];
    // TODO: allow Host to inject SEA in response to fault at
    // Unprotected IPA
} val_host_rec_entry_ts;

typedef struct __attribute__((packed)) {
    uint8_t emul_mmio:1;
    uint8_t inject_sea:1;
    uint8_t trap_wfi:1;
    uint8_t trap_wfe:1;
    uint64_t unused:60;
} val_host_rec_entry_flags_ts;

/*
 * Structure contains data passed from the RMM to the Host on REC exit
 */
typedef struct {
    /* Exit reason */
    SET_MEMBER(unsigned long exit_reason, 0, 0x100);/* Offset 0 */
    SET_MEMBER(struct {
            /* Exception Syndrome Register */
            unsigned long esr;      /* 0x100 */
            /* Fault Address Register */
            unsigned long far;      /* 0x108 */
            /* Hypervisor IPA Fault Address register */
            unsigned long hpfar;        /* 0x110 */
           }, 0x100, 0x200);
    /* General-purpose registers */
    SET_MEMBER(unsigned long gprs[VAL_REC_HVC_NUM_GPRS], 0x200, 0x300); /* 0x200 */
    SET_MEMBER(struct {
            /* GICv3 Hypervisor Control Register */
            unsigned long gicv3_hcr;    /* 0x300 */
            /* GICv3 List Registers */
            unsigned long gicv3_lrs[VAL_REC_GIC_NUM_LRS]; /* 0x308 */
            /* GICv3 Maintenance Interrupt State Register */
            unsigned long gicv3_misr;   /* 0x388 */
            /* GICv3 Virtual Machine Control Register */
            unsigned long gicv3_vmcr;   /* 0x390 */
           }, 0x300, 0x400);
    SET_MEMBER(struct {
            /* Counter-timer Physical Timer Control Register */
            unsigned long cntp_ctl;     /* 0x400 */
            /* Counter-timer Physical Timer CompareValue Register */
            unsigned long cntp_cval;    /* 0x408 */
            /* Counter-timer Virtual Timer Control Register */
            unsigned long cntv_ctl;     /* 0x410 */
            /* Counter-timer Virtual Timer CompareValue Register */
            unsigned long cntv_cval;    /* 0x418 */
           }, 0x400, 0x500);
    SET_MEMBER(struct {
            /* Base address of pending RIPAS change */
            unsigned long base;   /* 0x500 */
            /* Size of pending RIPAS change */
            unsigned long size;   /* 0x508 */
            /* RIPAS value of pending RIPAS change */
            unsigned char ripas_value;  /* 0x510 */
           }, 0x500, 0x600);
    /* Host call immediate value */
    SET_MEMBER(unsigned int imm, 0x600, 0x800); /* 0x600 */
} val_host_rec_exit_ts;

typedef struct {
    val_host_rec_entry_ts entry;
    /* TBD: Need to place exit at 2KB offset */
    val_host_rec_exit_ts exit;
} val_host_rec_run_ts;

typedef struct __attribute__((packed)) {
    uint8_t measure:1;
    uint64_t unused:63;
} val_host_rmi_data_flags_ts;

typedef struct val_host_granule_ts {
    uint64_t rd;
    uint32_t state;
    uint64_t PA;
    uint64_t ipa;
    uint64_t level;
    uint8_t  is_granule_sliced;
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
typedef val_host_granule_ts REC_LL;
typedef val_host_granule_ts DATA_LL;
typedef val_host_granule_ts VALID_NS_LL;

typedef struct {
    NS_LL *ns;
    RD_LL *rd;
    RTT_LL *rtt;
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

uint32_t val_host_realm_create(val_host_realm_ts *realm);
uint32_t val_host_realm_rtt_map(val_host_realm_ts *realm);
uint32_t val_host_rec_create(val_host_realm_ts *realm);
uint32_t val_host_realm_activate(val_host_realm_ts *realm);
uint32_t val_host_realm_destroy(uint64_t rd);
uint32_t val_host_realm_setup(val_host_realm_ts *realm, bool activate);
uint32_t val_host_check_realm_exit_host_call(val_host_rec_run_ts *run);
uint32_t val_host_check_realm_exit_ripas_change(val_host_rec_run_ts *run);
uint32_t val_host_check_realm_exit_psci(val_host_rec_run_ts *run, uint32_t psci_fid);
void val_host_add_granule(uint32_t state, uint64_t PA, val_host_granule_ts *node);
val_host_granule_ts *val_host_find_granule(uint64_t PA);
void val_host_update_granule_state(uint64_t rd,
                        uint32_t state,
                        uint64_t PA,
                        uint64_t ipa,
                        uint64_t level);
uint64_t val_host_postamble(void);
val_host_granule_ts *val_host_remove_granule(val_host_granule_ts **current, uint64_t PA);
val_host_granule_ts *val_host_remove_data_granule(val_host_granule_ts **current, uint64_t ipa);
int val_host_get_curr_realm(uint64_t rd);
void val_host_update_destroy_granule_state(uint64_t rd,
                        uint64_t PA,
                        uint32_t state,
                        uint32_t gran_list_state);
uint64_t val_host_destroy_rtt_levels(uint64_t level, int current_realm);
uint32_t val_host_map_ns_shared_region(val_host_realm_ts *realm, uint64_t size, uint64_t mem_attr);
int val_host_ripas_init(val_host_realm_ts *realm, uint64_t ipa,\
                        uint32_t level, uint64_t rtt_alignment);
uint32_t val_host_create_rtt_levels(val_host_realm_ts *realm,
                   uint64_t ipa,
                   uint32_t level,
                   uint32_t max_level,
                   uint64_t rtt_alignment);

uint32_t val_host_map_protected_data_to_realm(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create);
uint64_t val_get_pa_range_supported(void);

#endif /* _VAL_HOST_REALM_H_ */

