# BETTI Constitutional Security Note for Phase 2 Secure IPC

## Scope

This note ties the Phase 2 secure IPC code to BETTI-style constitutional security language without overstating what the class project proves.

## Trusted Surface

The trusted runtime surface remains the existing Phase 2 mailbox and semaphore code:

- `ipc.cpp`
- `ipc.h`
- `Message.h`
- `phase2_smoke_test.cpp`
- `Phase2_main.cpp`

## Security Laws Enforced in Code

1. Fail closed on unauthorized send
   - `ipc.cpp::Secure_Message_Send`
   - If the sender is not on the mailbox ACL, the transition is denied.

2. Fail closed on unauthorized read
   - `ipc.cpp::Secure_Message_Receive`
   - If the reader is not the mailbox owner or an allowed reader, the transition is denied.

3. Ciphertext-at-rest before authorized delivery
   - `ipc.cpp`
   - The stored mailbox payload is a hex-encoded `AES-256-GCM` packet while the secure message waits in the queue.

4. Authorized decrypt only on lawful receive
   - `ipc.cpp`
   - Plaintext is restored only for the lawful secure receive path after AEAD verification succeeds.

5. Witness-bearing sidecar capture
   - `phase2_smoke_test.cpp`
   - The BETTI sidecar is emitted from the real secure mailbox scenario, not a separate mock harness.

6. End-to-end security visibility in the UI
   - `Phase2_main.cpp`
   - The ncurses / terminal dashboard now shows Security/Privacy/Encryption from the beginning of the demo through cleanup.

## Constitutional Promotion / Trusted-Path Gate

The trusted-path record is `betti_phase2_secure_ipc_trusted_path_gate.json`.

Current gate results:
- ACL sender denial: pass
- ciphertext-at-rest: pass
- ACL reader denial: pass
- authorized decrypt: pass
- BETTI snapshot witness emitted: pass
- BETTI commit witness emitted: pass
- `bettictl` witness verify: pass

## IchorIR / MorphIR Note

`betti_phase2_secure_ipc_ichorir_morphir_lowering.json` expresses the secure IPC transitions in BETTI-style canonical terms:

- configure secure mailbox
- deny unauthorized sender
- encrypt and enqueue
- authorized decrypt

This is a real sidecar artifact for the submission, but it is not a native BETTI compiler ingestion path.

## Threat Model

Covered in this class submission:
- unauthorized sender attempts
- unauthorized reader attempts
- plaintext exposure inside the mailbox queue

Not covered:
- hardened key management
- adversarial cryptanalysis
- external attestation
- production runtime hardening

## Security Honesty Boundary

The project now demonstrates restricted access, privacy-oriented queue storage, BETTI sidecar witnesses, and trusted-path recording.

It does not demonstrate constitutionally ratified production security.
