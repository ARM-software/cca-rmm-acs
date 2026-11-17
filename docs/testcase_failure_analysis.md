# Test failure analysis document

This file contains list of failures and skips identified when testing the release on tgt_tfa_fvp target with TF-RMM and TF-A.<br />
The reason for each failing/skipping test is listed here in this file.<br />

**Note**

The example below uses the current local TF-RMM and TF-A revisions. The TF-RMM
changes not represented by the revision hash are supplied by the patch below.
The FVP run used a configured copy of the
[sample PCI hierarchy](../plat/targets/tgt_tfa_fvp/manifest/pci_hierarchy.json)
and the [sample measurement blocks](../plat/targets/tgt_tfa_fvp/manifest/measurements.json).
Set the hierarchy's certificate, private-key, and measurement paths to files
accessible to FVP.

## Example steps to run CCA-RMM-ACS tests with TF-RMM

```bash
# Set paths to the AArch64 toolchain, srec_cat, FVP model, and PCI hierarchy.
export CROSS_COMPILE=/path/to/toolchain/bin/aarch64-none-elf-
export SREC_CAT=/path/to/srec_cat
export FVP_MODEL=/path/to/FVP_Base_RevC-2xAEMvA
export PCI_HIERARCHY=/path/to/configured/pci_hierarchy.json

mkdir -p cca-rmm-work
cd cca-rmm-work
WORK_DIR=$(pwd)

# Download and build CCA-RMM-ACS for the FVP target.
git clone https://github.com/ARM-software/cca-rmm-acs.git
cd cca-rmm-acs
git submodule update --init --recursive
cmake -S . -B build -G "Unix Makefiles" \
    -DCROSS_COMPILE="$CROSS_COMPILE" -DTARGET=tgt_tfa_fvp \
    -DTEST_COMBINE=ON -DSREC_CAT="$SREC_CAT"
cmake --build build -j16
cd ..

# Download and build TF-RMM at the revision used for this release.
git clone https://git.trustedfirmware.org/TF-RMM/tf-rmm.git
cd tf-rmm
git checkout 687eb2357a52bb21d5bf77f30dec758e3601cda3
git submodule update --init --recursive
git apply "$WORK_DIR/cca-rmm-acs/patch/0002-TF-RMM-implement-missing-v2.0-behavior.patch"
cmake -S . -B build -DRMM_CONFIG=fvp_defcfg -DRMM_ALLOC_TRACKING_DATA=OFF
cmake --build build -j16
cd ..

# Download and build TF-A with the TF-RMM image.
git clone https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git
cd trusted-firmware-a
git checkout d70b52dedc4eba74386098a9033eec1e9cb522fa
git submodule update --init --recursive
make CROSS_COMPILE="$CROSS_COMPILE" PLAT=fvp \
    ENABLE_RME=1 \
    FVP_HW_CONFIG_DTS=fdts/fvp-base-gicv3-psci-1t.dts DEBUG=1 \
    RMM="$WORK_DIR/tf-rmm/build/Release/rmm.img" \
    PRELOADED_BL33_BASE=0x88000000 all fip -j16
cd ..

# Run the combined ACS image with the FVP options from latest_fvp.sh.
"$FVP_MODEL" \
-C bp.flashloader0.fname=$WORK_DIR/trusted-firmware-a/build/fvp/debug/fip.bin \
-C bp.secureflashloader.fname=$WORK_DIR/trusted-firmware-a/build/fvp/debug/bl1.bin \
-C bp.refcounter.non_arch_start_at_default=1 \
-C bp.ve_sysregs.exit_on_shutdown=1 \
-C cache_state_modelled=1 \
-C cluster0.NUM_CORES=4 \
-C cluster0.PA_SIZE=48 \
-C cluster0.ecv_support_level=2 \
-C cluster0.gicv3.cpuintf-mmap-access-level=2 \
-C cluster0.gicv3.without-DS-support=1 \
-C cluster0.gicv4.mask-virtual-interrupt=1 \
-C cluster0.has_arm_v8-7=1 \
-C cluster0.has_branch_target_exception=1 \
-C cluster0.has_rme=1 \
-C cluster0.has_rndr=1 \
-C cluster0.has_amu=1 \
-C cluster0.has_v8_7_pmu_extension=2 \
-C cluster0.max_32bit_el=-1 \
-C cluster0.restriction_on_speculative_execution=2 \
-C cluster0.restriction_on_speculative_execution_aarch32=2 \
-C cluster1.NUM_CORES=4 \
-C cluster1.PA_SIZE=48 \
-C cluster1.ecv_support_level=2 \
-C cluster1.gicv3.cpuintf-mmap-access-level=2 \
-C cluster1.gicv3.without-DS-support=1 \
-C cluster1.gicv4.mask-virtual-interrupt=1 \
-C cluster1.has_arm_v8-7=1 \
-C cluster1.has_branch_target_exception=1 \
-C cluster1.has_rme=1 \
-C cluster1.has_rndr=1 \
-C cluster1.has_amu=1 \
-C cluster1.has_v8_7_pmu_extension=2 \
-C cluster1.max_32bit_el=-1 \
-C cluster1.restriction_on_speculative_execution=2 \
-C cluster1.restriction_on_speculative_execution_aarch32=2 \
-C pci.pci_smmuv3.mmu.SMMU_AIDR=3 \
-C pci.pci_smmuv3.mmu.SMMU_IDR0=0x498F36BB \
-C pci.pci_smmuv3.mmu.SMMU_IDR1=0xCE73D10 \
-C pci.pci_smmuv3.mmu.SMMU_IDR3=0x773C \
-C pci.pci_smmuv3.mmu.SMMU_IDR5=0x475 \
-C pci.pci_smmuv3.mmu.SMMU_ROOT_IDR0=0x80000F \
-C pci.pci_smmuv3.mmu.SMMU_ROOT_IIDR=0x43B \
-C pci.pci_smmuv3.mmu.SMMU_R_IDR0=0x1012400 \
-C pci.pci_smmuv3.mmu.SMMU_R_IDR3=0x10000 \
-C pci.pci_smmuv3.mmu.SMMU_S_IDR0=0x1000000 \
-C pci.pci_smmuv3.mmu.SMMU_S_IDR1=0xA0000002 \
-C pci.pci_smmuv3.mmu.SMMU_S_IDR2=0 \
-C pci.pci_smmuv3.mmu.SMMU_S_IDR3=0 \
-C pci.pci_smmuv3.mmu.root_register_page_offset=0x20000 \
-C pci.pci_smmuv3.mmu.size_of_gpttlb=1 \
-C pci.pci_smmuv3.mmu.size_of_tlb=1 \
-C pci.pci_smmuv3.mmu.ish_is_osh=1 \
-C bp.pl011_uart2.out_file=- \
-C bp.pl011_uart3.out_file=- \
-C pctl.startup=0.0.0.0 \
-Q 1000 \
-C bp.pl011_uart0.uart_enable=1 \
-C bp.pl011_uart1.uart_enable=1 \
-C bp.pl011_uart2.uart_enable=1 \
-C bp.pl011_uart2.shutdown_tag="HOST : Entering standby.." \
-C cluster0.ish_is_osh=1 \
-C cluster1.ish_is_osh=1 \
--data cluster0.cpu0=$WORK_DIR/cca-rmm-acs/build/output/acs_non_secure.bin@0x88000000 \
-C pci.enable_device_id_checks=0 \
-C pci.hierarchy_file_name="$PCI_HIERARCHY" | tee "$WORK_DIR/acs_log"
```


