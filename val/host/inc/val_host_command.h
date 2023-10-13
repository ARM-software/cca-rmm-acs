 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef _VAL_HOST_COMMAND_H
#define _VAL_HOST_COMMAND_H

uint64_t create_mapping(uint64_t ipa, bool ripas_init, uint64_t rd1);
uint64_t val_host_delegate_granule(void);
uint64_t val_host_undelegate_granule(void);

#endif /* _VAL_HOST_COMMAND_H */