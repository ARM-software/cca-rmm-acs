/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#ifndef __PAL_COMMON_SUPPORT_H_
#define __PAL_COMMON_SUPPORT_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef uintptr_t addr_t;
typedef char     char8_t;

#define MEM_ALIGN_4K       0x1000
#define MEM_ALIGN_8K       0x2000
#define MEM_ALIGN_16K      0x4000
#define MEM_ALIGN_32K      0x8000
#define MEM_ALIGN_64K      0x10000

#define PCIE_EXTRACT_BDF_SEG(bdf)    ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)    ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)    ((bdf >> 3) & 0x1F)
#define PCIE_EXTRACT_BDF_FUNC(bdf)    (bdf & 0x7)

#define PCIE_CFG_SIZE  4096

#define PCIE_MAX_BUS   256
#define PCIE_MAX_DEV    32
#define PCIE_MAX_FUNC    8

#define PCIE_CREATE_BDF(Seg, Bus, Dev, Func) ((Seg << 16) | (Bus << 8) | (Dev << 3) | Func)

#define PCIE_SUCCESS            0x00000000  /* Operation completed successfully */
#define PCIE_NO_MAPPING         0x10000001  /* A mapping to a Function does not exist */
#define PCIE_CAP_NOT_FOUND      0x10000010  /* The specified capability was not found */
#define PCIE_UNKNOWN_RESPONSE   0xFFFFFFFF  /* Function not found or UR response from completer */

/* PCI/PCIe express extended capability structure's
   next capability pointer mask and cap ID mask */
#define PCIE_NXT_CAP_PTR_MASK 0x0FFF
#define PCIE_CAP_ID_MASK      0xFFFF
#define PCI_CAP_ID_MASK       0x00FF
#define PCI_NXT_CAP_PTR_MASK  0x00FF
#define CAP_PTR_MASK          0x00FF
#define PCIE_CAP_OFFSET       0x100
#define CAP_PTR_OFFSET        0x34

#define PCI_CAP_PTR_OFFSET    8
#define PCIE_CAP_PTR_OFFSET   20

/* TYPE 0/1 Cmn Cfg reg offsets and mask*/
#define TYPE01_CPR           0x34
#define TYPE01_CPR_MASK      0xff
#define COMMAND_REG_OFFSET   0x04
#define REG_ACC_DATA         0x7
#define SERR_ENABLE          0x10

#define BAR_MASK        0xFFFFFFF0

/* Class Code Masks */
#define CC_SUB_MASK     0xFF   /* Sub Class */
#define CC_BASE_MASK    0xFF   /* Base Class */

/* Class Code Shifts */
#define CC_SHIFT        8
#define CC_SUB_SHIFT    16
#define CC_BASE_SHIFT   24

#define HB_BASE_CLASS   0x06
#define HB_SUB_CLASS    0x00

/* Device Type Shift and mask*/
#define PCIE_DEVICE_TYPE_SHIFT  20
#define PCIE_DEVICE_TYPE_MASK   0xf
#define PCI_EXP_DEVCTL          8
#define DEVCTL_SNOOP_BIT        11

/* Bus Number reg shifts */
#define SECBN_SHIFT 8
#define SUBBN_SHIFT 16

/* Bus Number reg masks */
#define SECBN_MASK  0xff
#define SUBBN_MASK  0xff

/* Capability header reg shifts */
#define PCIE_CIDR_SHIFT      0
#define PCIE_NCPR_SHIFT      8
#define PCIE_ECAP_CIDR_SHIFT 0
#define PCIE_ECAP_NCPR_SHIFT 20

/* Capability header reg masks */
#define PCIE_CIDR_MASK       0xff
#define PCIE_NCPR_MASK       0xff
#define PCIE_ECAP_CIDR_MASK  0xffff
#define PCIE_ECAP_NCPR_MASK  0xfff

#define PCIE_ECAP_START      0x100

/* Capability Structure IDs */
#define CID_PCIECS           0x10
#define CID_MSI              0x05
#define CID_MSIX             0x11

/* PCI Express capability struct offsets */
#define CIDR_OFFSET    0x0
#define PCIECR_OFFSET  0x2
#define DCAPR_OFFSET   0x4
#define DCTLR_OFFSET   0x8
#define DCAP2R_OFFSET  0x24
#define DCTL2R_OFFSET  0x28

