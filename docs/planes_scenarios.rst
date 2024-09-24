.. Copyright [C] 2024, Arm Limited or its affiliates. All rights reserved.
      SPDX-License-Identifier: BSD-3-Clause

*************************************
Arm RMM ACS Planes Testcase checklist
*************************************

.. list-table::

  * - Test name
    - Test Assertion
    - Test Approach
    - Validated by ACS

  * - planes_plane_exit_smc_hvc
    - An exception due to execution of SMC/HVC instruction in Pn causes a plane exit due to
      Synchronous Exception taken to P0.
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Execute SMC/HVC instruction
      #. On Plane exit, all of the following are true.

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2 = SMC/HVC execution in Aarch64 state.
    - Yes

  * - planes_plane_exit_wfe
    - An exception due to execution ofi trapped WFE instruction in Pn causes a plane exit due
      to Synchronous Exception taken to P0
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA.
      #. Enter P1 with RsiPlaneEnterFlags.trap_wfx = RSI_TRAP.
      #. Execute WFE instruction
      #. On Plane exit, all of the following are true

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2.ec = 0x1 (Trapped WFx instruction.
       * plane_exit.esr_el2.ISS.TI = WFE Instruction
    - Yes

  * - planes_plane_exit_wfi
    - An exception due to execution ofi trapped WFI instruction in Pn causes a plane exit due
      to Synchronous Exception taken to P0
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA.
      #. Enter P1 with RsiPlaneEnterFlags.trap_wfx = RSI_TRAP.
      #. Execute WFI instruction
      #. On Plane exit, all of the following are true

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2.ec = 0x1 (Trapped WFx instruction.
       * plane_exit.esr_el2.ISS.TI = WFI Instruction
    - Yes

  * - planes_plane_exit_ia_unprtoected
    - An instruction fetch by Pn to an unprotected IPA causes a plane exit due to Synchronous
      Exception taken to P0
    - #. Pick a Unprotected IPA, Map the IPA at stage 2 with RW permission.
      #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Flat - Map an IPA belonging to P1 as CODE (RO + X) in stage 1 (since MMU is enabled)
      #. Perform a Instruction fetch from above IPA
      #. On Plane exit, all of the following are true

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2 = Instruction abort
       *  plane_exit.far_el2, plane_exit.hpfar_el2 = IPA
    - Yes

  * - planes_plane_exit_da_ia_ripas_empty
    - A data access or an instruction fetch by Pn to a protected IPA whose RIPAS is EMPTY causes
      a plane exit due to Synchronous Exception taken to P0.
    - #. Pick a protected IPA, Map the IPA at stage 2.
      #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Flat - Map an IPA belonging to P1 as CODE (RO + X) or DATA (RW) accordingly in stage 1
         (since MMU is enabled)
      #. Perform data access and Instruction fetch from above IPA
      #. On Plane exit, all of the following are true

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2 = Data abort/Instruction abort
       * plane_exit.far_el2, plane_exit.hpfar_el2 = IPA
    - Yes

  * - planes_plane_exit_da_ia_permission
    - A data access or an instruction fetch by Pn to a protected IPA belonging to P0 or violates
      the stage 2 permissions set by P0 causes a plane exit due to Synchronous Exception taken
      to P0.
    - #. Pick a protected IPA belonging to P0 .
      #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Flat - Map an IPA belonging to P0 as CODE (RO + X) or DATA (RW) accordingly in stage 1
         (since MMU is enabled)
      #. Perform data access and Instruction fetch from above IPA
      #. On Plane exit, all of the following are true

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2 = Data abort/Instruction abort
       * plane_exit.far_el2, plane_exit.hpfar_el2 = IPA
    - Yes

  * - planes_plane_exit_host_call
    - If plane_enter.flags.trap_hc == RSI_TRAP then execution by Pn of RSI_HOST_CALL results in a
      Plane exit due to synchronous exception
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA.
      #. Enter P1 with trap_hc == RSI_TRAP
      #. On Plane exit, all of the following are true

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr_el2 = SMC Execution
       * plane_exit.gprs[0] = RSI_HOST_CALL
    - Yes

  * - planes_rec_exit_da_ia_hipas_unassigned_ripas_ram
    - A data access or an instruction fetch by Pn to a protected IPA whose HIPAS is DESTORYED or
      HIPAS is UNASSIGNED and RIPAS is not EMPTY causes a REC exit due to Synchronous Exception
      taken to host
    - #. Pick a protected IPA whose (HIPAS, RIPAS ) is (UNASSIGNED, RAM) .
      #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Flat - Map the IPA as CODE (RO + X) or DATA (RW) accordingly in stage 1 (since MMU
         is enabled)
      #. Perform data access and Instruction fetch from above IPA
      #. On REC exit, all of the following are true

       * rec_exit.exit_reason = RMI_EXIT_SYNC
       * rec_exit.esr_el2 = Data abort/Instruction abort
       * rec_exit.far_el2, rec_exit.hpfar_el2 = IPA
    - Yes

  * - planes_rec_exit_ripas_destroyed
    - A data access or an instruction fetch by Pn to a protected IPA whose HIPAS is DESTORYED or
      HIPAS is UNASSIGNED and RIPAS is not EMPTY causes a REC exit due to Synchronous Exception
      taken to host
    - #. Pick a protected IPA whose (HIPAS, RIPAS ) is (ANY, DESTROYED).
      #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Flat - Map the IPA as CODE (RO + X) or DATA (RW) accordingly in stage 1 (since MMU
         is enabled)
      #. Perform data access and Instruction fetch from above IPA
      #. On REC exit, all of the following are true

       * rec_exit.exit_reason = RMI_EXIT_SYNC
       * rec_exit.esr_el2 = Data abort/Instruction abort
       * rec_exit.far_el2, rec_exit.hpfar_el2 = IPA
    - Yes

  * - planes_rec_exit_da_hipas_unassigned_ns
    - A data access by Pn to a Unprotected IPA whose HIPAS is UNASSIGNED_NS causes a REC exit
      due to Synchronous Exception taken to host
    - #. Pick a Uprotected IPA whose HIPAS is UNASSIGNED_NS
      #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Flat - Map the IPA as CODE (RO + X) in stage 1 (since MMU is enabled)
      #. Perform data access and Instruction fetch from above IPA
      #. On REC exit, all of the following are true

       * rec_exit.exit_reason = RMI_EXIT_SYNC
       * rec_exit.esr_el2 = Data abort/Instruction abort
       * rec_exit.far_el2, rec_exit.hpfar_el2 = IPA
    - Yes

  * - planes_rec_exit_irq
    - An exception due to IRQ while executing in Pn causes a REC exit due to asynchronous
      Exception taken to host
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Generate IRQ
      #. On REC exit, all of the following are true

       * rec_exit.exit_reason = RMI_EXIT_IRQ
       * rec_exit.esr_el2 = 0
    - No

  * - planes_rec_exit_host_call
    - If plane_enter.flags.trap_hc == RSI_NO_TRAP then execution by Pn of RSI_HOST_CALL results
      in a REC exit due to Host call
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Enter P1 with trap_hc == RSI_NO_TRAP
      #. Execute HOST_CALL
      #. On REC exit, all of the following are true

       * rec_exit.exit_reason = RMI_EXIT_HOST_CALL
       * rec_exit.plane = P1 index
       * rec_exit.esr_el2 = 0
    - Yes

  * - planes_s2ap_protected
    - At Realm Activation,
       * All Protected IPA to use Overlay index 0
       * Overlay index 0 has Permission value RW + upX for P0 and No access to all other planes
    - #. Configure Realm with one auxiliary plane
      #. Prepare a granule(CODE) with following contents

        .. code-block:: C

          LDR x1, [ x0 ]
          HVC

      #. Enter Realm, P0 to configure permissions RO+upX to granule CODE.
      #. Enter P1 with PC = CODE, gprs.x0 = Any other page belonging to P1
      #. Check for

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr = Data Abort due to permission fault.
    -

  * - planes_s2ap_unprotected
    - At Realm Activation,
       * All Protected IPA to use Overlay index 0
       * Overlay index 0 has Permission value RW + upX for P0 and No access to all other planes
    - #. Pick an Unprotected IPA, Map the IPA at stage 2 with RW permission.
      #. Configure Realm with one auxiliary plane.
      #. Enter Realm, P0 to set necessary Permissions for P1's protected IPA.
      #. Validate P1 has RW permissions for the above Unprotected IPA.
      #. Perform a instruction fetch from Unprotected IPA → Plane exit due to Permission fault
         Check for

       * plane_exit.exit_reason = RSI_EXIT_SYNC
       * plane_exit.esr = Instruction due to permission fault.
    -

  * - planes_s2ap_locking
    - At Realm Activation,
       * Permission overlay index 0 is LOCKED, index 1-14 are unlocked.
       * Overlay Permission index once used with RSI_MEM_SET_PERM_INDEX will be LOCKED for
         further use for RSI_MEM_SET_PERM_VALUE
    - #. Configure Realm with one auxiliary plane, Enter realm.
      #. Execute RSI_MEM_SET_PERM_VALUE for Overlay index 0 ---> RSI_ERROR_INPUT
      #. Execute RSI_MEM_SET_PERM_VALUE for Overlay index 1 ---> RSI_SUCCESS
      #. Execute RSI_MEM_SET_PERM_INDEX for P0 with Overlay index 1.
      #. Execute RSI_MEM_SET_PERM_VALUE for Overlay index 1 ---> RSI_ERROR_INPUT
    - No

  * - planes_s2ap_enforced_by_p0
    - To validate permissions set by P0
    - #. Configure Realm with one auxiliary plane
      #. Prepare code granules CODE1, CODE2, CODE3, CODE4 as shown in `Table 1 <table-1_>`__ below.
      #. Enter Realm, P0 to configure permissions to code and data granuels as shown
         in `Table 2 <table-2_>`__ below..
      #. Test for permissions with the all the combinations of inputs to RSI_PLANE_ENTER as
         listed in `Table 3 <table-3_>`__ below and check for respective results.
    - No

  * - planes_rec_entry_no_virt_int
    - On REC entry, if the values of enter.gicv3_lrs describe one or more Pending interrupts and the
      most recent REC exit was from a Plane which is not the GIC owner then control returns to P0.
      This results in a Plane exit due to synchronous exception.
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1 with P0 as GIC
         owner.
      #. Trigger a REC exit (any reason : eg access to HIPAS = UNASSIGNED)
      #. Call RMI_REC_ENTER with no virtual interrupt pending.

         * Check that control passes to Pn

    - No

  * - planes_rec_entry_p0_owner_virt_int
    - On REC entry, if the values of enter.gicv3_lrs describe one or more Pending interrupts and the
      most recent REC exit was from a Plane which is not the GIC owner then control returns to P0.
      This results in a Plane exit due to synchronous exception.

    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1 with P0 as GIC
         owner.
      #. Trigger a REC exit (any reason : eg access to HIPAS = UNASSIGNED)
      #. Call RMI_REC_ENTER with a virtual interrupt pending.

         * Check that control passes to P0
         * Check that Plane exit due to synchronous exception
         * Acknowledge the interrupt
    - No

  * - planes_rec_entry_pn_owner_virt_int
    - On REC entry, if the values of enter.gicv3_lrs describe one or more Pending interrupts and the
      most recent REC exit was from a Plane which is not the GIC owner then control returns to P0.
      This results in a Plane exit due to synchronous exception.
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1 with Pn as GIC
         owner.
      #. Trigger a REC exit (any reason : eg access to HIPAS = UNASSIGNED)
      #. Call RMI_REC_ENTER with a virtual interrupt pending

         * Check that control passes to Pn
         * Acknowledge the interrupt

    - No

  * - planes_rec_entry_maint_int
    - On REC entry, if the most recent REC exit was from Pn and the value of ICH_MISR_EL2 at the
      time of the REC exit was not zero then control returns to P0. This results in a Plane exit
      due to synchronous exception.
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1 with
         HCR_EL2.NPIE flag set.
      #. Trigger a REC exit (any reason : eg access to HIPAS = UNASSIGNED)
      #. Call RMI_REC_ENTER

         * Check that control passes to P0
         * Check that Plane exit due to synchronous exception
    - No

  * - planes_p0_gic_virt_pn
    - Check behavour when P0 is virtualising GIC for Pn
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1 wiht P0 as GIC
         owner.
      #. Trigger a REC exit (any reason : eg access to HIPAS = UNASSIGNED)
      #. Call RMI_REC_ENTER with a virtual interrupt pending

         * Check that control passes to P0
         * Check that Plane exit due to synchronous exception
         * Acknowledge the interrupt

      #. Call PLANE_ENTER with virtual interrupt pending

         * Check that control passes to P1
         * Acknowledge the interrupt

    - No

  * - planes_el1_timer_trig
    - On a change in the output of an EL1 timer which requires a Realm-observable change to the
      state of virtual interrupts, a REC exit
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1.
      #. Enable EL1 timer and wait for interrupt
      #. Upon REC exit

         * Check that rec exit due to IRQ
         * rec_exit->cntp_ctl is expected value
    - No

  * - planes_timer_state_rec_exit
    - On REC exit from Pn, for each of the EL1 virtual and physical timers, if any of the following
      is true then the timer state reported to the Host is Pn’s EL1 timer state:

       * The Pn timer is active and the P0 timer is not active.
       * Both Pn and P0 timers are active and the Pn timer deadline is earlier than the P0
         timer deadline
    - #. Configure Realm with one auxiliary plane and enter Realm.
      #. At P0 configure El1 timer compare value to V1 and enable it according to the
         `Table 4 <table-4_>`__ below.
      #. P0 to configure necessary permissions for P1's protected IPA and Enter P1 with
         trap_hc == RSI_NO_TRAP.
      #. P1 to Enable and set REL1 timer compare value to V2 and enable it according to
         `Table 4 <table-4_>`__ below
      #. Call RSI_HOST_CALL from Pn
      #. Upon REC exit check that rec_exit.cntv_cval is according to `Table 4 <table-4_>`__ below
      #. REC_ENTER again and repeat from Step 2 until all the configurations in the
         `Table 4 <table-4_>`__ below is covered
    - No



