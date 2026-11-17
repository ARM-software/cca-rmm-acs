/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_RMI_TYPES_H_
#define _VAL_HOST_RMI_TYPES_H_

#include "val.h"

/* RmiFeature type */
#define RMI_FEATURE_FALSE     0
#define RMI_FEATURE_TRUE      1

#define RMI_FEATURE_REGISTER_0_INDEX 0
#define RMI_FEATURE_REGISTER_1_INDEX 1
#define RMI_FEATURE_REGISTER_1_L0GPTSZ_START 10
#define RMI_FEATURE_REGISTER_1_L0GPTSZ_END 13
#define RMI_FEATURE_REGISTER_2_INDEX 2
#define RMI_FEATURE_REGISTER_2_DA    0
#define RMI_FEATURE_REGISTER_2_ATS   3
#define RMI_FEATURE_REGISTER_2_MAX_VDEVS_ORDER_START 4
#define RMI_FEATURE_REGISTER_2_MAX_VDEVS_ORDER_END   7
#define RMI_FEATURE_REGISTER_2_REALM_MAX_VDEVS_ORDER_START 10
#define RMI_FEATURE_REGISTER_2_REALM_MAX_VDEVS_ORDER_END   14
#define RMI_FEATURE_REGISTER_2_NON_TEE_STREAM         9
#define RMI_FEATURE_REGISTER_2_P2P                  10

#define RMI_L0GPTSZ_BASE_SHIFT 30

/* RmiAddrBlockSize type */
#define RMI_PAGE_L3           0
#define RMI_BLOCK_L2          1
#define RMI_BLOCK_L1          2
#define RMI_BLOCK_L0          3

/* RmiContinueBeyond type */
#define RMI_CONTINUE_KEEP_GOING  0
#define RMI_CONTINUE_STOP        1

/* RmiPlaneRttFeature type*/
#define RMI_PLANE_RTT_AUX              0
#define RMI_PLANE_RTT_AUX_SINGLE       1
#define RMI_PLANE_RTT_SINGLE           2

/* RmiInterfaceVersion type */
#define RMI_MAJOR_VERSION    0
#define RMI_MINOR_VERSION    0

#define RMI_VERSION_GET_MAJOR(ver) (((ver) >> 16) & 0xFFFF)
#define RMI_VERSION_GET_MINOR(ver) ((ver) & 0xFFFF)

/* RmiRealmMeasurementAlgorithm types */
#define RMI_SHA256  0

/* RmiHashAlgorithm type */
#define RMI_HASH_SHA_256    0
#define RMI_HASH_SHA_512    1
#define RMI_HASH_SHA_384    2

/* RmiRecEmulatedMmio types */
#define RMI_NOT_EMULATED_MMIO 0
#define RMI_EMULATED_MMIO     1

/* RmiPmuOverflowStatus types */
#define RMI_PMU_OVERFLOW_NOT_ACTIVE  0
#define RMI_PMU_OVERFLOW_ACTIVE      1

/* RmiRecExitReason type */
#define RMI_EXIT_SYNC               0
#define RMI_EXIT_IRQ                1
#define RMI_EXIT_FIQ                2
#define RMI_EXIT_PSCI               3
#define RMI_EXIT_RIPAS_CHANGE       4
#define RMI_EXIT_HOST_CALL          5
#define RMI_EXIT_SERROR             6
#define RMI_EXIT_S2AP_CHANGE        7
#define RMI_EXIT_VDEV_VALIDATE_MAPPING  8
#define RMI_EXIT_VSMMU_COMMAND          10
#define RMI_EXIT_VDEV_P2P_BINDING       11
#define RMI_EXIT_VDEV_MAP               9

#define RMI_VALIDN_NS 1

/* RmiRecRunnable types */
#define RMI_NOT_RUNNABLE 0
#define RMI_RUNNABLE     1

/* RmiRipas types */
#define RMI_EMPTY        0
#define RMI_RAM          1
#define RMI_DESTROYED    2
#define RMI_DEV          3
#define RMI_RIPAS_EMPTY      0
#define RMI_RIPAS_RAM        1
#define RMI_RIPAS_DESTROYED  2
#define RMI_RIPAS_DEV        3

/* RmiRttEntryState types */
#define RMI_UNASSIGNED          0
#define RMI_ASSIGNED            1
#define RMI_TABLE               2
#define RMI_ASSIGNED_DEV        3
#define RMI_AUX_DESTROYED       4
#define RMI_RTTE_VOID           0
#define RMI_RTTE_DATA           1
#define RMI_RTTE_TABLE          2
#define RMI_RTTE_NARCH_DEV      3
#define RMI_RTTE_AUX_DESTROYED  4