/* PCIe capabilities reg shifts and masks */
#define PCIECR_DPT_SHIFT 4
#define PCIECR_DPT_MASK  0xf

#define PASID_OFFSET         0x04
#define PASID_NUM_SHIFT      8
#define PASID_NUM_MASK       0x1f
#define PER_FLAG_MSI_ENABLED 0x2

/* Device bitmask definitions */
#define RCiEP    (1 << 0b1001)    /* Root Complex Integrated Endpoint */
#define RCEC     (1 << 0b1010)    /* Root Complex Event Collector */
#define EP       (1 << 0b0000)    /* Endpoint */
#define RP       (1 << 0b0100)    /* Root Port */
#define UP       (1 << 0b0101)
#define DPort       (1 << 0b0110)
#define iEP_EP   (1 << 0b1100)
#define iEP_RP   (1 << 0b1011)

#define CLEAN_AND_INVALIDATE  0x1
#define CLEAN                 0x2
#define INVALIDATE            0x3

#define NOT_IMPLEMENTED       0x4B1D

#define LEGACY_PCI_IRQ_CNT 4  /* Legacy PCI IRQ A, B, C. and D */
#define MAX_IRQ_CNT 0xFFFF    /* This value is arbitrary and may have to be adjusted */

typedef struct {
    uint32_t  irq_list[MAX_IRQ_CNT];
    uint32_t  irq_count;
} PERIFERAL_IRQ_LIST;

typedef struct {
    PERIFERAL_IRQ_LIST  legacy_irq_map[LEGACY_PCI_IRQ_CNT];
} PERIPHERAL_IRQ_MAP;

typedef struct __attribute__((packed)) {
    uint64_t   ecam_base;     /* ECAM Base address */
    uint32_t   segment_num;   /* Segment number of this ECAM */
    uint32_t   start_bus_num; /* Start Bus number for this ecam space */
    uint32_t   end_bus_num;   /* Last Bus number */
} PCIE_INFO_BLOCK;

typedef struct __attribute__((packed)) {
    uint32_t  num_entries;
    PCIE_INFO_BLOCK block[];
} PCIE_INFO_TABLE;

typedef struct {
    uint64_t   class_code;
    uint32_t   device_id;
    uint32_t   vendor_id;
    uint32_t   bus;
    uint32_t   dev;
    uint32_t   func;
    uint32_t   seg;
} PCIE_READ_BLOCK;

typedef struct {
    uint32_t num_entries;
    PCIE_READ_BLOCK device[];
} PCIE_READ_TABLE;

typedef enum {
    NON_PREFETCH_MEMORY = 0x0,
    PREFETCH_MEMORY = 0x1
} PCIE_MEM_TYPE_INFO_e;

typedef struct __attribute__((packed)) {
    uint32_t bdf;
    uint32_t rp_bdf;
} pcie_device_attr;

typedef struct __attribute__((packed)) {
    uint32_t num_entries;
    pcie_device_attr device[];    /* in the format of Segment/Bus/Dev/Func */
} pcie_device_bdf_table;


typedef struct {
    uint32_t    num_usb;   /* Number of USB  Controllers */
    uint32_t    num_sata;  /* Number of SATA Controllers */
    uint32_t    num_uart;  /* Number of UART Controllers */
    uint32_t    num_all;   /* Number of all PCI Controllers */
} PERIPHERAL_INFO_HDR;

typedef enum {
    PERIPHERAL_TYPE_USB = 0x2000,
    PERIPHERAL_TYPE_SATA,
    PERIPHERAL_TYPE_UART,
    PERIPHERAL_TYPE_OTHER,
    PERIPHERAL_TYPE_NONE
} PER_INFO_TYPE_e;

/**
  @brief  Instance of peripheral info
**/
typedef struct {
    PER_INFO_TYPE_e  type;  /* PER_INFO_TYPE */
    uint32_t         bdf;   /* Bus Device Function */
    uint64_t         base0; /* Base Address of the controller */
    uint64_t         base1; /* Base Address of the controller */
    uint32_t         irq;   /* IRQ to install an ISR */
    uint32_t         flags;
    uint32_t         msi;   /* MSI Enabled */
    uint32_t         msix;  /* MSIX Enabled */
    uint32_t         max_pasids;
} PERIPHERAL_INFO_BLOCK;

