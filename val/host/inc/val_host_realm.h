/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#define VAL_REC_EXIT_GPRS                   31
#define RPV_SIZE                            64
#define REC_CREATE_NR_GPRS                   8

#define VAL_MAX_REC_AUX_GRANULES 16
#define VAL_MAX_RTT_GRANULES 25
#define VAL_MAX_GRANULES_MAP 25

#define VAL_HOST_MAX_REALMS 10
#define SET_MEMBER_RMI    SET_MEMBER

#define REALM_FLAG_PMU_ENABLE (1UL << 2)

#define VAL_MAX_AUX_PLANES 3

#define MAX_PDEV_AUX_GRANULES    16
#define MAX_PDEV_ADDR            16

#define VAL_REALM_FLAG_RTT_TREE_PP      1ULL

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

typedef struct {
    /* Test Input start */
    uint64_t flags;
    uint64_t flags1;
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

    /* Test Input end */
    uint64_t image_pa_base;
    uint64_t aux_image_pa_base[VAL_MAX_AUX_PLANES];
    uint64_t image_pa_size;
    uint64_t rd;
    uint64_t rtt_l0_addr;
    uint64_t rtt_aux_l0_addr[VAL_MAX_AUX_PLANES];
    uint64_t granules_mapped_count;
    val_host_granules_mapped_ts granules[VAL_MAX_GRANULES_MAP];
    uint64_t rec[VAL_MAX_REC_COUNT];
    uint64_t run[VAL_MAX_REC_COUNT];
    uint64_t aux_count;
    uint64_t rec_aux_granules[VAL_MAX_REC_AUX_GRANULES * VAL_MAX_REC_COUNT];
    val_host_realm_state_te state;
} val_host_realm_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);                /* Offset 0 */
    /* Requested IPA width */
    SET_MEMBER_RMI(unsigned int s2sz, 0x8, 0x10);               /* 0x8 */
    /* Requested SVE vector length */
    SET_MEMBER_RMI(unsigned int sve_vl, 0x10, 0x18);            /* 0x10 */
    /* Requested number of breakpoints */
    SET_MEMBER_RMI(unsigned int num_bps, 0x18, 0x20);           /* 0x18 */
    /* Requested number of watchpoints */
    SET_MEMBER_RMI(unsigned int num_wps, 0x20, 0x28);           /* 0x20 */
    /* Requested number of PMU counters */
    SET_MEMBER_RMI(unsigned int pmu_num_ctrs, 0x28, 0x30);      /* 0x28 */
    /* Measurement algorithm */
    SET_MEMBER_RMI(unsigned char hash_algo, 0x30, 0x38);        /* 0x30 */
    /* Number of Auxillay planes */
    SET_MEMBER_RMI(unsigned char num_aux_planes, 0x38, 0x400);  /* 0x38 */
    /* Realm Personalization Value */
    SET_MEMBER_RMI(unsigned char rpv[RPV_SIZE], 0x400, 0x800);  /* 0x400 */
    SET_MEMBER_RMI(struct {
            /* Virtual Machine Identifier */
            unsigned short vmid;                                /* 0x800 */
            /* Realm Translation Table base */
            unsigned long rtt_base;                             /* 0x808 */
            /* RTT starting level */
            unsigned long rtt_level_start;                      /* 0x810 */
            /* Number of starting level RTTs */
            unsigned int rtt_num_start;		                    /* 0x818 */
            /* Flags 1 */
            unsigned long flags1;                                /* 0x820 */
            }, 0x800, 0xF00);
    SET_MEMBER_RMI(
    /* Virtual Machine Identifier of the auxiliary contexts.
     * The host must provide @num_aux_planes entries */
            unsigned short aux_vmid[VAL_MAX_AUX_PLANES];         /* 0xF00 */
            , 0xF00, 0xF80);
    SET_MEMBER_RMI(
    /* Base addresses of root level tables for auxiliary RTT trees.
     * If @rtt_tree_pp is true, the host must provide
     * @num_aux_planes entries */
            unsigned long aux_rtt_base[VAL_MAX_AUX_PLANES];     /* 0xF80 */
       , 0xF80, 0x1000);
} val_host_realm_params_ts;

