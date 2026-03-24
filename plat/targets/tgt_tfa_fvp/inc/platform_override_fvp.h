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

/* PCIe BAR config parameters*/
#define PLATFORM_OVERRIDE_PCIE_BAR64_VALUE   0x4000100000
#define PLATFORM_OVERRIDE_RP_BAR64_VALUE     0x4000000000
#define PLATFORM_OVERRIDE_PCIE_BAR32NP_VALUE 0x50000000
#define PLATFORM_OVERRIDE_PCIE_BAR32P_VALUE  0x50600000
#define PLATOFRM_OVERRIDE_RP_BAR32_VALUE     0x50850000

/* PCIE platform config parameters */
#define PLATFORM_OVERRIDE_NUM_ECAM                1

/* Offset from the memory range to be accesed
 * Modify this macro w.r.t to the requirement */
#define MEM_OFFSET_SMALL   0x10
#define MEM_OFFSET_MEDIUM  0x1000

/* Platform config parameters for ECAM_0 */
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0   0x40000000
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0  0x0
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0    0x0
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0      0xFF

#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0_SIZE 0x1000 * 0x10000

#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_1  0x1010000000
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_1 0x0
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_1   0x40
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_1     0x7F

#define PLATFORM_OVERRIDE_MAX_BDF           1
#define PLATFORM_OVERRIDE_PCIE_MAX_BUS      0x9
#define PLATFORM_OVERRIDE_PCIE_MAX_DEV      32
#define PLATFORM_OVERRIDE_PCIE_MAX_FUNC     8

/* Sample macros for ECAM_1
 * #define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_1  0x00000000
 * #define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_1 0x0
 * #define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_1   0x0
 * #define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_1     0x0
 */

/* PCIE device hierarchy table */

#define PLATFORM_PCIE_NUM_ENTRIES        5
#define PLATFORM_PCIE_P2P_NOT_SUPPORTED  1

#define PLATFORM_PCIE_DEV0_CLASSCODE     0xFF000000
#define PLATFORM_PCIE_DEV0_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV0_DEV_ID        0xFF80
#define PLATFORM_PCIE_DEV0_BUS_NUM       0
#define PLATFORM_PCIE_DEV0_DEV_NUM       0
#define PLATFORM_PCIE_DEV0_FUNC_NUM      0
#define PLATFORM_PCIE_DEV0_SEG_NUM       0

#define PLATFORM_PCIE_DEV1_CLASSCODE     0x6040000
#define PLATFORM_PCIE_DEV1_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV1_DEV_ID        0xDEF
#define PLATFORM_PCIE_DEV1_BUS_NUM       0
#define PLATFORM_PCIE_DEV1_DEV_NUM       0xF
#define PLATFORM_PCIE_DEV1_FUNC_NUM      0
#define PLATFORM_PCIE_DEV1_SEG_NUM       0

#define PLATFORM_PCIE_DEV2_CLASSCODE     0x6040000
#define PLATFORM_PCIE_DEV2_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV2_DEV_ID        0xDEF
#define PLATFORM_PCIE_DEV2_BUS_NUM       0
#define PLATFORM_PCIE_DEV2_DEV_NUM       0x1E
#define PLATFORM_PCIE_DEV2_FUNC_NUM      0
#define PLATFORM_PCIE_DEV2_SEG_NUM       0

#define PLATFORM_PCIE_DEV3_CLASSCODE     0xFF000000
#define PLATFORM_PCIE_DEV3_VENDOR_ID     0x13B5
#define PLATFORM_PCIE_DEV3_DEV_ID        0xFF80
#define PLATFORM_PCIE_DEV3_BUS_NUM       1
#define PLATFORM_PCIE_DEV3_DEV_NUM       0
#define PLATFORM_PCIE_DEV3_FUNC_NUM      0
#define PLATFORM_PCIE_DEV3_SEG_NUM       0

