# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - security integration, BETTI sidecar, and cumulative demo narrative
#   Zander Hayes   - mailbox IPC path, queue semantics, and message utility behavior
#   Nicholas Kobs  - validation scenario, annotated walkthrough flow, and transcript alignment

# Phase 4 Secure IPC Step-by-Step Walkthrough

This file explains the seventeen visible steps in the secure IPC extra-credit demo described in the Phase 4 PDF report and the matching runtime transcript.

Important locality note:
- The course labels this deliverable as Phase 4 extra credit.
- In this workspace, the submission-local walkthrough belongs under `ULTIMA/Phase 4 (IPC EC)`.
- The secure IPC capability is a cumulative extension of the original Phase 2 mailbox runtime, so the walkthrough follows the real runtime in `Phase2_main.cpp`, `ipc.cpp`, `phase2_transcript.txt`, and the PDF report `Team Thunder #001 Phase 4 Extra Credit.pdf`.

Important theorem-status boundary:
- Specified: the 17-step secure IPC story is described in the Phase 4 PDF and transcript.
- Implemented: mailbox ACL enforcement, ciphertext-at-rest storage, authorized decrypt, UI framing, and message utilities exist in code.
- Verified: the repo contains transcript evidence, smoke assertions, and BETTI sidecar verification artifacts.
- Ratified: no. The BETTI integration here is a class-submission sidecar, not a constitutionally closed production BETTI runtime.

## Revolutionary BETTI Uses Across The Demo

- The Phase 4 PDF explicitly frames BETTI as the team's revolutionary six-pillar system: one doctrine, one object algebra, one artifact memory, one replay law, one promotion law, and one operational covenant.
- `BETTI-BITS-512` is the local mailbox-key profile used by the secure path. It derives 512 bits of master material with `SHA-512`, uses the first 256 bits as the working `AES-256-GCM` key, and stores the nonce + ciphertext + authentication tag in `Msg_Cipher_Text`.
- `IchorIR / MorphIR` lowering records the secure mailbox flow as a sidecar transition artifact covering configure, deny, encrypt-at-rest, and authorized decrypt.
- `Betti-FFI` and `bettictl` support runtime witness emission and witness verification for the real secure mailbox smoke scenario.
- `shadow_cells`, `live_profile`, `artifact_fabric`, and `genealogy` record the secure mailbox state, lineage, and evidence surfaces around the demo.
- `trusted_path_gate` and the ratification docs record what was verified locally and what still remains outside constitutional closure.

## Step 1 of 17: Security baseline

What happens:
- `Configure_Mailbox_Security(t3, ...)` marks Task 3's mailbox as security-enabled, restricted, and encrypted.
- `Allow_Mailbox_Sender(t3, t2)` whitelists Task 2 as the lawful secure sender.
- Task 3 is automatically inserted as its own authorized reader.
- The demo header, security panes, and event log start in Security / Privacy / Encryption mode before any protected traffic exists.

What to observe:
- All three tasks exist and are still in their initial ready state.
- Task 3's mailbox summary shows `BETTI-BITS-512(AES-256-GCM) / Restricted`.
- The event log records the security configuration and ACL setup before any message delivery occurs.

Revolutionary BETTI use in this step:
- BETTI's discipline shows up as "configure the control surface before exposure."
- The sidecar lowering treats this as the lawful `configure_secure_mailbox` transition that begins the secure chain.
- The live-profile and shadow-cell artifacts use this step as the baseline secure mailbox state.

## Step 2 of 17: Unauthorized sender denial

What happens:
- Task 1 calls `Secure_Message_Send(t1, t3, ...)`.
- `Secure_Message_Send(...)` delegates into `Message_Send(...)` only if the destination mailbox is actually marked secure and encrypted.
- `Message_Send(...)` checks `sender_authorized(...)` before any queue mutation or semaphore-protected enqueue occurs.
- Because Task 1 is not on Task 3's authorized sender list, the send is denied immediately and the mailbox is left untouched.

What to observe:
- The event log prints `ACCESS DENIED: T-1 cannot send to secure mailbox T-3`.
- Task 3's mailbox count remains zero.
- No ciphertext appears, because the packet is rejected before encryption and queue insertion.

