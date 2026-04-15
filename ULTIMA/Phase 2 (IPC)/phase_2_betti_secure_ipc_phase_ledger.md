# Phase 2 BETTI Secure IPC Phase Ledger

## Named Work Product

BETTI sidecar integration and evidence bundle for the ULTIMA Phase 2 secure IPC extra-credit path.

## Goals

1. Extend the existing Phase 2 mailbox implementation with BETTI-facing sidecar evidence instead of creating a duplicate IPC subsystem.
2. Keep the standard class build path working unchanged.
3. Add a maximum-credible BETTI surface for a class submission:
   - `Betti-FFI` runtime dependency
   - `bettictl` witness verification
   - IchorIR / MorphIR sidecar lowering artifact
   - sovereign artifact fabric record
   - genealogy record
   - ratification manifest and gap report
   - shadow-cell / live-profile artifacts
   - trusted-path gate record
   - host-side `betti_millicent` and `betti_solver_ecology` proof-path builds

## Governing References

- BETTI Discovery Contractor Phasebook: `docs/reference/discovery_contractor_phasebook.md`
- BETTI IMP line: repository execution doctrine and theorem-status discipline in `README.md`
- IchorIR constitution: `docs/ichor_ir.md`
- Genealogy binding: `doctrine/genealogy_binding/src`
- Ratification surfaces: `artifacts/ratification/`
- Promotion / trusted-path gate reference: `appendix_e/ms_13_4c_dependency_isolation_witness_chain/src`

## HPAC Rewrite

`phase2_secure_mailbox_demo`
-> `phase2_secure_mailbox_demo + betti_sidecar_witness + artifact_lineage + trusted_path_record`

Law:
- preserve the existing mailbox / semaphore runtime
- bind secure-message evidence to BETTI sidecar artifacts
- fail closed if BETTI sidecar emission is requested without the BETTI runtime surface

## Primary Files

- `Phase2_main.cpp`
- `phase2_smoke_test.cpp`
- `Makefile`
- `CMakeLists.txt`
- `BETTI_usage_phase2_secure_ipc.md`
- `phase_2_betti_secure_ipc_phase_ledger.md`
- `betti_secure_ipc_evidence_bundle.md`
- `betti_secure_ipc_genealogy_record.md`
- `betti_secure_ipc_ratification_gap_report.md`
- `betti_secure_ipc_constitutional_security_note.md`

## Generated BETTI Sidecar Artifacts

- `betti_phase2_secure_ipc_snapshot_witness.jcs`
- `betti_phase2_secure_ipc_commit_witness.jcs`
- `betti_phase2_secure_ipc_snapshot_witness_verify.json`
- `betti_phase2_secure_ipc_commit_witness_verify.json`
- `betti_phase2_secure_ipc_live_profile.json`
- `betti_phase2_secure_ipc_shadow_cells.json`
- `betti_phase2_secure_ipc_ichorir_morphir_lowering.json`
- `betti_phase2_secure_ipc_artifact_fabric.json`
- `betti_phase2_secure_ipc_genealogy.json`
- `betti_phase2_secure_ipc_trusted_path_gate.json`
- `betti_phase2_secure_ipc_ratification_manifest.json`

## Ordered Implementation Steps

1. Preserve the Phase 2 secure mailbox implementation already added to `ipc.cpp`, `ipc.h`, and `Message.h`.
2. Make the ncurses / terminal demo visibly security-focused from the first frame to the last frame.
3. Reuse `phase2_smoke_test.cpp` as the canonical secure witness path.
4. Add an optional BETTI-enabled smoke target that links to `Betti-FFI`.
5. Emit BETTI sidecar artifacts from the real secure mailbox scenario.
6. Verify both emitted witness files with `bettictl`.
7. Build `betti_millicent` and `betti_solver_ecology` as host-side proof-path surfaces.
8. Record the result in the BETTI usage doc, ledger, evidence bundle, genealogy record, ratification-gap report, and constitutional security note.

## Build / Run Commands

Standard class path:

```bash
make
make smoke
make transcript
```

BETTI sidecar path:

```bash
make betti-bundle
make betti-full
```

## Security Analysis

- Unauthorized secure send is denied.
- Unauthorized secure read is denied.
- Task 3 stores ciphertext-at-rest before authorized decryption.
- The BETTI sidecar is observational; it does not weaken the Phase 2 ACL path.
- The classroom cipher remains deterministic simulation code, not production cryptography.

## Performance / Practical Targets

- Standard `phase2_test` and `phase2_smoke_test` remain buildable without BETTI.
- BETTI integration is opt-in through `phase2_smoke_betti`.
- Proof-path builds stay outside the critical class build path.

## Hard Gates

- `GATE-P2-IPC-ACL`: unauthorized sender denied
- `GATE-P2-IPC-READ`: unauthorized reader denied
- `GATE-P2-IPC-CIPHER`: ciphertext-at-rest observed
- `GATE-P2-BETTI-WITNESS`: snapshot + commit witnesses emitted
- `GATE-P2-BETTI-VERIFY`: `bettictl` verifies both witnesses
- `GATE-P2-BETTI-PROOF-PATH`: `betti_millicent` and `betti_solver_ecology` build

## Theorem Status

- Specified: yes
- Implemented: yes
- Verified: yes, repo-local
- Ratified: no

## Honest Boundary

This is now a BETTI-integrated class submission sidecar, not a fully BETTI-native runtime artifact. The lowering, artifact fabric, genealogy, and ratification surfaces are real files in the submission folder, but the ULTIMA C++ runtime still does not compile directly through BETTI’s native IchorIR / MorphIR pipeline.