typedef struct __attribute__((packed)) {
    uint8_t lpa2:1;
    uint8_t sve:1;
    uint8_t pmu:1;
    uint8_t da:1;
    uint64_t unused:59;
} val_host_realm_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t rtt_tree_pp:1;
    uint64_t unused:63;
} val_host_realm_flags1_ts;

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
    uint8_t da:1;
    uint8_t pdev_num_aux:4;
    uint64_t unused:29;
} val_host_rmifeatureregister0_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x100);                        /* Offset 0 */
    /* MPIDR of the REC */
    SET_MEMBER_RMI(unsigned long mpidr, 0x100, 0x200);                    /* 0x100 */
    /* Program counter */
    SET_MEMBER_RMI(unsigned long pc, 0x200, 0x300);                       /* 0x200 */
    /* General-purpose registers */
    SET_MEMBER_RMI(unsigned long gprs[REC_CREATE_NR_GPRS], 0x300, 0x800); /* 0x300 */
    SET_MEMBER_RMI(struct {
            /* Number of auxiliary Granules */
            unsigned long num_aux;                                        /* 0x800 */
            /* Addresses of auxiliary Granules */
            unsigned long aux[VAL_MAX_REC_AUX_GRANULES];                  /* 0x808 */
            }, 0x800, 0x1000);
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
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x200);                       /* Offset 0 */
    /* General-purpose registers */
    SET_MEMBER_RMI(unsigned long gprs[VAL_REC_EXIT_GPRS], 0x200, 0x300); /* 0x200 */
    SET_MEMBER_RMI(struct {
            /* GICv3 Hypervisor Control Register */
            unsigned long gicv3_hcr;                                     /* 0x300 */
            /* GICv3 List Registers */
            unsigned long gicv3_lrs[VAL_REC_GIC_NUM_LRS];                /* 0x308 */
            }, 0x300, 0x800);
} val_host_rec_enter_ts;

typedef struct __attribute__((packed)) {
    uint8_t emul_mmio:1;
    uint8_t inject_sea:1;
    uint8_t trap_wfi:1;
    uint8_t trap_wfe:1;
    uint8_t ripas_response:1;
    uint8_t s2ap_response:1;
    uint64_t unused:58;
} val_host_rec_enter_flags_ts;

/*
 * Structure contains data passed from the RMM to the Host on REC exit
 */
typedef struct {
    SET_MEMBER_RMI(struct {
            /* Exit reason */
            unsigned long exit_reason;          /* Offset 0x0 */
            /* REC exit flags */
            unsigned long flags;                      /* 0x8 */
            }, 0, 0x100);
    SET_MEMBER_RMI(struct {
            /* Exception Syndrome Register */
            unsigned long esr;                      /* 0x100 */
            /* Fault Address Register */
            unsigned long far;                      /* 0x108 */
            /* Hypervisor IPA Fault Address register */
            unsigned long hpfar;                    /* 0x110 */
            /* Index of RTT tree at the time of REC exit */
            unsigned long rtt_tree;                /* 0x118 */
            /* Level of requested RTT */
            unsigned long rtt_level;                /* 0x120 */
            }, 0x100, 0x200);
    /* General-purpose registers */
    SET_MEMBER_RMI(unsigned long gprs[VAL_REC_EXIT_GPRS], 0x200, 0x300); /* 0x200 */
    SET_MEMBER_RMI(struct {
            /* GICv3 Hypervisor Control Register */
            unsigned long gicv3_hcr;     /* 0x300 */
            /* GICv3 List Registers */
            unsigned long gicv3_lrs[VAL_REC_GIC_NUM_LRS]; /* 0x308 */
            /* GICv3 Maintenance Interrupt State Register */
            unsigned long gicv3_misr;    /* 0x388 */
            /* GICv3 Virtual Machine Control Register */
            unsigned long gicv3_vmcr;    /* 0x390 */
            }, 0x300, 0x400);
    SET_MEMBER_RMI(struct {
            /* Counter-timer Physical Timer Control Register */
            unsigned long cntp_ctl;        /* 0x400 */
            /* Counter-timer Physical Timer CompareValue Register */
            unsigned long cntp_cval;       /* 0x408 */
            /* Counter-timer Virtual Timer Control Register */
            unsigned long cntv_ctl;        /* 0x410 */
            /* Counter-timer Virtual Timer CompareValue Register */
            unsigned long cntv_cval;       /* 0x418 */
            }, 0x400, 0x500);
    SET_MEMBER_RMI(struct {
            /* Base address of pending RIPAS change */
            unsigned long ripas_base;     /* 0x500 */
            /* Top address of pending RIPAS change */
            unsigned long ripas_top;      /* 0x508 */
            /* RIPAS value of pending RIPAS change */
            unsigned long ripas_value;    /* 0x510 */
            /* Reserved */
            unsigned long rsvd;           /* 0x518 */
            /* Base address of pending S2AP change */
            unsigned long s2ap_base;      /* 0x520 */
            /* Top address of pending S2AP change */
            unsigned long s2ap_top;      /* 0x528 */
            }, 0x500, 0x600);
    SET_MEMBER_RMI(struct {
            /* Host call immediate value */
            unsigned long  imm;          /* 0x600 */
            /* Plane index if Host call from Pn*/
            unsigned long plane;         /* 0x608 */
            }, 0x600, 0x700);
    /* PMU overflow status */
    SET_MEMBER_RMI(unsigned long pmu_ovf_status, 0x700, 0x800);    /* 0x700 */
} val_host_rec_exit_ts;