/* RmiStatusCode types */
#define RMI_SUCCESS                0
#define RMI_ERROR_INPUT            1
#define RMI_ERROR_REALM            2
#define RMI_ERROR_REC              3
#define RMI_ERROR_RTT              4
#define RMI_ERROR_NOT_SUPPORTED    5
#define RMI_ERROR_DEVICE           6
#define RMI_ERROR_RTT_AUX          7
#define RMI_ERROR_PSMMU_ST         8
#define RMI_ERROR_DPT              9
#define RMI_BUSY                  10
#define RMI_ERROR_GLOBAL          11
#define RMI_ERROR_TRACKING        12
#define RMI_INCOMPLETE            13
#define RMI_BLOCKED               14
#define RMI_ERROR_GPT             15
#define RMI_ERROR_GRANULE         16

/* RmiDataMeasureContent types */
#define RMI_NO_MEASURE_CONTENT      0
#define RMI_MEASURE_CONTENT         1

/* RmiInjectSea types */
#define RMI_NO_INJECT_SEA           0
#define RMI_INJECT_SEA              1

/* RmiTrap types */
#define RMI_NO_TRAP                 0
#define RMI_TRAP                    1

/* RmiResponse types */
#define RMI_ACCEPT                  0
#define RMI_REJECT                  1
#define RMI_RESPONSE_ACCEPT         0
#define RMI_RESPONSE_REJECT         1

/* RmiUnprotectedS2AP type */
#define RMI_UNPROTECTED_S2AP_NO_ACCESS  0
#define RMI_UNPROTECTED_S2AP_RO         1
#define RMI_UNPROTECTED_S2AP_WO         2
#define RMI_UNPROTECTED_S2AP_RW         3

/* RmiBoolean type */
#define RMI_FALSE                   0
#define RMI_TRUE                    1

/* RmiPdevState type */
#define RMI_PDEV_NEW                0
#define RMI_PDEV_NEEDS_KEY          1
#define RMI_PDEV_HAS_KEY            2
#define RMI_PDEV_READY              3
#define RMI_PDEV_STOPPED            4
#define RMI_PDEV_ERROR              5

/* RmiSignatureAlgorithm type */
#define RMI_SIG_RSASSA_3072         0
#define RMI_SIG_ECDSA_P256          1
#define RMI_SIG_ECDSA_P384          2

/* RmiVdevState type */
#define RMI_VDEV_NEW                0
#define RMI_VDEV_UNLOCKED           1
#define RMI_VDEV_LOCKED             2
#define RMI_VDEV_STARTED            3
#define RMI_VDEV_ERROR              4
#define RMI_VDEV_KEY_REFRESH        5
#define RMI_VDEV_KEY_PURGE          6

/*
 * RmiDevCommExitFlags
 * Fieldset contains flags provided by the RMM during a device transaction.
 * Width: 64 bits
 */
#define RMI_DEV_COMM_EXIT_FLAGS_REQ_CACHE_BIT        (UL(1) << 0U)
#define RMI_DEV_COMM_EXIT_FLAGS_RSP_CACHE_BIT        (UL(1) << 1U)
#define RMI_DEV_COMM_EXIT_FLAGS_REQ_SEND_BIT        (UL(1) << 2U)
#define RMI_DEV_COMM_EXIT_FLAGS_RSP_WAIT_BIT        (UL(1) << 3U)
#define RMI_DEV_COMM_EXIT_FLAGS_RSP_RESET_BIT        (UL(1) << 4U)
#define RMI_DEV_COMM_EXIT_FLAGS_MULTI_BIT        (UL(1) << 5U)
#define RMI_DEV_COMM_EXIT_FLAGS_STREAM_WAIT_BIT       (UL(1) << 6U)

/* RmiDevCommObject type */
#define RMI_DEV_VCA                 0
#define RMI_DEV_CERTIFICATE         1
#define RMI_DEV_MEASUREMENTS        2
#define RMI_DEV_INTERFACE_REPORT    3
#define RMI_DEV_PROTOCOL_DATA       0
#define RMI_DEV_IDENTITY            1

/* RmiDevCommProtocol type */
#define RMI_PROTOCOL_SPDM           0
#define RMI_PROTOCOL_SECURE_SPDM    1

/* RmiDevCommStatus type */
#define RMI_DEV_COMM_NONE           0
#define RMI_DEV_COMM_RESPONSE       1
#define RMI_DEV_COMM_ERROR          2

/* RmiPdevSpdm type */
#define RMI_SPDM_FALSE              0
#define RMI_SPDM_TRUE               1

/* RmiPdevIde type */
#define RMI_IDE_FALSE               0
#define RMI_IDE_TRUE                1

/* RmiPdevCoherent type */
#define RMI_NCOH                   0
#define RMI_COH                    1

/* RmiPdevCategory type */
#define RMI_PDEV_SMEM               0
#define RMI_PDEV_CMEM_CXL           1
#define RMI_PDEV_ROOT_PORT          0
#define RMI_PDEV_ENDPOINT_ACCEL_OFF_CHIP  1
#define RMI_PDEV_ENDPOINT_ACCEL_ON_CHIP   2

/* RmiPdevTrust type */
#define RMI_TRUST_SEL               0
#define RMI_TRUST_COMP              1