#define PLATFORM_PCIE_DEV4_CLASSCODE     0x1060101
#define PLATFORM_PCIE_DEV4_VENDOR_ID     0xABC
#define PLATFORM_PCIE_DEV4_DEV_ID        0xACED
#define PLATFORM_PCIE_DEV4_BUS_NUM       2
#define PLATFORM_PCIE_DEV4_DEV_NUM       0
#define PLATFORM_PCIE_DEV4_FUNC_NUM      0
#define PLATFORM_PCIE_DEV4_SEG_NUM       0

/* DMA platform config parameters */
#define PLATFORM_OVERRIDE_DMA_CNT   0

/*Exerciser platform config details*/
#define TEST_REG_COUNT              10
#define EXERCISER_ID                0xED0113B5
#define PCIE_CAP_CTRL_OFFSET        0x4// offset from the extended capability header

/* Exerciser MMIO Offsets */
#define INTXCTL         0x004
#define MSICTL          0x000
#define DMACTL1         0x08
#define DMA_BUS_ADDR    0x010
#define DMA_LEN         0x018
#define DMASTATUS       0x01C
#define PCI_MAX_BUS     255
#define PCI_MAX_DEVICE  31
#define PASID_VAL       0x020
#define ATSCTL          0x024
#define TXN_TRACE     0x40
#define TXN_CTRL_BASE 0x44
#define ATS_ADDR        0x028

#define DVSEC_CTRL     0x8
#define PCI_EXT_CAP_ID  0x10
#define PASID           0x1B
#define PCIE            0x1
#define PCI             0x0
#define DVSEC          0x0023
#define AER            0x0001

/* PCI/PCIe express extended capability structure's
   next capability pointer mask and cap ID mask */
#define PCIE_NXT_CAP_PTR_MASK 0x0FFF
#define PCIE_CAP_ID_MASK      0xFFFF
#define PCI_CAP_ID_MASK       0x00FF
#define PCI_NXT_CAP_PTR_MASK  0x00FF
#define CAP_PTR_MASK          0x00FF

#define CLR_INTR_MASK       0xFFFFFFFE
#define PASID_TLP_STOP_MASK 0xFFFFFFBF
#define PASID_VAL_MASK      ((0x1ul << 20) - 1)
#define PASID_VAL_SHIFT     12
#define PASID_LEN_SHIFT     7
#define PASID_LEN_MASK      0x7ul
#define PASID_EN_SHIFT      6
#define DMA_TO_DEVICE_MASK  0xFFFFFFEF

/* shift_bit */
#define SHIFT_1BIT             1
#define SHIFT_2BIT             2
#define SHIFT_4BIT             4
#define SHITT_8BIT             8
#define MASK_BIT               1
#define PREFETCHABLE_BIT_SHIFT 3
#define ERR_CODE_SHIFT         20
#define FATAL_SHIFT            31
#define ERROR_INJECT_BIT       17

#define PCI_CAP_PTR_OFFSET     8
#define PCIE_CAP_PTR_OFFSET    20

#define MSI_GENERATION_MASK (1 << 31)

#define NO_SNOOP_START_MASK 0x20
#define NO_SNOOP_STOP_MASK  0xFFFFFFDF
#define PCIE_CAP_DIS_MASK   0xFFFEFFFF
#define PCIE_CAP_EN_MASK    (1 << 16)
#define PASID_EN_MASK       (1 << 6)

/* PCIe Config space Offset */
#define BAR0_OFFSET         0x10
#define COMMAND_REG_OFFSET  0x04
#define CAP_PTR_OFFSET      0x34
#define PCIE_CAP_OFFSET     0x100

#define RID_CTL_REG    0x3C
#define RID_VALUE_MASK 0xFFFF
#define RID_VALID_MASK (1ul << 31)
#define RID_VALID      1
#define RID_NOT_VALID  0
#define ATS_TRIGGER    1
#define ATS_STATUS     (1ul << 7)
#define TXN_INVALID    0xFFFFFFFF
#define TXN_START      1
#define TXN_STOP       0

#define PCIE_CAP_CTRL_OFFSET  0x4 /* offset from the extended capability header */

#define PLATFORM_IDE_STREAM_ID           255
#define PLATFORM_PCIE_RID_BASE           0x100
#define PLATFORM_PCIE_RID_TOP            0x801

/** End config **/
