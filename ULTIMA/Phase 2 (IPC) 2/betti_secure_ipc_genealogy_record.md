# BETTI Secure IPC Genealogy Record

## Purpose

Record the parent-child relationships introduced by the second-pass BETTI integration without creating a duplicate IPC subsystem.

## Root

`phase2/ipc/base_mailbox_surface`

## Runtime Genealogy

1. `ipc.cpp::Message_Send`
   -> `ipc.cpp::Secure_Message_Send`
   Reason: secure ACL + encryption extension on the existing mailbox path

2. `ipc.cpp::Message_Receive`
   -> `ipc.cpp::Secure_Message_Receive`
   Reason: authorized decrypt extension on the existing receive path

3. `Phase2_main.cpp`
   -> `Phase 2 secure UI storyline`
   Reason: the demo now shows Security/Privacy/Encryption from the first frame to the last frame

4. `phase2_smoke_test.cpp`
   -> `phase2_smoke_betti`
   Reason: BETTI sidecar witness emission uses the real secure smoke scenario

## Artifact Genealogy

1. `phase2_smoke_betti`
   -> `betti_phase2_secure_ipc_snapshot_witness.jcs`

2. `phase2_smoke_betti`
   -> `betti_phase2_secure_ipc_commit_witness.jcs`

3. `betti_phase2_secure_ipc_snapshot_witness.jcs`
   -> `betti_phase2_secure_ipc_snapshot_witness_verify.json`

4. `betti_phase2_secure_ipc_commit_witness.jcs`
   -> `betti_phase2_secure_ipc_commit_witness_verify.json`

5. `phase2 secure mailbox state`
   -> `betti_phase2_secure_ipc_live_profile.json`
   -> `betti_phase2_secure_ipc_shadow_cells.json`
   -> `betti_phase2_secure_ipc_ichorir_morphir_lowering.json`
   -> `betti_phase2_secure_ipc_trusted_path_gate.json`
   -> `betti_phase2_secure_ipc_ratification_manifest.json`

6. `BETTI sidecar artifacts`
   -> `betti_phase2_secure_ipc_artifact_fabric.json`
   -> `betti_phase2_secure_ipc_genealogy.json`

## No-Duplicate-Files Justification

- The secure IPC runtime remains in the original Phase 2 files.
- BETTI additions were limited to:
  - optional sidecar build surfaces
  - generated evidence artifacts
  - documentation
- No second IPC stack or replacement message layer was introduced.

## Result

The second pass added lineage and evidence closure around the existing secure IPC code instead of forking the project into a separate “BETTI version” of Phase 2.