Revolutionary BETTI use in this step:
- This is the fail-closed rule in action.
- The trusted-path gate records this as the ACL sender-denial proof point.
- The sidecar witness path uses this denial to prove that illegal transitions do not mutate secure state.

## Step 3 of 17: Plain IPC path A

What happens:
- Task 3 dispatches and sends a normal Notification message to Task 1 using `Message_Send(...)`.
- The destination is an open mailbox, so the standard Phase 2 message path remains active.
- The message is enqueued under the mailbox semaphore and logged as plaintext.

What to observe:
- Task 1's mailbox count increases.
- The event log shows semaphore entry, semaphore exit, and the plaintext send record.
- The secure mailbox on Task 3 remains restricted, but the rest of the IPC system still behaves like Phase 2.

Revolutionary BETTI use in this step:
- BETTI's practical contribution here is architectural restraint.
- Instead of forking IPC into a second subsystem, the secure phase extends the original mailbox path and preserves cumulative behavior.
- Genealogy records this as secure functionality growing out of the original `Message_Send` surface rather than replacing it.

## Step 4 of 17: Observe Task 1's greeting

What happens:
- The demo pauses so the UI can show the visible effect of Step 3.
- No new mailbox mutation happens in this step; it is an observation checkpoint.

What to observe:
- Task 1's window now displays the professor greeting.
- The scheduler, mailbox panes, and security pane remain stable.
- The event log preserves the earlier plaintext send and the secure baseline context at the same time.

Revolutionary BETTI use in this step:
- BETTI sidecar value here is observability, not mutation.
- The secure story stays cumulative: baseline security is still visible while ordinary IPC remains readable.
- This is the kind of state snapshot that the live-profile and witness surfaces are meant to preserve.

## Step 5 of 17: Plain IPC path B

What happens:
- Task 1 sends a Text message to Task 2 with `Message_Send(t1, t2, ...)`.
- The message is typed as `Text`, queued under semaphore protection, and logged in plaintext.

What to observe:
- Task 2's mailbox count increases.
- The message category is visible as `Text`.
- The event log again shows the semaphore bracket around queue access.

Revolutionary BETTI use in this step:
- This step proves the secure work did not break the base IPC semantics.
- BETTI's "extend, do not duplicate" rule is visible because the same runtime still supports normal Text traffic next to the secure mailbox path.

## Step 6 of 17: Observe Task 2's greeting

What happens:
- The demo pauses again for a display checkpoint.
- This step is purely about visible confirmation of the message flow from Step 5.

What to observe:
- Task 2's window now shows the Team Thunder greeting.
- Task 3 is still the designated secure mailbox target for the later encrypted send.
- The status bar and event panes continue to frame the run as a security demo rather than a disconnected sequence of screenshots.

Revolutionary BETTI use in this step:
- BETTI contributes continuity.
- Security is not turned on only when encryption appears; it remains a through-line while ordinary IPC still works.

## Step 7 of 17: Authorized secure send and ciphertext-at-rest

What happens:
- Task 2 sends a Service message to Task 3 with `Secure_Message_Send(t2, t3, ...)`.
- `Message_Send(...)` first confirms that Task 2 is an authorized sender for Task 3.
- `apply_mailbox_security(...)` then encrypts the payload because the destination mailbox is marked encrypted.
- `derive_betti_bits_master_key(...)` builds the 512-bit master material from mailbox identity and secret.
- `encrypt_to_hex(...)` generates the nonce, authenticates the message metadata as AAD, runs `EVP_aes_256_gcm()`, and stores the hex-encoded AEAD packet in `Msg_Cipher_Text`.
- The original plaintext buffer is cleared before the message is queued.

What to observe:
- The log says Task 3's secure mailbox now holds ciphertext-at-rest only.
- The visible payload is shown as a `BETTI-BITS-512:` digest label rather than the plaintext service text.
- Task 3's queue gains one secure message while the plaintext itself is no longer exposed in the mailbox dump.

