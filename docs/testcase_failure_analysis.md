# Test failure analysis document

This file contains list of failures and skips identified when testing the release on tgt_tfa_fvp target with TF-RMM and TF-A.<br />
TF-RMM Hash: f16509d96ba87bd4ec99c745c6fd1a99d9b15407 <br />
TF-A Hash: 9244331f354af870a2f38775aaaddb47bbec7b39<br />
The reason for each failing/skipping test is listed here in this file.<br />

## List of skipped tests

| Test | Skip description                                                                |
|------|---------------------------------------------------------------------------------|
| exception_rec_exit_wfe | Platform limitation |
| mm_gpf_exception | Platform limitation |

## License

Arm CCA RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023-2025, Arm Limited or its affliates. All rights reserved.*
