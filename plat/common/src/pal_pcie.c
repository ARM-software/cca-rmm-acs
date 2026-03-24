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

#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "platform_override_struct.h"
#include "pal_mmio.h"

extern pcie_device_bdf_table * g_pcie_bdf_table;
extern PERIPHERAL_INFO_TABLE *g_peripheral_info_table;


PCIE_INFO_TABLE platform_pcie_cfg = {
    .num_entries             = PLATFORM_OVERRIDE_NUM_ECAM,
    .block[0]      = {PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0,
                      PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0,
                      PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0,
                      PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0}
    /* Sample code for more than 1 ECAM */
    /*
     * .block[0]      = {PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_1,
     *                   PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_1,
     *                  PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_1,
     *                   PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_1}
     */
};

PCIE_READ_TABLE platform_pcie_device_hierarchy = {
    .num_entries             = PLATFORM_PCIE_NUM_ENTRIES,
    .device[0] = {PLATFORM_PCIE_DEV0_CLASSCODE,
    PLATFORM_PCIE_DEV0_DEV_ID,
    PLATFORM_PCIE_DEV0_VENDOR_ID,
    PLATFORM_PCIE_DEV0_BUS_NUM,
    PLATFORM_PCIE_DEV0_DEV_NUM,
    PLATFORM_PCIE_DEV0_FUNC_NUM,
    PLATFORM_PCIE_DEV0_SEG_NUM
    },

    .device[1] = {PLATFORM_PCIE_DEV1_CLASSCODE,
    PLATFORM_PCIE_DEV1_DEV_ID,
    PLATFORM_PCIE_DEV1_VENDOR_ID,
    PLATFORM_PCIE_DEV1_BUS_NUM,
    PLATFORM_PCIE_DEV1_DEV_NUM,
    PLATFORM_PCIE_DEV1_FUNC_NUM,
    PLATFORM_PCIE_DEV1_SEG_NUM
    },

    .device[2] = {PLATFORM_PCIE_DEV2_CLASSCODE,
    PLATFORM_PCIE_DEV2_DEV_ID,
    PLATFORM_PCIE_DEV2_VENDOR_ID,
    PLATFORM_PCIE_DEV2_BUS_NUM,
    PLATFORM_PCIE_DEV2_DEV_NUM,
    PLATFORM_PCIE_DEV2_FUNC_NUM,
    PLATFORM_PCIE_DEV2_SEG_NUM
    },

    .device[3] = {PLATFORM_PCIE_DEV3_CLASSCODE,
    PLATFORM_PCIE_DEV3_DEV_ID,
    PLATFORM_PCIE_DEV3_VENDOR_ID,
    PLATFORM_PCIE_DEV3_BUS_NUM,
    PLATFORM_PCIE_DEV3_DEV_NUM,
    PLATFORM_PCIE_DEV3_FUNC_NUM,
    PLATFORM_PCIE_DEV3_SEG_NUM
    },

    .device[4] = {PLATFORM_PCIE_DEV4_CLASSCODE,
    PLATFORM_PCIE_DEV4_DEV_ID,
    PLATFORM_PCIE_DEV4_VENDOR_ID,
    PLATFORM_PCIE_DEV4_BUS_NUM,
    PLATFORM_PCIE_DEV4_DEV_NUM,
    PLATFORM_PCIE_DEV4_FUNC_NUM,
    PLATFORM_PCIE_DEV4_SEG_NUM
    },
};

void pal_pcie_create_info_table(PCIE_INFO_TABLE *PcieTable)
{

    uint32_t i = 0;

    if (PcieTable == NULL) {
      pal_printf("ERROR: Input PCIe Table Pointer is NULL. Cannot create PCIe INFO\n", 0, 0);
      return;
    }

    PcieTable->num_entries = 0;

    if (platform_pcie_cfg.num_entries == 0) {
        pal_printf("ERROR: Number of ECAM is 0. Cannot create PCIe INFO\n", 0, 0);
        return;
    }

    for (i = 0; i < platform_pcie_cfg.num_entries; i++)
    {
        PcieTable->block[i].ecam_base      = platform_pcie_cfg.block[i].ecam_base;
        PcieTable->block[i].segment_num    = platform_pcie_cfg.block[i].segment_num;
        PcieTable->block[i].start_bus_num  = platform_pcie_cfg.block[i].start_bus_num;
        PcieTable->block[i].end_bus_num    = platform_pcie_cfg.block[i].end_bus_num;
        PcieTable->num_entries++;
    }
    return;
}

/**
  @brief  Returns the ECAM address of the input PCIe bridge function

  @param   bus        PCI bus address
  @param   dev        PCI device address
  @param   fn         PCI function number
  @param   seg        PCI segment number

  @return ECAM address if success, else NULL address
**/
uint64_t pal_pcie_ecam_base(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func)
{

    uint8_t ecam_index;
    uint64_t ecam_base;

    (void) dev;
    (void) func;

    ecam_index = 0;
    ecam_base = 0;


    while (ecam_index < platform_pcie_cfg.num_entries)
    {
        if ((bus >= platform_pcie_cfg.block[ecam_index].start_bus_num) &&
            (bus <= platform_pcie_cfg.block[ecam_index].end_bus_num) &&
            (seg == platform_pcie_cfg.block[ecam_index].segment_num))
        {
            ecam_base = platform_pcie_cfg.block[ecam_index].ecam_base;
            break;
        }
        ecam_index++;
    }

    return ecam_base;
}