/* RmiProgress type */
#define RMI_PROGRESS_COMPLETE       0
#define RMI_PROGRESS_INCOMPLETE     1

/* RmiVdevMeasureCacheType type */
#define RMI_VDEV_CACHE_BLOCKS       0
#define RMI_VDEV_CACHE_EXCHANGE     1

/* RmiVdevMeasureRaw type */
#define RMI_VDEV_MEASURE_NOT_RAW       0
#define RMI_VDEV_MEASURE_RAW           1

/* RmiVdevMeasureSigned type */
#define RMI_VDEV_MEASURE_NOT_SIGNED       0
#define RMI_VDEV_MEASURE_SIGNED           1


/* RmiForceP0 type */
#define RMI_NO_FORCE_P0             0
#define RMI_FORCE_P0                1

/* RmiGranuleSize type */
#define RMI_GRANULE_SIZE_4KB        0
#define RMI_GRANULE_SIZE_16KB       1
#define RMI_GRANULE_SIZE_64KB       2

/* RmiIrqCfg type */
#define RMI_IRQ_DISABLED            0
#define RMI_IRQ_WIRED               1
#define RMI_IRQ_MSI                 2

/* RmiLfaPolicy type */
#define RMI_LFA_DISALLOW            0
#define RMI_LFA_ALLOW               1

/* RmiMemCategory type */
#define RMI_MEM_CATEGORY_CONVENTIONAL  0
#define RMI_MEM_CATEGORY_DEV_NCOH      1
#define RMI_MEM_CATEGORY_DEV_COH       2
#define RMI_MEM_CATEGORY_NONE          3

/* RmiOpCanCancel type */
#define RMI_OP_CANNOT_CANCEL        0
#define RMI_OP_CAN_CANCEL           1

/* RmiOpMemContig type */
#define RMI_OP_MEM_NON_CONTIG       0
#define RMI_OP_MEM_CONTIG           1

/* RmiOpMemReq type */
#define RMI_OP_MEM_REQ_NONE         0
#define RMI_OP_MEM_REQ_DONATE       1
#define RMI_OP_MEM_REQ_RECLAIM      2

/* RmiOpMemState type */
#define RMI_OP_MEM_DELEGATED        0
#define RMI_OP_MEM_UNDELEGATED      1
#define RMI_OP_MEM_CONDITIONAL      2

/* RmiPdevStreamType type */
#define RMI_PDEV_STREAM_NON_TEE     0
#define RMI_PDEV_STREAM_NCOH        1
#define RMI_PDEV_STREAM_COH         2
#define RMI_PDEV_STREAM_NCOH_SYS    3
#define RMI_PDEV_STREAM_COH_SYS     4

/* RmiPsmmuIrq type */
#define RMI_PSMMU_IRQ_GERROR        0
#define RMI_PSMMU_IRQ_EVENTQ        1
#define RMI_PSMMU_IRQ_PRIQ          2
#define RMI_PSMMU_IRQ_CMDQ          3

/* RmiPsmmuIrqEvent type */
#define RMI_PSMMU_IRQ_EVENT_NONE    0
#define RMI_PSMMU_IRQ_EVENT_ERROR   1
#define RMI_PSMMU_IRQ_EVENT_PSMMU   2

/* RmiRmmState type */
#define RMI_RMM_STATE_INIT          0
#define RMI_RMM_STATE_ACTIVE        1

/* RmiRttAddrType type */
#define RMI_ADDR_TYPE_NONE          0
#define RMI_ADDR_TYPE_SINGLE        1
#define RMI_ADDR_TYPE_LIST          2

/* RmiTrackingRegionState type */
#define RMI_TRACKING_RESERVED       0
#define RMI_TRACKING_NONE           1
#define RMI_TRACKING_FINE           2
#define RMI_TRACKING_COARSE         3
#define RMI_TRACKING_INTERMEDIATE   4

#define RMI_TRACKING_LEVEL_COARSE       0U
#define RMI_TRACKING_LEVEL_INTERMEDIATE 1U

/* Get the status field from the return code */
#define RMI_STATUS(ret)       ((ret) & 0xFF)
/* Get the status-specific index or level field from the return code */
#define RMI_INDEX(ret)        (((ret) >> 8) & 0xFF)
/* RMI_ERROR_TRACKING uses bits [15:8] for the tracking level. */
#define RMI_TRACKING_LEVEL(ret) (((ret) >> 8) & 0xFF)