Revolutionary BETTI use in this step:
- This is the most visible BETTI-branded step in the whole demo.
- `BETTI-BITS-512` is the local profile name for the mailbox-key path layered over standard `AES-256-GCM`.
- The sidecar ledger and trusted-path gate use this step as the ciphertext-at-rest proof point.

## Step 8 of 17: Plain receive A

What happens:
- Task 1 reads the greeting from its own mailbox.
- `Message_Receive(...)` routes through `Secure_Message_Receive(...)`, so there is one receive law for both plain and secure cases.
- Because this mailbox message is plaintext, delivery succeeds without decryption.

What to observe:
- Task 1's mailbox count drops.
- The event log prints the received Notification payload in plaintext.
- Task 3's secure ciphertext remains queued and untouched.

Revolutionary BETTI use in this step:
- BETTI's contribution is unification of the receive path.
- The same governed delivery function handles both plain and secure mail, which makes later secure proofs easier to reason about.

## Step 9 of 17: Plain receive B

What happens:
- Task 2 reads the Text greeting that Task 1 previously queued.
- The message leaves the mailbox and is logged as a normal plaintext receive.

What to observe:
- Task 2's mailbox count returns to zero.
- The event log shows the sender, type, and plaintext payload.
- The secure ciphertext for Task 3 still remains in place for the denial and decrypt steps that follow.

Revolutionary BETTI use in this step:
- This is another cumulative-compatibility proof.
- The secure phase does not turn the whole IPC runtime into encrypted-only behavior; it secures only the mailbox that was explicitly configured for protection.

## Step 10 of 17: Unauthorized reader denial while ciphertext stays queued

What happens:
- Task 1 tries to read Task 3's secure mailbox through `Secure_Message_Receive(t1, t3, ...)`.
- `receiver_authorized(...)` rejects the request before delivery.
- The queued secure message is not removed from the mailbox because the requester is not lawful.

What to observe:
- The event log prints `ACCESS DENIED: T-1 cannot read secure mailbox T-3`.
- The follow-up line confirms that Task 3's secure mailbox still holds ciphertext-at-rest only.
- Queue state does not change even though a read was attempted.

Revolutionary BETTI use in this step:
- This is the second fail-closed gate and the read-side partner to Step 2.
- The trusted-path record uses it as the unauthorized-reader denial proof point.
- BETTI's admissibility discipline matters here because an illegal reader cannot even consume the ciphertext placeholder.

## Step 11 of 17: Authorized decrypt

What happens:
- Task 3 performs the lawful `Secure_Message_Receive(t3, t3, ...)`.
- The secure message is dequeued under semaphore protection.
- `deliver_mailbox_message(...)` detects that the message is encrypted and calls `decrypt_from_hex(...)`.
- `decrypt_from_hex(...)` reconstructs the working key, validates the AEAD packet, restores the plaintext into `Msg_Text`, and returns it only after verification succeeds.

What to observe:
- The event log prints `MSG RECV SECURE` and then logs the restored plaintext service message.
- Task 3's queue count drops after delivery.
- This is the first moment where the original service payload is visible again.

Revolutionary BETTI use in this step:
- BETTI's secure-law story culminates here: ciphertext-at-rest stays sealed until the lawful receiver performs the governed decrypt.
- The sidecar lowering treats this as the `authorized_decrypt` transition that closes the protected communication loop.
- The witness and verification artifacts use this step as the "good path" counterpart to the earlier denials.

## Step 12 of 17: DeleteAll setup

What happens:
- Three demo messages are sent to Task 1: one Text, one Service, and one Notification.
- This step intentionally builds a non-trivial mailbox state before the bulk-delete utility is exercised.

What to observe:
- Task 1's mailbox count rises to three.
- The event log shows all three message categories entering the queue.
- The secure mailbox story remains intact while the demo now pivots to utility coverage.

Revolutionary BETTI use in this step:
- BETTI's role here is less about cryptography and more about cumulative completeness.
- The secure submission still has to prove that the old utility surface is alive after the security extension.

## Step 13 of 17: DeleteAll completion

What happens:
- `Message_DeleteAll(t1)` clears Task 1's mailbox under semaphore protection.
- The function returns the number of removed messages, which the event log reports immediately.