/**
    @brief   Reads 32-bit data from PCIe config space pointed by Bus,
           Device, Function and register offset

    @param   Bdf      - BDF value for the device
    @param   offset - Register offset within a device PCIe config space
    @param   *data - 32 bit value at offset from ECAM base of the device specified by BDF value
    @return  success/failure
**/
uint32_t pal_pcie_read_cfg(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func,
                                                  uint32_t offset, uint32_t *value)
{

    uint32_t cfg_addr;
    uint64_t ecam_base = pal_pcie_ecam_base(seg, bus, dev, func);

    ecam_base = pal_pcie_ecam_base(seg, bus, dev, func);
    cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
                (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

    *value = pal_mmio_read32(ecam_base + cfg_addr + offset);
    return 0;

}

/**
    @brief   Get the PCIe device/port type

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  Returns PCIe device/port type
**/
uint32_t pal_pcie_get_pcie_type(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

    uint32_t reg_value;
    uint32_t next_cap_offset;

    pal_pcie_read_cfg(seg, bus, dev, fn, TYPE01_CPR, &reg_value);
    next_cap_offset = (reg_value & TYPE01_CPR_MASK);
    while (next_cap_offset)
    {
      pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
      if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
      {
            return ((reg_value >> PCIE_DEVICE_TYPE_SHIFT) & PCIE_DEVICE_TYPE_MASK);
      }
      next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
    }

    return 0;
}

/**
    @brief   Get the PCIe device snoop bit transaction attribute

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  0 snoop
             1 no snoop
             2 device error
**/
uint32_t pal_pcie_get_snoop_bit(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

    uint32_t reg_value;
    uint32_t next_cap_offset;

    pal_pcie_read_cfg(seg, bus, dev, fn, TYPE01_CPR, &reg_value);
    next_cap_offset = (reg_value & TYPE01_CPR_MASK);
    while (next_cap_offset)
    {
      pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
      if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
      {
            pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset + PCI_EXP_DEVCTL, &reg_value);
            /* Extract bit 11 (no snoop) */
            return ((reg_value >> DEVCTL_SNOOP_BIT) & 0x1);
      }
      next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
    }

    return 2;
}

void pal_pcie_read_ext_cap_word(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn,
                           uint32_t ext_cap_id, uint8_t offset, uint16_t *val)
{

    uint32_t next_cap_offset;
    uint32_t reg_value;

    next_cap_offset = PCIE_ECAP_START;
    while (next_cap_offset)
    {
        pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
        if ((reg_value & PCIE_ECAP_CIDR_MASK) == ext_cap_id)
        {
            pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset + offset, &reg_value);
            *val = reg_value & 0xffff;
        }
        next_cap_offset = ((reg_value >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK);
    }

    return;
}


/**
    @brief   Get bdf of root port

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   seg        PCI segment number

    @return  BDF of root port
    @return  status code
             0: Success
             1: Input BDF cannot be found
             2: RP of input device not found
**/
uint32_t pal_pcie_get_root_port_bdf(uint32_t *Seg, uint32_t *Bus, uint32_t *Dev, uint32_t *Func)
{

    uint32_t bdf;
    uint32_t dp_type;
    uint32_t tbl_index;
    uint32_t reg_value;
    uint32_t next_cap_offset;
    uint32_t curr_seg, curr_bus, curr_dev, curr_func;
    pcie_device_bdf_table *bdf_tbl_ptr;

    tbl_index = 0;
    bdf_tbl_ptr = g_pcie_bdf_table;

    while (tbl_index < bdf_tbl_ptr->num_entries)
    {
        bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
        curr_seg  = PCIE_EXTRACT_BDF_SEG(bdf);
        curr_bus  = PCIE_EXTRACT_BDF_BUS(bdf);
        curr_dev  = PCIE_EXTRACT_BDF_DEV(bdf);
        curr_func = PCIE_EXTRACT_BDF_FUNC(bdf);
        pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, TYPE01_CPR, &reg_value);
        next_cap_offset = (reg_value & TYPE01_CPR_MASK);
        while (next_cap_offset)
        {
          pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, next_cap_offset, &reg_value);
          if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
          {
              pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func,
                            next_cap_offset + CIDR_OFFSET, &reg_value);
              /* Read Device/Port bits [7:4] in Function's PCIe Capabilities register */
              dp_type = (reg_value >> ((PCIECR_OFFSET - CIDR_OFFSET)*8 +
                              PCIECR_DPT_SHIFT)) & PCIECR_DPT_MASK;
              dp_type = (1 << dp_type);

              if (dp_type == RP || dp_type == iEP_RP)
              {
                  /* Check if the entry's bus range covers down stream function */
                pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, BUS_NUM_REG_OFFSET,
                                                                                  &reg_value);
                if ((*Bus >= ((reg_value >> SECBN_SHIFT) & SECBN_MASK)) &&
                    (*Bus <= ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK)))
                {
                    *Seg  = PCIE_EXTRACT_BDF_SEG(bdf);
                    *Bus  = PCIE_EXTRACT_BDF_BUS(bdf);
                    *Dev  = PCIE_EXTRACT_BDF_DEV(bdf);
                    *Func = PCIE_EXTRACT_BDF_FUNC(bdf);
                    return 0;
                }
              }
              return 2;
          }
          next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
        }
    }

    return 1;
}