/**
  @brief Peripheral Info Structure
**/
typedef struct {
    PERIPHERAL_INFO_HDR     header;
    PERIPHERAL_INFO_BLOCK   info[]; /* Array of Information blocks - instantiated
                                       for each peripheral */
} PERIPHERAL_INFO_TABLE;

typedef struct {
    uint64_t  Address;
    uint8_t   AddressSpaceId;
    uint8_t   RegisterBitWidth;
    uint8_t   RegisterBitOffset;
    uint8_t   AccessSize;
} PLATFORM_OVERRIDE_GENERIC_ADDRESS_STRUCTURE;

typedef struct {
    uint64_t                                     Address;
    PLATFORM_OVERRIDE_GENERIC_ADDRESS_STRUCTURE  BaseAddress;
    uint32_t                                     GlobalSystemInterrupt;
    uint32_t                                     PciFlags;
    uint16_t                                     PciDeviceId;
    uint16_t                                     PciVendorId;
    uint8_t                                      PciBusNumber;
    uint8_t                                      PciDeviceNumber;
    uint8_t                                      PciFunctionNumber;
    uint8_t                                      PciSegment;
} PLATFORM_OVERRIDE_UART_INFO_TABLE;

/**
  @brief MSI(X) controllers info structure
**/

typedef struct {
    uint32_t  vector_upper_addr; /* Bus Device Function */
    uint32_t  vector_lower_addr; /* Base Address of the controller */
    uint32_t  vector_data;       /* Base Address of the controller */
    uint32_t  vector_control;    /* IRQ to install an ISR */
    uint64_t  vector_irq_base;   /* Base IRQ for the vectors in the block */
    uint32_t  vector_n_irqs;     /* Number of irq vectors in the block */
    uint32_t  vector_mapped_irq_base; /* Mapped IRQ number base for this MSI */
} PERIPHERAL_VECTOR_BLOCK;

typedef struct PERIPHERAL_VECTOR_LIST_STRUCT {
    PERIPHERAL_VECTOR_BLOCK vector;
    struct PERIPHERAL_VECTOR_LIST_STRUCT *next;
} PERIPHERAL_VECTOR_LIST;

uint32_t pal_get_msi_vectors(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn,
                             PERIPHERAL_VECTOR_LIST **mvector);

/**
  @brief DMA controllers info structure
**/
typedef enum {
    DMA_TYPE_USB  =  0x2000,
    DMA_TYPE_SATA,
    DMA_TYPE_OTHER,
} DMA_INFO_TYPE_e;

typedef struct {
    DMA_INFO_TYPE_e type;
    void            *target;  /* The actual info stored in these pointers is
                                 implementation specific. */
    void            *port;
    void            *host;    /* It will be used only by PAL. hence void. */
    uint32_t        flags;
} DMA_INFO_BLOCK;

typedef struct {
    uint32_t         num_dma_ctrls;
    DMA_INFO_BLOCK   info[];    /* Array of information blocks - per DMA controller */
} DMA_INFO_TABLE;

typedef enum {
    EDMA_NO_SUPPORT   = 0x0,
    EDMA_COHERENT     = 0x1,
    EDMA_NOT_COHERENT = 0x2,
    EDMA_FROM_DEVICE  = 0x3,
    EDMA_TO_DEVICE    = 0x4
} EXERCISER_DMA_ATTR;

typedef enum {
    SNOOP_ATTRIBUTES = 0x1,
    LEGACY_IRQ       = 0x2,
    MSIX_ATTRIBUTES  = 0x3,
    DMA_ATTRIBUTES   = 0x4,
    P2P_ATTRIBUTES   = 0x5,
    PASID_ATTRIBUTES = 0x6,
    CFG_TXN_ATTRIBUTES = 0x7,
    ATS_RES_ATTRIBUTES = 0x8,
    TRANSACTION_TYPE  = 0x9,
    NUM_TRANSACTIONS  = 0xA,
    ADDRESS_ATTRIBUTES = 0xB,
    DATA_ATTRIBUTES = 0xC,
    ERROR_INJECT_TYPE = 0xD

} EXERCISER_PARAM_TYPE;