What to observe:
- Task 1's mailbox becomes empty.
- The log prints the exact removal count.
- The utility behavior matches the visible mailbox state.

Revolutionary BETTI use in this step:
- This is a small but important integrity check.
- Security hardening did not break queue cleanup or utility reporting.
- In BETTI terms, the system preserved the pre-existing lawful transition for mailbox reset.

## Step 14 of 17: Message_Count demonstration

What happens:
- The demo sends one message to Task 2 and two messages to Task 1.
- `Message_Count(t1)`, `Message_Count(t2)`, and `Message_Count()` are then called to report per-mailbox and global totals.

What to observe:
- The event log prints `Task1=2`, `Task2=1`, and `Total=3`.
- The distribution is intentionally uneven, so the totals prove real counting rather than a trivial mirrored case.
- The UI counts line up with the utility output.

Revolutionary BETTI use in this step:
- BETTI's contribution is evidence discipline.
- The demo does not merely claim that `Message_Count` still works; it constructs a visible state that makes a wrong implementation easy to catch.

## Step 15 of 17: Security summary

What happens:
- The walkthrough stops to restate the security invariants that were just demonstrated.
- The log summarizes restricted access, fail-closed denial, ciphertext-at-rest, and successful authorized decrypt.

What to observe:
- The summary lines compress the whole run into one audit statement.
- The security framing remains visible in the UI rather than being relegated to a report appendix.

Revolutionary BETTI use in this step:
- This is where the trusted-path gate, genealogy record, and evidence bundle all conceptually line up with the visible demo.
- The system is not just "secure-looking"; it has a documented chain of denial, encryption, and lawful release.

## Step 16 of 17: Cleanup and garbage collection

What happens:
- The scheduler marks all three demo tasks as `DEAD`.
- Garbage collection removes them from the process table.
- The event log records the cleanup sequence.

What to observe:
- The scheduler pane no longer shows live demo tasks.
- Mailbox activity is over and the run transitions to shutdown.
- The cleanup happens in the normal scheduler surface rather than through a fake report-only ending.

Revolutionary BETTI use in this step:
- BETTI's value here is honest closure.
- The demo ends with a visible state transition back toward quiescence instead of just stopping after the successful decrypt.
- The commit-witness sidecar exists to pair this end-state with the earlier secure-state evidence.

## Step 17 of 17: Final secure closeout

What happens:
- The demo emits the final `SECURE PHASE 2 DEMONSTRATION COMPLETE` banner.
- The banner keeps the cumulative `Phase 2` runtime label because the Phase 4 extra-credit work hardens the original Phase 2 IPC subsystem instead of replacing it with a separate demo path.
- The status line explicitly says that Security / Privacy / Encryption stayed visible from start to finish.

What to observe:
- The walkthrough closes with the same security framing it opened with.
- The final frame acts as the bookend to the baseline in Step 1.

Revolutionary BETTI use in this step:
- BETTI's strongest contribution here is narrative integrity plus honest status reporting.
- The repo-local evidence supports specified, implemented, and verified for this classroom path.
- The sidecar docs also make clear that ratification is still pending and that this remains a class-scoped secure IPC artifact rather than a production BETTI closure.

## Summary Of What The Full Run Proves

- Restricted mailbox policy is configured before any protected traffic is allowed.
- Unauthorized senders are denied before queue mutation.
- Unauthorized readers are denied while ciphertext remains queued.
- Task 3's secure mailbox stores ciphertext-at-rest rather than plaintext.
- Only the lawful receiver can trigger authenticated decrypt and recover the service payload.
- Plain IPC still works for Notification, Text, and Service traffic on open mailboxes.
- `Message_DeleteAll` and `Message_Count` still behave correctly after the secure extension.
- The BETTI sidecar adds witness, lineage, canonical lowering, gate, and verification surfaces around the real secure IPC runtime.

## Honest Boundary

- `BETTI-BITS-512` is a local profile name, not a standards-body algorithm name.
- The actual AEAD primitive used by the runtime is `AES-256-GCM`.
- The sidecar evidence in this folder is real and repo-local.
- This workspace does not establish constitutional ratification, external live-operational closure, or production-grade key management.