/**
  @brief  Checks the discovered PCIe hierarchy is matching with the
          topology described in info table.
  @return Returns 0 if device entries matches , 1 if there is mismatch.
**/
uint32_t pal_pcie_check_device_list(void)
{
    uint32_t tbl_index = 0;
    uint32_t pltf_pcie_device_bdf;
    uint32_t bdf;
    pcie_device_bdf_table *bdf_tbl_ptr;
    uint32_t vendor_id, device_id, class_code;
    uint32_t pltf_vendor_id, pltf_device_id, pltf_class_code;
    uint32_t i = 0;
    uint32_t Seg, Bus, Dev, Func;
    uint32_t data = 0;

      bdf_tbl_ptr = g_pcie_bdf_table;

      if (platform_pcie_device_hierarchy.num_entries != bdf_tbl_ptr->num_entries) {
        pal_printf("ERROR:  Number of PCIe devices entries in info table not equal to \
                              platform hierarchy %d\n", bdf_tbl_ptr->num_entries, 0);
        return 1;
      }

      while (tbl_index < bdf_tbl_ptr->num_entries)
      {
        Seg  = platform_pcie_device_hierarchy.device[tbl_index].seg;
        Bus  = platform_pcie_device_hierarchy.device[tbl_index].bus;
        Dev  = platform_pcie_device_hierarchy.device[tbl_index].dev;
        Func = platform_pcie_device_hierarchy.device[tbl_index].func;
        pltf_vendor_id = platform_pcie_device_hierarchy.device[tbl_index].vendor_id;
        pltf_device_id = platform_pcie_device_hierarchy.device[tbl_index].device_id;
        pltf_class_code =
            (uint32_t)platform_pcie_device_hierarchy.device[tbl_index].class_code >> CC_SHIFT;

        pltf_pcie_device_bdf = PCIE_CREATE_BDF(Seg, Bus, Dev, Func);
        tbl_index++;
        while (i < bdf_tbl_ptr->num_entries) {
            bdf = bdf_tbl_ptr->device[i++].bdf;

            if (pltf_pcie_device_bdf == bdf)
            {
              Seg  = PCIE_EXTRACT_BDF_SEG(bdf);
              Bus  = PCIE_EXTRACT_BDF_BUS(bdf);
              Dev  = PCIE_EXTRACT_BDF_DEV(bdf);
              Func = PCIE_EXTRACT_BDF_FUNC(bdf);

              pal_pcie_read_cfg(Seg, Bus, Dev, Func, TYPE0_HEADER, &data);
              vendor_id = data & 0xFFFF;
              if (vendor_id != pltf_vendor_id) {
                  pal_printf("ERROR: VendorID mismatch for PCIe device with bdf = 0x%x\n", bdf, 0);
                  return 1;
              }
              device_id = data >> DEVICE_ID_OFFSET;
              if (device_id != pltf_device_id) {
                  pal_printf("ERROR: DeviceID mismatch for PCIe device with bdf = 0x%x\n", bdf, 0);
                  return 1;
              }
              pal_pcie_read_cfg(Seg, Bus, Dev, Func, TYPE01_RIDR, &class_code);
              class_code = class_code >> CC_SHIFT;
              if (class_code != pltf_class_code) {
                  pal_printf("ERROR: ClassCode mismatch for PCIe device with bdf = 0x%x\n", bdf, 0);
                  return 1;
              }

              i = 0;
              break;
            }

        }

        /* If any bdf match not found in platform device hierarchy and info table, return false */
        if (i == bdf_tbl_ptr->num_entries) {
          pal_printf("ERROR: Bdf not found in info table = 0x%x i=%x\n", pltf_pcie_device_bdf, i);
            return 1;
        }
      }
      return 0;
}

/**
    @brief   Reads 32-bit data from BAR space pointed by Bus,
             Device, Function and register offset.

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   *data   - 32 bit value at BAR address
    @return  success/failure
**/
uint32_t
pal_pcie_bar_mem_read(uint32_t Bdf, uint64_t address, uint32_t *data)
{
    (void) Bdf;

    *data = pal_mmio_read32(address);
    return 0;
}

/**
    @brief   Write 32-bit data to BAR space pointed by Bus,
             Device, Function and register offset.

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   data    - 32 bit value to writw BAR address
    @return  success/failure
**/

uint32_t
pal_pcie_bar_mem_write(uint32_t Bdf, uint64_t address, uint32_t data)
{
    (void) Bdf;

    pal_mmio_write32(address, data);
    return 0;
}
