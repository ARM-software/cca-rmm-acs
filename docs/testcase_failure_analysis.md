# Test failure analysis document

This file contains list of failures identified when testing the release on tgt_tfa_fvp target with TF-RMM and TF-A.<br />
TF-RMM Hash: 5955abce65e60d34f1fc33af699d28372302675a <br />
TF-A Hash: d94a71193a3a3dfae4301456b9785b8e7edfc859 <br />
The reason for each failing test is listed here in this file.<br />

## List of failed tests

| Test | Fail description                                                                |
|------|---------------------------------------------------------------------------------|
| cmd_rec_create | Reference implementation issue |
| cmd_rmi_version | Reference implementation issue |
| cmd_rsi_version | Reference implementation issue |
| exception_rec_exit_wfe | Reference Platform limitation |
| exception_non_emulatable_da | Reference implementation issue |
| gic_hcr_invalid | Reference implementation issue |
| mm_gpf_exception | Reference Platform limitation |
| mm_hipas_destroyed_ripas_ram_ia | Reference implementation issue |
| mm_hipas_destroyed_ripas_empty_ia | Reference implementation issue |
| mm_hipas_unassigned_ripas_ram_ia | Reference implementation issue |
| mm_rtt_fold_assigned | Reference implementation issue |
| mm_rtt_fold_unassigned | Reference implementation issue |
| mm_rtt_fold_destroyed | Reference implementation issue |
| mm_rtt_fold_u_ipa_assigned | Reference implementation issue |


## License

Arm CCA RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023, Arm Limited or its affliates. All rights reserved.*
