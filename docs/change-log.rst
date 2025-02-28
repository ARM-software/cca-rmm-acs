.. Copyright [C] 2023-2025, Arm Limited or its affiliates. All rights reserved.
      SPDX-License-Identifier: BSD-3-Clause

##########
Change-log
##########

***********************
v1.1_MEC_LFA_BETA_02_25
***********************

-  This release is aligned to `RMM v1.1 alp12 specification`_ . Details are given below.

New Features / Tests Added
==========================

- Added MEC and LFA infra
- Added MEC shared and private ID scenarios
- Added MEC_SET_PRIVATE and MEC_SET_SHARED command ABIs 
- Added LFA test
- For more details on these tests details check `MEC LFA Scenarios`_.

***************
v1.0_REL0_12.24
***************

-  This release is aligned to `RMM v1.0 rel0 specification`_ . Details are given below.

New Features / Tests Added
==========================

- Changes to VAL, Test suites to align with REL0 specification.
- XLAT(Page table management library) updated to be built independently and moved to tools/lib
  folder.
- Documentation updated to align with REL0 specification.

Bug fixes/improvements
======================

- Fix bug in S2AP change flow and aux_vmid offset.
- Fix bugs in tests generating non emulatable aborts and entering REC with EMULATED_MMIO flag.

***********************
v1.1_PLANES_BETA_09_24
***********************

-  This release is aligned to `RMM v1.1 alp8 specification`_ . Details are given below.

New Features / Tests Added
==========================

- Additional test to Planes exeption model.
- Added new tests in for Planes ABIs in Command suite.
- Added new tests for Planes memory mangement, Planes GIC and Planes timers modules.
- For more details on these tests details check `Planes Scenarios`_.


***********************
v1.1_PLANES_ALPHA_07_24
***********************

-  This release is aligned to `RMM v1.1 alp5 specification`_ . Details are given below.

New Features / Tests Added
==========================

- Added stage 2 permission indirection support in VAL.
- Added Planes leaf level ABI support.
- VAL infra updates to support Planes.
- Added Planes command scenarios to Command scenario document.
- Added Planes exception model tests. For details check `Planes Scenarios`_.

*******************
v1.1_DA_ALPHA_07_24
*******************

-  This release is aligned to `RMM v1.1 alp5 specification`_ . Details are given below.

New Features / Tests Added
==========================

- Added PCI enumeration code.
- Added PCI DOE driver.
- Added secure SPDM session test.
- Added DA commands scenario document.
- Added DA commands leaf-level ABIs support
- VAL infra updates

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

.. _RMM v1.0 rel0 specification: https://developer.arm.com/documentation/den0137/1-0rel0/?lang=en
.. _RMM Eac5 specification: https://developer.arm.com/documentation/den0137/1-0eac5/?lang=en
.. _RMM Eac2 specification: https://developer.arm.com/documentation/den0137/1-0eac2/?lang=en
.. _RMM Beta1 specification: https://developer.arm.com/documentation/den0137/1-0bet1/?lang=en
.. _RMM v1.1 alp5 specification: https://armh.sharepoint.com/:b:/r/sites/ts-atg/SystemTechnol
    ogy/projectwork/Security/Projects/Fenimore/Release/RMMArchSpec/Internal/1.1-alp5/DEN0137_1.1
    -alp5_rmm-arch_internal.pdf?csf=1&web=1&e=xvBiLU
.. _RMM v1.1 alp8 specification:  https://developer.arm.com/-/cdn-downloads/permalink/PDF/
    Architectures/DEN0137_1.1-alp8_rmm-arch_external.pdf
.. _RMM v1.1 alp12 specification:  https://developer.arm.com/-/cdn-downloads/permalink/
    Architectures/Armv9/DEN0137_1.1-alp12.zip
.. _PMU and Debug Scenarios: ./pmu_debug.md
.. _Attestation and Measurement Scenarios: ./attestation_measurement_scenarios.md
.. _Planes Scenarios: ./planes_scenarios.rst
.. _MEC LFA Scenarios: ./mec_lfa.md