typedef struct {
    /* Entry information */
    SET_MEMBER_RMI(val_host_rec_enter_ts enter, 0, 0x800);    /* Offset 0 */
    /* Exit information */
    SET_MEMBER_RMI(val_host_rec_exit_ts exit, 0x800, 0x1000);/* 0x800 */
} val_host_rec_run_ts;

typedef struct __attribute__((packed)) {
    uint8_t measure:1;
    uint64_t unused:63;
} val_host_rmi_data_flags_ts;

typedef struct {
    uint64_t base;
    uint64_t top;
} val_host_addr_range_ts;

typedef struct __attribute__((packed)) {
    uint8_t share:1;
    uint64_t unused:63;
} val_host_rmi_io_create_flags_ts;

/* Defined as per the prototype branch, Need to update once prototype branch migrated */
typedef struct {
    uint64_t status;
    uint64_t req_addr;
    uint64_t req_len;
    uint64_t resp_addr;
    uint64_t resp_len;
} val_host_rmi_io_enter_ts;

/* Defined as per the prototype branch, Need to update once prototype branch migrated */
typedef struct {
    uint64_t flags;
    uint64_t cache_offset;
    uint64_t cache_len;
    uint64_t req_len;
    uint64_t timeout;
} val_host_rmi_io_exit_ts;

typedef struct __attribute__((packed)) {
    uint8_t cache:1;
    uint8_t send:1;
    uint8_t wait:1;
    uint8_t multi:1;
    uint64_t unused:60;
} val_host_rmi_io_exit_flags_ts;

typedef struct {
    /* Entry information */
    SET_MEMBER_RMI(val_host_rmi_io_enter_ts enter, 0x0, 0x800);
    /* Exit information */
    SET_MEMBER_RMI(val_host_rmi_io_exit_ts exit, 0x800, 0x1000);
} val_host_rmi_io_data_ts;

typedef struct __attribute__((packed)) {
    uint8_t share:1;
    uint64_t unused:63;
} val_host_rmi_io_delegate_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t ripas_io_shared:1;
    uint64_t unused:63;
} val_host_rmi_rec_exit_flags_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* device class */
    SET_MEMBER_RMI(unsigned long cls, 0x8, 0x10);
    /* Device id */
    SET_MEMBER_RMI(unsigned long pdev_id, 0x10, 0x18);
    /* Segment identifier */
    SET_MEMBER_RMI(unsigned long segment_id, 0x18, 0x20);
    /* Root port id */
    SET_MEMBER_RMI(unsigned long root_id, 0x20, 0x28);
    /* Certificate slot id */
    SET_MEMBER_RMI(unsigned int cert_id, 0x28, 0x30);
    /* STREAM id */
    SET_MEMBER_RMI(unsigned int ide_sid, 0x30, 0x38);

    /* IDE RID range association */
    SET_MEMBER_RMI(unsigned long rid_base, 0x38, 0x40);
    SET_MEMBER_RMI(unsigned long rid_top, 0x40, 0x100);

    /* IDE address range association */
    SET_MEMBER_RMI(unsigned int num_addr_range, 0x100, 0x108);
    SET_MEMBER_RMI(val_host_addr_range_ts addr[MAX_PDEV_ADDR], 0x108, 0x300);

    /* Auxiliary granules for pdev */
    SET_MEMBER_RMI(unsigned int num_aux, 0x300, 0x308);
    SET_MEMBER_RMI(unsigned long aux[MAX_PDEV_AUX_GRANULES], 0x308, 0x400);
} val_host_pdev_params_ts;

typedef struct {
    uint64_t flags;
    uint64_t vdev_id;
    uint64_t tdi_id;
} val_host_vdev_params_ts;

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
#endif /* _VAL_HOST_REALM_H_ */