#define VAL_REC_NUM_GPRS                      8
#define VAL_REC_HVC_NUM_GPRS                 31
#define VAL_REC_GIC_NUM_LRS                  16
#define VAL_REC_EXIT_GPRS                   31
#define RPV_SIZE                            64
#define REC_CREATE_NR_GPRS                   8
#define VAL_MAX_REC_AUX_GRANULES 16
#define SET_MEMBER_RMI    SET_MEMBER
#define VAL_MAX_AUX_PLANES 3
#define MAX_PDEV_AUX_GRANULES    32
#define MAX_PDEV_ADDR            16
#define MAX_PUB_KEY_LEN            1024
#define MAX_METADATA_LEN            1024
#define VAL_REALM_FLAG_RTT_TREE_PP      1ULL
#define MAX_PDEV_AUX_GRANULES        32
#define MAX_PDEV_NCOH_ADDR_RANGE     16
#define MAX_PDEV_COH_ADDR_RANGE       4
#define MAX_PUB_KEY_LEN            1024
#define MAX_METADATA_LEN            1024
#define MAX_VDEV_AUX_GRANULES        32
#define CACHE_NONE                0
#define CACHE_CERTIFICATE         1
#define CACHE_MEASUREMENTS        2
#define CACHE_INTERFACE_REPORT    3
#define REC_EXIT_NR_GPRS        U(31)
#define VDEV_MEAS_PARAM_NONCE_LEN        U(32)
/*
 * Maximum number of aux granules paramenter passed in rmi_pdev_params during
 * PDEV create to PDEV create
 */
#define PDEV_PARAM_AUX_GRANULES_MAX        U(32)
#define VDEV_PARAM_AUX_GRANULES_MAX        U(32)
/*
 * Maximum number of device non-coherent RmiAddressRange parameter passed in
 * rmi_pdev_params during PDEV create
 */
#define PDEV_PARAM_NCOH_ADDR_RANGE_MAX        U(16)
/*
 * Maximum number of device coherent RmiAddressRange parameter passed in
 * rmi_pdev_params during PDEV create
 */
#define PDEV_PARAM_COH_ADDR_RANGE_MAX        U(4)
#define GRANULE_SIZE            PAGE_SIZE_4KB

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags0, 0, 0x8);               /* Offset 0 */
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
    SET_MEMBER_RMI(unsigned char rpv[RPV_SIZE], 0x400, 0x440);  /* 0x400 */
    /*
     * If ATS is enabled, determines the stage 2 translation used by devices
     * assigned to the Realm.
     */
    SET_MEMBER_RMI(unsigned long ats_plane, 0x440, 0x808);
    /* Realm Translation Table base */
    SET_MEMBER_RMI(unsigned long rtt_base, 0x808, 0x810);
    /* RTT starting level */
    SET_MEMBER_RMI(long rtt_level_start, 0x810, 0x818);
    /* Number of starting level RTTs */
    SET_MEMBER_RMI(unsigned int rtt_num_start, 0x818, 0x820);
    /* Flags 1 */
    SET_MEMBER_RMI(unsigned long flags1, 0x820, 0x828);
    SET_MEMBER_RMI(unsigned long max_num_vdevs, 0x828, 0xF80);
    SET_MEMBER_RMI(
    /* Base addresses of root level tables for auxiliary RTT trees.
     * If @rtt_tree_pp is true, the host must provide
     * @num_aux_planes entries */
            unsigned long aux_rtt_base[VAL_MAX_AUX_PLANES];    /* 0xF80 */
       , 0xF80, 0x1000);
} val_host_realm_params_ts;

typedef struct __attribute__((packed)) {
    uint64_t lpa2:1;
    uint64_t sve:1;
    uint64_t pmu:1;
    uint64_t da:1;
    uint64_t reserved0:1;
    uint64_t lfa_policy:2;
    uint64_t mec_policy:2;
    uint64_t reserved1:55;
} val_host_realm_flags_ts;

typedef struct __attribute__((packed)) {
    uint64_t rtt_tree_pp:1;
    uint64_t rtt_s2ap_encoding:1;
    uint64_t ats:1;
    uint64_t reserved:61;
} val_host_realm_flags1_ts;

typedef struct __attribute__((packed)) {
    uint8_t s2sz;
    uint8_t lpa2:1;
    uint8_t sve_en:1;
    uint8_t sve_vl:4;
    uint8_t num_bps:6;
    uint8_t num_wps:6;
    uint8_t pmu_en:1;
    uint8_t pmu_num_ctrs:5;
    uint8_t hash_sha_256:1;
    uint8_t hash_sha_512:1;
    uint8_t gicv3_num_lrs:4;
    uint8_t max_recs_order:4;
    uint8_t da:1;
    uint8_t rtt_plane:2;
    uint8_t max_num_aux_planes:4;
    uint8_t rtt_s2ap_indirect:1;
    uint64_t unused:14;
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
    uint8_t dev_mem_response:1;
    uint64_t unused:57;
} val_host_rec_enter_flags_ts;

/*
 * Structure contains data passed from the RMM to the Host on REC exit
 */
