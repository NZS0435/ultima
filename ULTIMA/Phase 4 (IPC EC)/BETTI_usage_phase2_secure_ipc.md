# BETTI Usage for Phase 2 Secure IPC

## Scope

This file records the second-pass BETTI integration for the ULTIMA Phase 2 secure IPC submission.

Important honesty boundary:
- This is no longer “BETTI method only.”
- It now uses real BETTI runtime and evidence surfaces as a class-submission sidecar.
- It is still not a fully BETTI-native production artifact.

## Security Primitive Used In This Pass

The secure Phase 2 runtime now uses the `BETTI-BITS-512` profile backed by the standard `AES-256-GCM` authenticated-encryption primitive.

Important honesty boundary:
- `BETTI-BITS-512` is not a stock BETTI runtime component and it is not a standards-body algorithm name.
- It is a local 512-bit master-key profile added to the existing Phase 2 IPC runtime.
- It derives 512 bits of master key material with `SHA-512` from the mailbox secret and mailbox identity.
- It uses the first 256 bits of that master material as the working `AES-256-GCM` key.
- It stores a full AEAD packet in `Msg_Cipher_Text`: nonce + ciphertext + authentication tag.
- It authenticates message metadata as additional authenticated data before delivery.
- The overall assignment remains class-scoped because secret provisioning, key lifecycle management, and external security review are still minimal.

## What From BETTI Was Used, Why, Where, and How

| BETTI feature | Why it was used | Where it was used | How it was used | Status |
| --- | --- | --- | --- | --- |
| IchorIR / MorphIR lowering | To express the secure IPC state transitions in BETTI-style canonical form | `betti_phase2_secure_ipc_ichorir_morphir_lowering.json` | The secure mailbox flow was lowered into a sidecar object/transition artifact covering configure, deny, encrypt-at-rest, and authorized decrypt | Implemented / Verified sidecar |
| Betti-FFI | To attach a real BETTI runtime dependency to the secure IPC witness path | `phase2_smoke_test.cpp`, `Makefile`, `CMakeLists.txt` | The optional `phase2_smoke_betti` build links against `libbetti_ffi` and emits BETTI snapshot + commit witnesses from the real secure mailbox scenario | Implemented / Verified |
| Bettictl integration | To verify emitted witnesses with an actual BETTI tool surface | `Makefile`, `betti_phase2_secure_ipc_*_verify.json` | `bettictl witness verify` was run on both emitted witness files and both returned `valid: true` | Implemented / Verified |
| Sovereign artifact fabric | To give the sidecar evidence bundle explicit lineage and artifact identity | `betti_phase2_secure_ipc_artifact_fabric.json` | The generated artifact-fabric record lists witness, profile, shadow-cell, and lowering artifacts plus their lineage edges | Implemented / Verified sidecar |
| Genealogy updates | To bind the new secure/BETTI work to the existing Phase 2 surfaces | `betti_phase2_secure_ipc_genealogy.json`, `betti_secure_ipc_genealogy_record.md` | Parent-child edges were recorded from the original mailbox path to secure send/receive and to the BETTI evidence bundle | Implemented / Verified sidecar |
| Ratification manifests | To record closure honestly instead of implying ratification | `betti_phase2_secure_ipc_ratification_manifest.json`, `betti_secure_ipc_ratification_gap_report.md` | A sidecar ratification manifest was emitted and explicitly marked `pending_live_ratification` with blockers | Implemented / Verified sidecar |
| Shadow cells / live operational profiles | To record the secure mailbox state as a BETTI-style live profile | `betti_phase2_secure_ipc_live_profile.json`, `betti_phase2_secure_ipc_shadow_cells.json` | The BETTI-enabled smoke run captured mailbox state, risk vector, PH summary, and a shadow-cell record from the real secure scenario | Implemented / Verified |
| Solver / MILLICENT proof paths | To show that BETTI proof-path packages were actually exercised, not just named | `Makefile` target `betti-proof-paths` | `cargo build -p betti_millicent` and `cargo build -p betti_solver_ecology` were run successfully from the class project folder via `make betti-full` | Implemented / Verified host-side |
| Constitutional promotion / trusted-path gating | To make the security closure criteria explicit | `betti_phase2_secure_ipc_trusted_path_gate.json`, `betti_secure_ipc_constitutional_security_note.md` | The secure IPC path now carries a trusted-path gate record for ACL denial, ciphertext-at-rest, authorized decrypt, witness emission, and `bettictl` verification | Implemented / Verified sidecar |
| BETTI runtime components as code dependencies | To move beyond methodology-only usage | `phase2_smoke_test.cpp`, `Makefile`, `CMakeLists.txt` | The assignment now has an optional compiled BETTI sidecar path that depends on `Betti-FFI` at link/runtime | Implemented / Verified |

## Why BETTI Was Useful Here

BETTI was useful in two distinct ways.

First, it continued to provide engineering discipline:
- extend the existing IPC surface instead of creating a duplicate subsystem
- fail closed on illegal security transitions
- separate specified / implemented / verified / ratified states honestly

Second, it now provides actual runtime and evidence surfaces:
- real witness emission through `Betti-FFI`
- real witness verification through `bettictl`
- real sidecar artifact, genealogy, shadow-cell, and gate files in the submission folder

## Where the Runtime Integration Lives

### 1. Phase 2 secure runtime

- `ipc.cpp`
- `ipc.h`
- `Message.h`

This is still the real Phase 2 runtime surface. In this pass, it is also where the `BETTI-BITS-512` profile lives:
- `derive_betti_bits_master_key(...)` derives the 512-bit master material used by the profile.
- `encrypt_to_hex(...)` and `decrypt_from_hex(...)` run the real `AES-256-GCM` authenticated-encryption and verification path.
- `betti_bits_digest_hex(...)` produces the full digest string shown for encrypted payload previews.

### 2. BETTI sidecar witness path

- `phase2_smoke_test.cpp`
- `Makefile`
- `CMakeLists.txt`

This is where BETTI moved from “method only” to “actual dependency and evidence path.”

### 3. Security-visible UI

- `Phase2_main.cpp`

The full `phase2_test` demo now shows Security/Privacy/Encryption from start to finish in the header, IPC panel, mailbox summaries, system-event panel, status panel, and transcript output. Encrypted payload previews are labeled `BETTI-BITS-512:` and are derived from the authenticated `AES-256-GCM` packet, and the UI wrapping logic was extended so the full value remains visible inside the ncurses boxes.

## Generated BETTI Artifacts

These are generated when the optional BETTI sidecar path is run:

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

## Supporting BETTI Documentation Added

- `phase_2_betti_secure_ipc_phase_ledger.md`
- `betti_secure_ipc_evidence_bundle.md`
- `betti_secure_ipc_genealogy_record.md`
- `betti_secure_ipc_ratification_gap_report.md`
- `betti_secure_ipc_constitutional_security_note.md`

## Reproduction Commands

Standard path:

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

## What Is Still Not True

The second pass does **not** justify these stronger claims:

- native ULTIMA-to-BETTI compiler lowering
- constitutional ratification
- external live-operational evidence closure
- `BETTI-BITS-512` being itself a standards-body-named primitive
- production-grade operational assurance around keys and deployment

## Bottom Line

The secure IPC assignment is now BETTI-informed **and** BETTI-integrated.

The integration is real enough for a class submission:
- runtime dependency
- witness generation
- witness verification
- proof-path builds
- artifact fabric
- genealogy
- trusted-path gate
- ratification-gap reporting

The integration is still sidecar-grade, not constitutionally closed production BETTI.
