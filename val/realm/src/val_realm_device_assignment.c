/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_realm_device_assignment.h"

/**
 *   @brief    Decode the interface report.
 *   @param    report_buff          Interface report to decode
 *   @param    interface_report     Strcuture pointer to device interface report
 *   @param    mmio_range_struct    Structure pointer to MMIO Range
 *   @return   SUCCESS/FAILURE
**/
uint64_t interface_report_decoding(uint8_t *report_buff,
    val_host_pci_tdisp_device_interface_report_struct_t *interface_report,
    pci_tdisp_mmio_range_t mmio_range_struct[])
{
    uint64_t interface_report_offset[6] = {INTERFACE_INFO, RESERVED,
                                 MSI_X_MESSGAE_CONTROL, LNR_CONTROL,
                                     TPH_CONTROL, MMIO_RANGE_COUNT};
    uint64_t interface_report_size_in_bytes[6] = {INTERFACE_INFO_SIZE, RESERVED_SIZE,
                                        MSI_X_MESSGAE_CONTROL_SIZE, LNR_CONTROL_SIZE,
                                            TPH_CONTROL_SIZE, MMIO_RANGE_COUNT_SIZE};
    uint64_t mmio_range_size_in_bytes[4] = {FIRST_PAGE_SIZE, NUM_PAGES,
                                           RANGE_ATTRIBUTES, RID_RANGE};
    uint64_t mmio_range_offset, i, j, k, offset, value;

    if (!report_buff)
        return VAL_ERROR;

    for (i = 0; i < sizeof(interface_report_offset)/8; i++)
    {
        offset = interface_report_offset[i] + interface_report_size_in_bytes[i];
        value = 0;
        for (j = 1; j <= interface_report_size_in_bytes[i]; j++)
        {
            value = value | report_buff[offset - j] <<
                            (8 * (interface_report_size_in_bytes[i] - j));
        }

        switch (i)
        {
            case 0:
                /* offset=0, size=2 bytes */
                interface_report->interface_info = (uint16_t)value;
                break;
            case 1:
                /* offset=2, size=2 bytes */
                interface_report->reserved = (uint16_t)value;
                break;
            case 2:
                /* offset=4, size=2 bytes */
                interface_report->msi_x_message_control = (uint16_t)value;
                break;
            case 3:
                /* offset=6, size=2 bytes */
                interface_report->lnr_control = (uint16_t)value;
                break;
            case 4:
                /* offset=8, size=4 bytes */
                interface_report->tph_control = (uint32_t)value;
                break;
            case 5:
                /* offset=12, size=4 bytes */
                interface_report->mmio_range_count = (uint32_t)value;
                break;
            default:
                LOG(ERROR, " Error in decoding interface report.\n");
                break;
        }
    }

    LOG(TEST, "\n Interface info %x\n", (uint64_t)interface_report->interface_info);
    LOG(TEST, "\n Reserved %x\n", (uint64_t)interface_report->reserved);
    LOG(TEST, "\n MSI X Message Control %x\n",
         (uint64_t)interface_report->msi_x_message_control);
    LOG(TEST, "\n LNR Control %x\n", (uint64_t)interface_report->lnr_control);
    LOG(TEST, "\n TPH Control %x\n", (uint64_t)interface_report->tph_control);
    LOG(TEST, "\n MMIO Range Count %x\n", (uint64_t)interface_report->mmio_range_count);

    for (i = 0; i < interface_report->mmio_range_count; i++)
    {
        mmio_range_offset = 16 + (i * 16);
        offset = mmio_range_offset;

        for (j = 0; j < 4; j++)
        {
            offset = offset + mmio_range_size_in_bytes[j];
            value = 0;
            for (k = 1; k <= mmio_range_size_in_bytes[j]; k++)
            {
                value = value | report_buff[offset - k] << (8 * (mmio_range_size_in_bytes[j] - k));
            }

            switch (j)
            {
                case 0:
                    mmio_range_struct[i].first_page = value << 12;
                    break;
                case 1:
                    mmio_range_struct[i].number_of_pages = (uint32_t)value;
                    break;
                case 2:
                    mmio_range_struct[i].range_attributes = (uint16_t)value;
                    break;
                case 3:
                    mmio_range_struct[i].range_id = (uint16_t)value;
                    break;
                default:
                    LOG(ERROR, " Error in decoding interface report.\n");
                    return VAL_ERROR;
            }
        }
    }

    for (i = 0; i < interface_report->mmio_range_count ; i++)
    {
        LOG(TEST, "\n MMIO Range mmio_range_count: %d\n", i);
        LOG(TEST, "\n MMIO Range first_page :%lx\n", mmio_range_struct[i].first_page);
        LOG(TEST, "\n MMIO Range number_of_pages :%lx\n", mmio_range_struct[i].number_of_pages);
        LOG(TEST, "\n MMIO Range range_attributes :%lx\n",
                mmio_range_struct[i].range_attributes);
        LOG(TEST, "\n MMIO Range range_id :%lx\n", mmio_range_struct[i].range_id);
    }

    return VAL_SUCCESS;
}