# Test failure analysis document

This file contains list of failures identified when testing the release on tgt_tfa_fvp target with TF-RMM and TF-A.<br />
TF-RMM Hash: 690c2aade299fd082327ee65c5e5d9da3185bd97 <br />
TF-A Hash: 56ddb3f0922506d304177274b4c0f15b2069a7e6 <br />
The reason for each failing test is listed here in this file.<br />

## List of failed tests

| Test | Fail description                                                                |
|------|---------------------------------------------------------------------------------|
| cmd_rmi_version | TF-RMM Reference implementation issue |
| cmd_rsi_version | TF-RMM Reference implementation issue |
| mm_ripas_destroyed_da_ia | TF-RMM Reference implementation issue |
| mm_hipas_unassigned_ripas_ram_da_ia | TF-RMM Reference implementation issue |
| mm_rtt_fold_assigned_ns | Validated with TF-RMM patch https://review.trustedfirmware.org/c/TF-RMM/tf-rmm/+/23708 |

## License

Arm CCA RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023, Arm Limited or its affliates. All rights reserved.*
