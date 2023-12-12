# Test failure analysis document

This file contains list of failures and skips identified when testing the release on tgt_tfa_fvp target with TF-RMM and TF-A.<br />
TF-RMM Hash: 9aca983c50230cb9fe444fa00babb0d4a534ff47 <br />
TF-A Hash: b6c0948400594e3cc4dbb5a4ef04b815d2675808 <br />
The reason for each failing/skipping test is listed here in this file.<br />

## List of failed tests

| Test | Fail description                                                                |
|------|---------------------------------------------------------------------------------|
| cmd_rtt_init_ripas | TF-RMM Reference implementation issue |
| cmd_rtt_set_ripas | TF-RMM Reference implementation issue |

## List of skipped tests

| Test | Skip description                                                                |
|------|---------------------------------------------------------------------------------|
| exception_rec_exit_wfe | Platform limitation |
| mm_gpf_exception | Platform limitation |

## License

Arm CCA RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023, Arm Limited or its affliates. All rights reserved.*