## List of skipped tests

| Test | Skip description                                                                |
|------|---------------------------------------------------------------------------------|
| cmd_secure_test | Platform limitation |
| exception_rec_exit_fiq | Platform limitation |
| mm_gpf_exception | Platform limitation |
| mm_gpt_l1_create_sro_donate | Platform limitation |
| mm_gpt_l1_destroy_sro_reclaim | Platform limitation |
| da_integrated_pdev_rid_range | Platform limitation |
| psmmu_interrupt_handling | Platform limitation |
| dpt_l0_create_alignment | Platform limitation |

## List of Failed tests

The commented-out tests below were not executed and have no runtime failure result.

| Test | Fail description                                                                |
|------|---------------------------------------------------------------------------------|
| cmd_gpt_l1_create | Platform limitation |
| cmd_gpt_l1_destroy | Platform limitation |
| cmd_pdev_stream_key_purge | Platform limitation |
| da_pdev_stream_ncoh_addr_range | Platform limitation |
| da_pdev_stream_addr_overlap | Platform limitation |
| da_pdev_stream_ide_sid_unique | Platform limitation |
| da_pdev_stream_rid_overlap | Platform limitation |
| da_pdev_single_active_transaction | Platform limitation |
| rhi_da_vdev_p2p_unbind | Platform limitation |


## License

Arm CCA RMM ACS is distributed under BSD-3-Clause License.

--------------

*Copyright (c) 2023-2026, Arm Limited or its affliates. All rights reserved.*