typedef enum {
    EXERCISER_RESET = 0x1,
    EXERCISER_ON    = 0x2,
    EXERCISER_OFF   = 0x3,
    EXERCISER_ERROR = 0x4
} EXERCISER_STATE;

typedef enum {
    START_DMA     = 0x1,
    GENERATE_MSI  = 0x2,
    GENERATE_L_INTR = 0x3,  /* Legacy interrupt */
    MEM_READ      = 0x4,
    MEM_WRITE     = 0x5,
    CLEAR_INTR    = 0x6,
    PASID_TLP_START = 0x7,
    PASID_TLP_STOP  = 0x8,
    TXN_NO_SNOOP_ENABLE = 0x9,
    TXN_NO_SNOOP_DISABLE  = 0xa,
    START_TXN_MONITOR    = 0xb,
    STOP_TXN_MONITOR     = 0xc,
    ATS_TXN_REQ          = 0xd,
    INJECT_ERROR         = 0xe
} EXERCISER_OPS;

/* LibC functions declaration */

int pal_mem_compare(void *Src, void *Dest, uint32_t Len);
void *pal_strncpy(void *DestinationStr, const void *SourceStr, uint32_t Length);
uint32_t pal_strncmp(const char8_t *str1, const char8_t *str2, uint32_t len);
void pal_mem_set(void *buf, uint32_t size, uint8_t value);

uint32_t pal_pcie_p2p_support(void);
uint32_t pal_pcie_device_driver_present(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
uint32_t pal_pcie_get_rp_transaction_frwd_support(uint32_t seg, uint32_t bus, uint32_t dev,
                                                                              uint32_t fn);
uint32_t pal_pcie_is_onchip_peripheral(uint32_t bdf);
uint32_t pal_pcie_check_device_valid(uint32_t bdf);
uint32_t pal_pcie_mem_get_offset(uint32_t bdf, PCIE_MEM_TYPE_INFO_e mem_type);

void     pal_pcie_create_info_table(PCIE_INFO_TABLE *PcieTable);
uint64_t pal_pcie_ecam_base(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func);
uint32_t pal_pcie_get_pcie_type(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
uint32_t pal_pcie_get_snoop_bit(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
void pal_pcie_read_ext_cap_word(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn,
                                uint32_t ext_cap_id, uint8_t offset, uint16_t *val);
uint32_t pal_pcie_get_dma_support(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
uint32_t pal_pcie_get_dma_coherent(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);

uint32_t pal_pcie_dev_p2p_support(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
uint32_t pal_pcie_is_devicedma_64bit(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
uint32_t pal_pcie_is_cache_present(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn);
uint32_t pal_pcie_get_legacy_irq_map(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn,
                                                              PERIPHERAL_IRQ_MAP *irq_map);
uint32_t pal_pcie_get_root_port_bdf(uint32_t *seg, uint32_t *bus, uint32_t *dev, uint32_t *func);
uint32_t pal_pcie_check_device_list(void);
uint32_t pal_pcie_bar_mem_read(uint32_t bdf, uint64_t address, uint32_t *data);
uint32_t pal_pcie_bar_mem_write(uint32_t bdf, uint64_t address, uint32_t data);
void     pal_pci_cfg_write(uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset,
                                                                       uint32_t data);
void pal_pcie_rp_program_bar(uint32_t bus, uint32_t dev, uint32_t func);

uint32_t pal_is_bdf_exerciser(uint32_t bdf);
void pal_pcie_program_bar_reg(uint32_t bus, uint32_t dev, uint32_t func);
uint32_t pal_pcie_enumerate_device(uint32_t bus, uint32_t sec_bus);
void pal_clear_pri_bus(void);
void pal_pcie_enumerate(void);
uint32_t pal_pcie_get_bdf_wrapper(uint32_t class_code, uint32_t start_bdf);
void *pal_pci_bdf_to_dev(uint32_t bdf);
uint64_t pal_exerciser_get_pcie_config_offset(uint32_t Bdf);
uint64_t pal_exerciser_get_ecam(uint32_t bdf);
uint64_t pal_exerciser_get_ecsr_base(uint32_t Bdf, uint32_t BarIndex);
uint32_t pal_exerciser_find_pcie_capability (uint32_t ID, uint32_t Bdf, uint32_t Value,
                                                                     uint32_t *Offset);
void pal_pcie_info_table_init(void);

#endif
