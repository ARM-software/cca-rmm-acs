
# Architecture Compliance Suite for Realm Management Monitor

## Realm Management Monitor

The Realm Management Monitor is a system component which forms part of a system which implements the Arm Confidential Compute Architecture (Arm CCA). In other words, RMM is the Realm world firmware that is used to manage the execution of the Realm VMs and their interaction with the hypervisor in Normal world.

The responsibilities of the RMM are to:
- Provide services that allow the Host to create, populate, execute and destroy Realms.
- Provide services that allow the initial configuration and contents of a Realm to be attested.
- Protect the confidentiality and integrity of Realm state during the lifetime of the Realm.
- Protect the confidentiality of Realm state during and following destruction of the Realm.

The RMM exposes the following interfaces, which are accessed via SMC instructions, to the Host:
- The Realm Management Interface (RMI), which provides services for the creation, population, execution and destruction of Realms.

The RMM exposes the following interfaces, which are accessed via SMC instructions, to the Realms:
- The Realm Services Interface (RSI), which provides services used to manage resources allocated to the Realm, and to request an attestation report.
- The Power State Coordination Interface (PSCI), which provides services used to control power states of VPEs within a Realm.

For more information on RMM refer [RMM Specification](https://developer.arm.com/documentation/den0137/1-0eac2/?lang=en)

## Architecture Compliance Suite

The Architecture Compliance Suite (ACS) contains a set of functional tests, demonstrating the invariant behaviors that are specified in the architecture specification. It is used to ensure architecture compliance of the implementations to Realm Management Monitor specification.

This suite contains self-checking, and portable C and assembly based tests with directed stimulus. These tests are available as open source. The tests and the corresponding abstraction layers are available with a BSD-3-Clause License allowing for external contribution.
This suite is not a substitute for design verification.

For more information on Architecture Compliance Suite see [Validation Methodology](<./docs/Arm CCA RMM Architecture Compliance Suite Validation Methodology.pdf>) document.

## This release
- Release Version - 0.8
- Code Quality: Beta - ACS is being developed, please use this opportunity to ameliorate.
- The tests are written for Arm RMM 1.0-EAC2 specification version.
- For information about the test coverage scenarios that are implemented in the current release of ACS and the scenarios that are planned for the future releases, see [docs](./docs/).
- The [Change log](./docs/change-log.rst) has details of the features implemented by this version of CCA-RMM-ACS.

**Note:** The current release has been tested on tgt_tfa_fvp reference platforms with GNUARM Compiler.

## GitHub branch
- To pick up the release version of the code, checkout the release branch.
- To get the latest version of the code with bug fixes and new features, use the main branch.

## Porting steps

For more information on porting steps to run ACS for the target platform, see [Arm CCA-RMM-ACS Porting Guide](./docs/porting_guide.md) document.

## Building ACS
### Software requirements

The following tools are required to build the ACS: <br />
- Host operating system: Ubuntu 18.04, RHEL 7
- Git 2.17 or later.
- CMake 3.19 or later version from https://cmake.org/download/.
- srecord-1.64 : srec_cat utility to concatenate binaries
- Python 3.7.1 or later version
- Cross-compiler toolchain supporting AArch64 target: GCC >= 10.2-2020.11 can be downloaded from [Arm Developer website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads)

### Download source

To download the main branch of the repository, type the following command: <br />
~~~
git clone https://github.com/ARM-software/cca-rmm-acs.git
~~~



### Build steps

To build the ACS for your target platform, perform the following steps:<br />

```
cd rmm-acs
mkdir build
cd build
cmake ../ -G"<generator_name>" -DCROSS_COMPILE=<path-to-aarch64-gcc>/bin/aarch64-none-elf- -DTARGET=<platform_name>  -DTEST_COMBINE=ON -DSREC_CAT=<path-to-srec_cat>/bin/srec_cat
make
```

<br />Options information:<br />
- -G"<generator_name>" : "Unix Makefiles" to generate Makefiles for Linux and Cygwin. "MinGW Makefiles" to generate Makefiles for cmd.exe on Windows. Currently supported only Unix Makefiles target.
- -DCROSS_COMPILE=<path_to_aarch64_gcc> Set the cross-compiler toolchain supporting AArch64 target. For example, -DCROSS_COMPILE=/bin/aarch64-none-elf-
- -DTARGET=<platform_name> is the same as the name of the target-specific directory created in the plat/targets/ directory. The default value is -DTARGET=tgt_tfa_fvp.
- -DCC=<path_to_armclang_or_clang_binary> To compile ACS using clang or armclang cross compiler toolchain. The default compilation is with aarch64-gcc.
- -DSUITE=<suite_name> is the sub test suite name specified in test/ directory. The default value is -DSUITE=all
- -DTEST_COMBINE=<ON/OFF> To generate single binary for all tests.
- -DSREC_CAT=<path_to_srec_cat> To concatenate acs_host.bin and acs_realm.bin into acs_non_secure.bin binaries.
- -DVERBOSE=<verbose_level>. Print verbosity level. Supported print levels are 1(INFO & above), 2(DEBUG & above), 3(TEST & above), 4(WARN & ERROR) and 5(ERROR). Default value is 3.
- -DCMAKE_BUILD_TYPE=<build_type>: Chooses between a debug and release build. It can take either release or debug as values. The default value is release.
- -DSUITE_TEST_RANGE="<test_start_name>;<test_end_name>" is to select range of tests for build. All tests under -DSUITE are considered by default if not specified.
- -RMM_ACS_TARGET_QCBOR=<path_for_pre_fetched_cbor_folder> this is option used where no network  connectivity is possible during the build.
- -DSECURE_TEST_ENABLE=<value_to_enable_secure_test> Enable secure test macro defination and it will run secure test in regression. Valid value is 1. By default this macro will not define and secure test will not run in regression.

*To compile tests for tgt_tfa_fvp platform*:<br />
```
cd rmm-acs
mkdir build
cd build
cmake ../ -G"Unix Makefiles" -DCROSS_COMPILE=<path-to-aarch64-gcc>/bin/aarch64-none-elf- -DTARGET=tgt_tfa_fvp -DTEST_COMBINE=ON -DSREC_CAT=<path_to_srec_cat>
make
```

### Build output
The ACS build generates the binaries as follow :<br />
- build/output/acs_host.bin
- build/output/acs_realm.bin
- build/output/acs_non_secure.bin
- build/output/acs_secure.bin

For information on integrating the binaries into the target platform, test suite execution flow, analysing the test results and more, see [Validation Methodology](<./docs/Arm CCA RMM Architecture Compliance Suite Validation Methodology.pdf>) document.

## Security implication
The ACS tests may run at higher privilege level. An attacker can utilize these tests to elevate privilege which can potentially reveal the platform secure attests. To prevent such security vulnerabilities into the production system, it is recommended that CCA-RMM-ACS is run on development platforms. If it is run on production system, ensure that the system is scrubbed after running the tests.

## License

Arm CCA-RMM-ACS is distributed under BSD-3-Clause License.


## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to support-cca-rmm-acs@arm.com with details.
 - Arm licensees can contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests.

--------------

*Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.*
