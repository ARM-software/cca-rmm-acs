
# Arm CCA RMM ACS Porting Guide
-----------------------------

## Introduction
The architecture test suite contains a *Platform Abstraction Layer* (PAL) which abstracts platform-specific information from the tests. You must implement and port the PAL interface functions available in **plat/** directory to your target platform. Create and update the target configuration file to match the details of this target platform.

This document provides the porting steps and the list of PAL APIs.

## Steps to add new target (or Platform)
  1. Create new directories in **plat/targets/<platform_name>**. For reference, see the existing platform tgt_tfa_fvp directory.
  2. Execute `cp -rf plat/targets/tgt_tfa_fvp/* plat/targets/<platform_name>/`
  3. Update **plat/targets/<platform_name>/pal_config_def.h** with your target platform details.
  4. Refer section **List of PAL APIs** file or **plat/common/inc/pal_interfaces.h** to view the list of PAL APIs that must be ported for your target platform. The reference implementation for these APIs are available in **plat/targets/tgt_tfa_fvp/src/pal_\*\.c** files. You can reuse the code if it works for your platform. Otherwise, port them for your platform-specific peripherals.
  5. Update **plat/targets/<platform_name>/pal.cmake** appropriately to select the correct instances of PAL files for compilation.

## Peripheral requirement
- One Non-secure UART must be available for printing test messages on to console.
- One Non-secure Watchdog timer must be available to help system recovery from any fatal error conditions
- 64KB Non-secure Non-volatile memory must be available for preserving test data over watchdog timer reset. Each byte of this region must be initialised to 0xFF at power on reset.

## RMM v2.0 tracking requirements

Update the tracking definitions in the target's `pal_config_def.h`:

1. Set `PLATFORM_RMM_RMI_GRANULE_SIZE` and
   `PLATFORM_RMM_TRACKING_REGION_SIZE` to the RMM configuration used by the
   platform. ACS currently supports 4 KB RMI granules. For this granule size,
   region-size encoding 0 selects 1 GB and encoding 1 selects 2 MB. The FVP
   uses 2 MB regions by default.
2. Set `PLATFORM_RMM_TRACKING_REGION_BASE` to the start of the tracking region
   containing the ACS image. It must equal the image base rounded down to the
   selected tracking-region size. The ACS image and memory pool must be in
   conventional DRAM. ACS fine-tracks every region they occupy, preparing the
   memory-pool regions before the image regions.
3. Reserve `PLATFORM_RMM_TRACKING_SETUP_POOL_BASE` and
   `PLATFORM_RMM_TRACKING_SETUP_POOL_SIZE` separately from the per-test heap.
   This pool must be within the ACS DRAM range and large enough for address
   lists and granules donated during tracking setup. The FVP reserves 10 MB.
   ACS does not reset this allocator when the RMM is already active because
   the RMM may still own previously donated granules. Other SRO completion
   uses the normal test heap.
4. Set `PLATFORM_TRACKING_TEST_REGION_BASE` to a separate conventional DRAM
   region for tracking failure-condition tests. It must be aligned to the
   selected tracking-region size, backed by DRAM for the full region, and
   outside the ACS image and memory pool. Tests prepare and restore its
   tracking state as needed.
5. Define `PLATFORM_RMM_DEVICE_TRACKING_RANGES` as a comma-separated list of
   `{ base, exclusive_top, RMI_MEM_CATEGORY_* }` entries. On platforms with
   device assignment, cover the device memory used by the PCI hierarchy. The
   FVP includes the 32-bit non-coherent PCIe range and the first 1 GB of its
   64-bit range; extend the latter if the hierarchy uses addresses beyond it.

At startup, ACS checks 4 KB granule support with `RMI_FEATURES(1)` and reads
the RMM configuration. If the RMM is not active, ACS sets the requested
configuration when necessary, activates the RMM, and sets the ACS DRAM and
configured device regions to FINE tracking. If the RMM is already active, its
configuration must match the platform definitions and these regions must
already be FINE. A category mismatch or RESERVED region is a setup error.

## Others

The sample [FVP PCI hierarchy](../plat/targets/tgt_tfa_fvp/manifest/pci_hierarchy.json)
is the layout used to validate the DA Foundation tests. Replace its placeholder
paths and asymmetric-algorithm value to match your FVP setup.

The sample [SPDM measurement blocks](../plat/targets/tgt_tfa_fvp/manifest/measurements.json)
can be used with this hierarchy. Set each endpoint's
`endpoint.measurement_block_filename` to the path of the measurement file
accessible to FVP.

## List of PAL APIs

Since the test suite is agnostic to various system targets, you must port the following PAL APIs before building the tests. <br />

| No | Prototype                                 | Description                                  | Parameters                            |
|----|-----------------------------------------------------------------------------------|-----------------------------------------------------------------------------------|----------------------------------------------------------|
| 1 |  uint32_t pal_printf(const char *msg, uint64_t data1, uint64_t data2); |  This function prints the given string and data onto the uart| msg: Input String <br /> data1: Value for first format specifier <br /> data2: Value for second format specifier <br /> Return: SUCCESS(0)/FAILURE(any positive number) |
| 2 | uint32_t pal_nvm_write(uint32_t offset, void *buffer, size_t size); | Writes into given non-volatile address | Input: offset: Offset into nvmem <br />buffer: Pointer to source address<br /> size: Number of bytes<br /> Return: SUCCESS/FAILURE|
| 3 | uint32_t pal_nvm_read(uint32_t offset, void *buffer, size_t size); | Reads from given non-volatile address | Input: offset: Offset into nvmem <br />buffer: Pointer to source address<br /> size: Number of bytes<br /> Return: SUCCESS/FAILURE|
| 4 | uint32_t pal_watchdog_enable(void); | Initializes and enable the hardware watchdog timer | Input: void <br /> Return: SUCCESS/FAILURE|
| 5 | uint32_t pal_watchdog_disable(void); | Disable the hardware watchdog timer | Input: void <br /> Return: SUCCESS/FAILURE|
| 6 | uint32_t pal_terminate_simulation(void);| Terminates the simulation at the end of all tests completion.| Input: void <br /> Return:SUCCESS/FAILURE|
| 7 | uint32_t pal_get_cpu_count(void);|Returns number of cpus in the system. | Input: Void <br /> Return: Number of cpus|
| 8 | uint64_t* pal_get_phy_mpidr_list_base(void); | Return base of phy_mpidr_array | Input: Void <br /> Return:Base of phy_mpidr_array|
| 9 | uint32_t pal_register_acs_service(void); | Registers ACS secure service with EL3/EL2 software  | Input:Void <br /> Return: SUCCESS(0)/FAILURE |
| 10 | uint32_t pal_wait_for_sync_call(void); | Send control back to normal world during boot phase and and waits for message from normal world | Input:Void <br /> Return: SUCCESS(0)/FAILURE |
| 11 | uint32_t pal_sync_resp_call_to_host(void); | Send control back to normal world during test execution phase and waits for message from normal world | Input:Void <br /> Return: SUCCESS(0)/FAILURE |
| 12 | uint32_t pal_sync_req_call_to_secure(void); | Normal sends message to secure world and wait for the respond from secure world | Input:Void <br /> Return: SUCCESS(0)/FAILURE |
| 13 | void pal_ns_wdog_enable(uint32_t ms); | Initializes and enable the non-secure watchdog timer | Input: ms: timeout <br /> Return: void|
| 14 | void pal_ns_wdog_disable(void); | Disable the non-secure watchdog timer | Input: void <br /> Return: void|
| 15 | uint32_t pal_twdog_enable(uint32_t ms); | Initializes and enable the hardware trusted watchdog timer | Input: ms: timeout <br /> Return: SUCCESS/FAILURE|
| 16 | uint32_t pal_twdog_disable(void); | Disable the hardware trusted watchdog timer | Input: void <br /> Return: SUCCESS/FAILURE|
| 17 | void pal_irq_setup(void); | Initialise the irq vector table. | Input: void <br /> Return: void|
| 18 | int pal_irq_handler_dispatcher(void); | Generic handler called upon reception of an IRQ. This function acknowledges the interrupt, calls the user-defined handler. If one has been registered then marks the processing of the interrupt as complete. | Input: void <br /> return: SUCCESS/FAILURE |
| 19 | void pal_irq_enable(int irq_num, uint8_t irq_priority); | Enable interrupt #irq_num for the calling core. | Input: irq_num: irq number. <br /> irq_priority:  irq priority value. <br /> return: void |
| 20 | void pal_configure_secure_irq_enable(unsigned int irq_num); |Enable secure interrupt #irq_num for the calling core. | Input: irq_num: irq number. <br /> return: void |
| 21 | void pal_irq_disable(int irq_num); | Disable interrupt #irq_num for the calling core. | Input: irq_num: irq number. <br /> return: void |
| 22 | int pal_irq_register_handler(int irq_num, void *irq_handler); | Register an interrupt handler for a given interrupt number. This will fail if there is already an interrupt handler registered for the same interrupt.| Input: irq_num: irq number. <br /> irq_handler:  irq handler pointer <br /> return: Return 0 on success, a negative value otherwise |
| 23 | int pal_irq_unregister_handler(int irq_num); | Unregister an interrupt handler for a given interrupt number. This will fail if there is no an interrupt handler registered for the same interrupt.| Input: irq_num: irq number. <br /> return: Return 0 on success, a negative value otherwise |
| 24 | void pal_send_sgi(int sgi_id, unsigned int core_pos); | Send an SGI to a given core. | Input: sgi_id: SGI interrupt number. <br />core_pos:  CPU core number. <br /> return: void |
| 25 | uint32_t pal_get_irq_num(void); | Get IRQ number | Input: void <br /> return: irq_num: irq number. |
| 26 | void pal_gic_end_of_intr(unsigned int irq_num); | End of the interrupt | Input: irq number <br /> return: void. |
| 27 | uint32_t pal_verify_signature(uint64_t *token); | function interface for verifying the signature of the provided token | Input: token recived from the platform <br /> return: true/false. |

## License

Arm RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023, 2026, Arm Limited or its affliates. All rights reserved.*