.. _table-1:

.. list-table:: Table 1 : Contents of Granueles

  * - Granule
    - Content
  * - CODE1
    - .. code-block:: C

        HVC

  * - CODE2
    - .. code-block:: C

        HVC

  * - CODE3
    - .. code-block:: C

        LDR x1, [x0]
        HVC

  * - CODE4
    - .. code-block:: C

        STR x1, [x0]
        HVC

.. _table-2:

.. table:: Table 2 : Permissions for Granules

  +------------+--------------------------------+
  | Granules   |        Permissions             |
  |            +---------------+----------------+
  |            |      P1       |      P2        |
  +============+===============+================+
  |  CODE1     |    No Access  |    No Access   |
  +------------+---------------+----------------+
  |  CODE2     |    RO + upX   |    RO + upX    |
  +------------+---------------+----------------+
  |  CODE3     |    RO + upX   |    RO + upX    |
  +------------+---------------+----------------+
  |  CODE4     |    RO + upX   |    RO + upX    |
  +------------+---------------+----------------+
  |  DATA1     |       RO      |    No Access   |
  +------------+---------------+----------------+
  |  DATA2     |       RW      |    No Access   |
  +------------+---------------+----------------+
  |  DATA3     |       WO      |    No Access   |
  +------------+---------------+----------------+
  |  DATA4     |    No Access  |       RO       |
  +------------+---------------+----------------+
  |  DATA5     |    No Access  |       RW       |
  +------------+---------------+----------------+
  |  DATA6     |    No Access  |       WO       |
  +------------+---------------+----------------+

