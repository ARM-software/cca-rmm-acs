.. Copyright [C] 2023, Arm Limited or its affiliates. All rights reserved.
      SPDX-License-Identifier: BSD-3-Clause

##########
Change-log
##########

******************
v1.0_EAC1.0_12.23
******************

-  This release is aligned to `RMM Eac5 specification`_ . Details are given below.

New Features / Tests Added
==========================

- One new test added in command suite
- Enhancements and bug fixes to existing tests and infrastructure.

******************
v1.0_BETA0.8_10.23
******************

-  This release adds additional tests and intfrastructure and is aligned to
   `RMM Eac2 specification`_ . Details are given below.

New Features / Tests Added
==========================

- Two new suites added : Debug & PMU and Attestation & Measurement.
- Added tests & infrastructure for Debug & PMU and Attestation & Measurement whose scenarios can be
  found at `PMU and Debug Scenarios`_ and `Attestation and Measurement Scenarios`_.
- Enhancements and bug fixes to existing tests and infrastructure.
- New tests added in Command, Exception, GIC and memory management suites.

***************
v29.03_ALPHA0.7
***************

-  First CCA-RMM-ACS release aligned to `RMM Beta1 specification`_
   populated with initial tests and infrastructure testing various aspects
   of the specification

New Features / Tests Added
==========================

- Initial tests and infrastructure for Interface, Exception, GIC and Memory management tests.

*****
Note:
*****

- Tag format for latest version: v{SPEC-VERSION}_{ACS-CODE-QUALITY}{ACS-VERSION}_{MONTH.YEAR}

.. _RMM Eac5 specification: https://developer.arm.com/documentation/den0137/1-0eac5/?lang=en
.. _RMM Eac2 specification: https://developer.arm.com/documentation/den0137/1-0eac2/?lang=en
.. _RMM Beta1 specification: https://developer.arm.com/documentation/den0137/1-0bet1/?lang=en
.. _PMU and Debug Scenarios: ./pmu_debug.md
.. _Attestation and Measurement Scenarios: ./attestation_measurement_scenarios.md