typedef struct {
    /* Exit reason */
    SET_MEMBER_RMI(unsigned long exit_reason, 0, 0x100);/* Offset 0 */
    SET_MEMBER_RMI(struct {
            /* Exception Syndrome Register */
            unsigned long esr;        /* 0x100 */
            /* Fault Address Register */
            unsigned long far;        /* 0x108 */
            /* Hypervisor IPA Fault Address register */
            unsigned long hpfar;        /* 0x110 */
            /* Index of RTT tree active at time of exit */
            unsigned long rtt_tree;        /* 0x118 */
            /* Level of requested RTT */
            long rtt_level;            /* 0x120 */
           }, 0x100, 0x200);
    /* General-purpose registers */
    SET_MEMBER_RMI(unsigned long gprs[REC_EXIT_NR_GPRS], 0x200, 0x300); /* 0x200 */
    SET_MEMBER_RMI(struct {
            /* GICv3 Hypervisor Control Register */
            unsigned long gicv3_hcr;    /* 0x300 */
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
            unsigned long cntp_cval;    /* 0x408 */
            /* Counter-timer Virtual Timer Control Register */
            unsigned long cntv_ctl;        /* 0x410 */
            /* Counter-timer Virtual Timer CompareValue Register */
            unsigned long cntv_cval;    /* 0x418 */
           }, 0x400, 0x500);
    SET_MEMBER_RMI(struct {
            /* Base address of pending RIPAS change */
            unsigned long ripas_base;    /* 0x500 */
            /* Size of pending RIPAS change */
            unsigned long ripas_top;    /* 0x508 */
            /* RIPAS value of pending RIPAS change */
            unsigned char ripas_value;    /* 0x510 */
           }, 0x500, 0x520);
    SET_MEMBER_RMI(struct {
            /* Base addr of target region for pending S2AP change */
            unsigned long s2ap_base; /* 0x520 */
            /* Top addr of target region for pending S2AP change */
            unsigned long s2ap_top; /* 0x528 */
            /* Virtual device ID 1 */
            unsigned long vdev_id_1; /* 0x530 */
            /* Virtual device ID 2 */
            unsigned long vdev_id_2; /* 0x538 */
            /* Base IPA of target region for VDEV mapping validation */
            unsigned long dev_mem_base; /* 0x540 */
            /* Top IPA of target region for VDEV mapping validation */
            unsigned long dev_mem_top; /* 0x548 */
            /* Base PA of device memory region */
            unsigned long dev_mem_pa; /* 0x550 */
           }, 0x520, 0x600);

    /* Host call immediate value */
    SET_MEMBER_RMI(unsigned int imm, 0x600, 0x608);
    /* UInt64: Plane Index */
    SET_MEMBER_RMI(unsigned long plane, 0x608, 0x700);
    /* PMU overflow status */
    SET_MEMBER_RMI(unsigned long pmu_ovf_status, 0x700, 0x800);
} val_host_rec_exit_ts;

typedef struct {
    /* Entry information */
    SET_MEMBER_RMI(val_host_rec_enter_ts enter, 0, 0x800);    /* Offset 0 */
    /* Exit information */
    SET_MEMBER_RMI(val_host_rec_exit_ts exit, 0x800, 0x1000);/* 0x800 */
} val_host_rec_run_ts;

typedef struct __attribute__((packed)) {
    uint8_t beyond:1;
    uint64_t unused:63;
} val_host_rmi_continue_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t oaddr_type:2;
    uint16_t list_count:14;
    uint64_t unused:48;
} val_host_rmi_rtt_prot_map_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t oaddr_type:2;
    uint16_t list_count:14;
    uint64_t unused:48;
} val_host_rmi_rtt_unmap_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t oaddr_type:2;
    uint16_t list_count:14;
    uint8_t memattr:3;
    uint8_t s2ap:4;
    uint64_t unused:41;
} val_host_rmi_rtt_unprot_map_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t measure:1;
    uint64_t unused:63;
} val_host_rmi_data_flags_ts;


typedef struct __attribute__((packed)) {
    uint64_t spdm:1;
    uint64_t category:2;
    uint64_t unused:61;
} val_host_pdev_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t vsmmu:1;
    uint64_t unused:63;
} val_host_vdev_flags_ts;

typedef struct __attribute__((packed)) {
    uint64_t raw:1;
    uint64_t unused:63;
} val_host_vdev_measure_flags_ts;

typedef struct {
    /* RmiVdevMeasureFlags: Properties of device measurements */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x100);
    /* Bits256: Nonce value used in measurement requests */
    SET_MEMBER_RMI(unsigned char nonce[VDEV_MEAS_PARAM_NONCE_LEN], 0x100, 0x1000);
} val_host_vdev_measure_params_ts;
typedef struct {
    uint64_t base;
    uint64_t top;
} val_host_addr_range_ts;

typedef struct __attribute__((packed)) {
    uint8_t size:2;
    uint16_t count:10;
    uint64_t addr:40;
    uint16_t unused:11;
    uint8_t state:1;
} val_host_rmi_addr_range_desc_4kb_ts;

typedef struct __attribute__((packed)) {
    uint8_t size:2;
    uint16_t count:12;
    uint64_t addr:38;
    uint16_t unused:11;
    uint8_t state:1;
} val_host_rmi_addr_range_desc_16kb_ts;