.. _table-3:

.. table:: Table 3 : Test Pattern

  +-------------------------------+------------------------------------+
  |            Inputs             |       | Result of execution        |
  |                               |       | plane_exit.esr_el2.EC      |
  +----------------+--------------+------------------+-----------------+
  |    entry.PC    |   entry.x0   |  plane_index = 1 | plane_index = 2 |
  +================+==============+==================+=================+
  |     CODE1      |     --       |      I.A         |        I.A      |
  +----------------+--------------+------------------+-----------------+
  |     CODE2      |     --       |      HVC         |        HVC      |
  +----------------+--------------+------------------+-----------------+
  |     CODE3      |    DATA1     |      HVC         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA2     |      HVC         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA3     |      D.A         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA4     |      D.A         |        HVC      |
  |                +--------------+------------------+-----------------+
  |                |    DATA5     |      D.A         |        HVC      |
  |                +--------------+------------------+-----------------+
  |                |    DATA6     |      D.A         |        D.A      |
  +----------------+--------------+------------------+-----------------+
  |     CODE4      |    DATA1     |      D.A         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA2     |      HVC         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA3     |      HVC         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA4     |      D.A         |        D.A      |
  |                +--------------+------------------+-----------------+
  |                |    DATA5     |      D.A         |        HVC      |
  |                +--------------+------------------+-----------------+
  |                |    DATA6     |      D.A         |        HVC      |
  +----------------+--------------+------------------+-----------------+

.. _table-4:

.. list-table:: Table 4 : Timer configuration

  * - P0 enabled
    - P1 enabled
    - V1 (ms)
    - V2 (ms)
    - Expected palane index
  * - No
    - No
    - 5
    - 10
    - P0
  * - No
    - No
    - 10
    - 5
    - P0
  * - No
    - Yes
    - 5
    - 10
    - P1
  * - No
    - Yes
    - 10
    - 5
    - P1
  * - Yes
    - No
    - 5
    - 10
    - P0
  * - Yes
    - No
    - 10
    - 5
    - P0
  * - Yes
    - Yes
    - 5
    - 10
    - P0
  * - Yes
    - Yes
    - 10
    - 5
    - P1

