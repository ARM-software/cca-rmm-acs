# Test failure analysis document

This file contains list of failures identified when testing the release on tgt_tfa_fvp target with TF-RMM and TF-A.<br />
TF-RMM Hash: 5955abce65e60d34f1fc33af699d28372302675a <br />
TF-A Hash: d94a71193a3a3dfae4301456b9785b8e7edfc859 <br />
The reason for each failing test is listed here in this file.<br />

## List of failed tests

| Test | Fail description                                                                |
|------|---------------------------------------------------------------------------------|
| cmd_rec_create | RMM implementation issue(GENFW-7292) |
| cmd_rmi_version | RMM implementation issue(GENFW-7291) |
| cmd_rsi_version | RMM implementation issue(GENFW-7291) |
| exception_rec_exit_wfe | Reference Platform limitation |
| exception_non_emulatable_da | RMM implementation issue(GENFW-7272) |
| gic_hcr_invalid | RMM implementation issue(GENFW-7279) |
| mm_gpf_exception | Reference Platform limitation |
| mm_hipas_destroyed_ripas_ram_ia | RMM implementation issue(GENFW-7271) |
| mm_hipas_destroyed_ripas_empty_ia | RMM implementation issue(GENFW-7271) |
| mm_hipas_unassigned_ripas_ram_ia | RMM implementation issue(GENFW-7271) |
| mm_rtt_fold_assigned | RMM implementation issue(GENFW-7293) |
| mm_rtt_fold_unassigned | RMM implementation issue(GENFW-7293) |
| mm_rtt_fold_destroyed | RMM implementation issue(GENFW-7293) |
| mm_rtt_fold_u_ipa_assigned | RMM implementation issue(GENFW-7293) |
| mm_feat_s2fwb_check_2 | RMM ACS infrastructure issue(GENFW-7297) |
| mm_feat_s2fwb_check_3 | RMM ACS infrastructure issue(GENFW-7297) |


## License

Arm RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023, Arm Limited or its affliates. All rights reserved.*