typedef struct __attribute__((packed)) {
    uint8_t size:2;
    uint16_t count:14;
    uint64_t addr:36;
    uint16_t unused:11;
    uint8_t state:1;
} val_host_rmi_addr_range_desc_64kb_ts;

typedef struct __attribute__((packed)) {
    uint8_t size:2;
    uint16_t count:14;
    uint8_t contig:1;
    uint8_t state:1;
    uint64_t unused:46;
} val_host_rmi_op_mem_donate_req_ts;

typedef struct __attribute__((packed)) {
    uint8_t mem:2;
    uint8_t cancel:1;
    uint64_t unused:53;
} val_host_rmi_result_data_incomplete_ts;

typedef struct __attribute__((packed)) {
    uint8_t level;
    uint64_t unused:48;
} val_host_rmi_result_data_level_ts;

typedef struct __attribute__((packed)) {
    uint64_t unused:56;
} val_host_rmi_result_data_null_ts;

typedef struct __attribute__((packed)) {
    uint8_t status;
    uint64_t data:56;
} val_host_rmi_result_ts;

typedef struct {
    /* Tracking region size */
    SET_MEMBER_RMI(unsigned char tracking_region_size, 0, 0x8);
    /* RMI Granule size */
    SET_MEMBER_RMI(unsigned char rmi_granule_size, 0x8, 0x1000);
} val_host_rmi_rmm_config_ts;

typedef struct __attribute__((packed)) {
    uint8_t irq_cfg:2;
    uint8_t cmdq_sync_irq_wired:1;
    uint8_t ats:1;
    uint8_t pri:1;
    uint64_t unused:59;
} val_host_rmi_psmmu_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t irq_cfg:2;
    uint8_t cmdq_sync_irq_wired:1;
    uint64_t unused:61;
} val_host_rmi_psmmu_info_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t pending:1;
    uint8_t irq_event:2;
    uint64_t unused:61;
} val_host_rmi_psmmu_irq_result_ts;

typedef struct __attribute__((packed)) {
    uint8_t gerror:1;
    uint8_t eventq:1;
    uint8_t priq:1;
    uint8_t cmdq_sync:1;
    uint64_t unused:60;
} val_host_rmi_psmmu_irq_set_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* GERROR interrupt number */
    SET_MEMBER_RMI(unsigned int gerror_intr_num, 0x8, 0x10);
    /* EventQ interrupt number */
    SET_MEMBER_RMI(unsigned int eventq_intr_num, 0x10, 0x18);
    /* PRIQ interrupt number */
    SET_MEMBER_RMI(unsigned int priq_intr_num, 0x18, 0x20);
    /* CMD sync interrupt number */
    SET_MEMBER_RMI(unsigned int cmdq_sync_intr_num, 0x20, 0x1000);
} val_host_rmi_psmmu_info_ts;

typedef val_host_rmi_psmmu_info_ts val_host_psmmu_info_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* Physical MSI address of the GERROR interrupt */
    SET_MEMBER_RMI(unsigned long gerr_addr, 0x8, 0x10);
    /* Physical MSI data of the GERROR interrupt */
    SET_MEMBER_RMI(unsigned long gerr_data, 0x10, 0x18);
    /* Physical MSI address of the EVENTQ interrupt */
    SET_MEMBER_RMI(unsigned long eventq_addr, 0x18, 0x20);
    /* Physical MSI data of the EVENTQ interrupt */
    SET_MEMBER_RMI(unsigned long eventq_data, 0x20, 0x28);
    /* Physical MSI address of the PRIQ interrupt */
    SET_MEMBER_RMI(unsigned long priq_addr, 0x28, 0x30);
    /* Physical MSI data of the PRIQ interrupt */
    SET_MEMBER_RMI(unsigned long priq_data, 0x30, 0x1000);
} val_host_rmi_psmmu_params_ts;

typedef val_host_rmi_psmmu_params_ts val_host_psmmu_params_ts;

typedef struct __attribute__((packed)) {
    uint64_t unused;
} val_host_rmi_pdev_stream_flags_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* Stream type */
    SET_MEMBER_RMI(unsigned char stream_type, 0x8, 0x10);
    /* Address of first PDEV */
    SET_MEMBER_RMI(unsigned long pdev_1, 0x10, 0x18);
    /* Address of second PDEV */
    SET_MEMBER_RMI(unsigned long pdev_2, 0x18, 0x20);
    /* IDE stream ID */
    SET_MEMBER_RMI(unsigned long ide_sid, 0x20, 0x28);
    /* Number of device address ranges */
    SET_MEMBER_RMI(unsigned long num_addr_range, 0x28, 0x100);
    /* CPU physical address range used to access the device */
    SET_MEMBER_RMI(val_host_addr_range_ts addr_range[MAX_PDEV_ADDR], 0x100, 0x1000);
} val_host_rmi_pdev_stream_params_ts;

/*
 * RmiDevCommEnter structure contains data passed
 * from the Host to the RMM during device communication.
 */
typedef struct {
    /* Status of device transaction */
    SET_MEMBER(unsigned long status, 0x0, 0x8);
    /* Address of request buffer */
    SET_MEMBER(unsigned long req_addr, 0x8, 0x10);
    /* Address of response buffer */
    SET_MEMBER(unsigned long rsp_addr, 0x10, 0x18);
    /* Amount of valid data in response buffer in bytes */
    SET_MEMBER(unsigned long rsp_len, 0x18, 0x100);
} val_host_dev_comm_enter_ts;

/*
 * RmiDevCommExit structure contains data passed from
 * RMM to the Host during device communication
 */
typedef struct {
    /*
     * RmiDevCommExitFlags: Flags indicating the actions the host is
     * requested to perform
     */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /*
     * UInt64: If flags.req_cache is true, offset in the device request
     * buffer to the start of data to be cached, in bytes
     */
    SET_MEMBER_RMI(unsigned long req_cache_offset, 0x8, 0x10);
    /*
     * UInt64: If flags.req_cache is true, amount of device request data to
     * be cached, in bytes
     */
    SET_MEMBER_RMI(unsigned long req_cache_len, 0x10, 0x18);
    /*
     * UInt64: If flags.rsp_cache is true, offset in the device response
     * buffer to the start of data to be cached, in bytes
     */
    SET_MEMBER_RMI(unsigned long rsp_cache_offset, 0x18, 0x20);
    /*
     * UInt64: If flags.rsp_cache is true, amount of device response data to
     * be cached, in bytes.
     */
    SET_MEMBER_RMI(unsigned long rsp_cache_len, 0x20, 0x28);
    /*
     * RmiDevCommObject: If flags.req_cache is true and / or flags.rsp_cache
     * is true, identifier for the object to be cached
     */
    SET_MEMBER_RMI(unsigned char cache_object_id, 0x28, 0x30);
    /* RmiDevCommProtocol: If flags.req_send is true, protocol to use */
    SET_MEMBER_RMI(unsigned char protocol, 0x30, 0x38);
    /*
     * UInt64: If flags.req_send is true, amount of time to wait before
     * sending the request, in microseconds.
     */
    SET_MEMBER_RMI(unsigned long req_delay, 0x38, 0x40);
    /*
     * UInt64: If flags.send is true, amount of valid data in request buffer
     * in bytes
     */
    SET_MEMBER_RMI(unsigned long req_len, 0x40, 0x48);
    /*
     * UInt64: Amount of time to wait (measured from the
     * most recent exit which had flags.rsp_reset = true)
     * for device response in microseconds.
     */
    SET_MEMBER_RMI(unsigned long rsp_timeout, 0x48, 0x100);
} val_host_dev_comm_exit_ts;


typedef struct __attribute__((packed)) {
    uint8_t req_cache:1;
    uint8_t rsp_cache:1;
    uint8_t req_send:1;
    uint8_t rsp_wait:1;
    uint8_t rsp_reset:1;
    uint8_t multi:1;
    uint8_t stream_wait:1;
    uint64_t unused:58;
} val_host_dev_comm_exit_flags_ts;

typedef struct {
    /* Entry information */
    SET_MEMBER_RMI(val_host_dev_comm_enter_ts enter, 0x0, 0x800);
    /* Exit information */
    SET_MEMBER_RMI(val_host_dev_comm_exit_ts exit, 0x800, 0x1000);
} val_host_dev_comm_data_ts;


typedef struct __attribute__((packed)) {
    uint8_t ripas_io_shared:1;
    uint64_t unused:63;
} val_host_rmi_rec_exit_flags_ts;

typedef struct {
    /* Base of address range (inclusive) */
    SET_MEMBER_RMI(unsigned long base, 0, 0x8);
    /* Top of address range (exclusive) */
    SET_MEMBER_RMI(unsigned long top, 0x8, 0x10);
} val_host_address_range_ts;

/* Defined as per the RMM v1.1 APLHA7 */
typedef struct {
    /* RmiPdevFlags: Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* Bits64: Physical device identifier */
    SET_MEMBER_RMI(unsigned long pdev_id, 0x8, 0x10);
    /* Bits8: Segment ID */
    SET_MEMBER_RMI(unsigned char segment_id, 0x10, 0x18);
    /* Address: ECAM base address of the PCIe configuration space */
    SET_MEMBER_RMI(unsigned long ecam_addr, 0x18, 0x20);
    /* Bits16: Root Port identifier */
    SET_MEMBER_RMI(unsigned short root_id, 0x20, 0x28);
    /* UInt64: Certificate identifier */
    SET_MEMBER_RMI(unsigned long cert_id, 0x28, 0x30);
    /* UInt16: Base requester ID range (inclusive) */
    SET_MEMBER_RMI(unsigned short rid_base, 0x30, 0x38);
    /* UInt16: Top of requester ID range (exclusive) */
    SET_MEMBER_RMI(unsigned short rid_top, 0x38, 0x40);
    /* RmiHashAlgorithm: Algorithm used to generate device digests */
    SET_MEMBER_RMI(unsigned char hash_algo, 0x40, 0x48);
    /* UInt64: Number of auxiliary granules */
    SET_MEMBER_RMI(unsigned long num_aux, 0x48, 0x50);
    /* UInt64: IDE stream identifier */
    SET_MEMBER_RMI(unsigned long ncoh_ide_sid, 0x50, 0x58);
    /* UInt64: Number of device non-coherent address ranges */
    SET_MEMBER_RMI(unsigned long ncoh_num_addr_range, 0x58, 0x60);
    /* UInt64: Number of device coherent address ranges */
    SET_MEMBER_RMI(unsigned long coh_num_addr_range, 0x60, 0x100);
    /* Address: Addresses of auxiliary granules */
    SET_MEMBER_RMI(unsigned long aux[PDEV_PARAM_AUX_GRANULES_MAX], 0x100,
               0x200);
    /* RmiAddressRange: Device non-coherent address range */
    SET_MEMBER_RMI(val_host_address_range_ts
               ncoh_addr_range[PDEV_PARAM_NCOH_ADDR_RANGE_MAX],
               0x200, 0x300);
    /* RmiAddressRange: Device coherent address range */
    SET_MEMBER_RMI(val_host_address_range_ts
               coh_addr_range[PDEV_PARAM_COH_ADDR_RANGE_MAX],
               0x300, 0x1000);

} val_host_pdev_params_ts;

/* Defined as per the RMM v2.0 specification */
typedef struct {
    /* RmiPdevFlags: Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* Bits64: Host bridge base */
    SET_MEMBER_RMI(unsigned long hb_base, 0x8, 0x10);
    /* Bits64: Physical device identifier */
    SET_MEMBER_RMI(unsigned long pdev_id, 0x10, 0x18);
    /* Bits64: Routing identifier */
    SET_MEMBER_RMI(unsigned long routing_id, 0x18, 0x20);
    /* UInt64: Device identity index */
    SET_MEMBER_RMI(unsigned long id_index, 0x20, 0x28);
    /* UInt32: Base requester ID range (inclusive) */
    SET_MEMBER_RMI(unsigned int rid_base, 0x28, 0x30);
    /* UInt32: Top requester ID range (exclusive) */
    SET_MEMBER_RMI(unsigned int rid_top, 0x30, 0x38);
    /* RmiHashAlgorithm: Algorithm used to generate device digests */
    SET_MEMBER_RMI(unsigned char hash_algo, 0x38, 0x1000);
} val_host_rmi_pdev_params_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* Virtual device identifier */
    SET_MEMBER_RMI(unsigned long vdev_id, 0x8, 0x10);
    /* TDI identifier */
    SET_MEMBER_RMI(unsigned long tdi_id, 0x10, 0x18);
    /* Number of auxiliary Granules */
    SET_MEMBER_RMI(unsigned long num_aux, 0x18, 0x100);
    /* Addresses of auxiliary Granules */
    SET_MEMBER_RMI(unsigned long aux[MAX_VDEV_AUX_GRANULES], 0x100, 0x1000);
} val_host_vdev_params_ts;

/* Defined as per the RMM v2.0 specification */
typedef struct {
    /* RmiVdevFlags: Flags */
    SET_MEMBER_RMI(unsigned long flags, 0, 0x8);
    /* Bits64: Virtual device identifier */
    SET_MEMBER_RMI(unsigned long vdev_id, 0x8, 0x10);
    /* Bits64: TDI identifier */
    SET_MEMBER_RMI(unsigned long tdi_id, 0x10, 0x20);
    /* Address: PA of VSMMU */
    SET_MEMBER_RMI(unsigned long vsmmu_addr, 0x20, 0x28);
    /* Bits64: Virtual Stream Identifier */
    SET_MEMBER_RMI(unsigned long vsid, 0x28, 0x30);
    /* UInt64: Number of device address ranges */
    SET_MEMBER_RMI(unsigned long num_addr_range, 0x30, 0x200);
    /* RmiAddrRange: Device address ranges */
    SET_MEMBER_RMI(val_host_address_range_ts addr_range[8], 0x200, 0x1000);
} val_host_rmi_vdev_params_ts;

typedef struct {
    /* Address of key data */
    SET_MEMBER_RMI(unsigned long key_addr, 0x0, 0x400);
    /* Address of key metadata */
    SET_MEMBER_RMI(unsigned long metadata_addr, 0x400, 0x800);
    /* Length of key data in bytes */
    SET_MEMBER_RMI(unsigned long key_len, 0x800, 0x808);
    /* Length of metadata in bytes */
    SET_MEMBER_RMI(unsigned long metadata_len, 0x808, 0x1000);
} val_host_public_key_params_ts;

#endif /* _VAL_HOST_RMI_TYPES_H_ */
