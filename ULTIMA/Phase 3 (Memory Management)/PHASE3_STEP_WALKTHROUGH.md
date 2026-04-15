# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - cumulative integration narrative and final phase flow
#   Zander Hayes   - allocator/coalesce behavior described in the step flow
#   Nicholas Kobs  - read/write, dump, and failure-case evidence described here

# Phase 3 Step-by-Step Walkthrough

This file describes what happens at each visible step in the Phase 3 ncurses demonstration.
It matches:

- the bottom step/status bar in `phase3_test`
- the cumulative transcript output
- the screenshot/evidence sequence used for the Phase 3 write-up

## Step 1 of 12: Initial READY state

What happens:
- The scheduler, IPC system, semaphores, and MMU are reset.
- Four tasks are created: `Task_A_Writer`, `Task_B_Writer`, `Task_C_Reader`, and `Task_D_Waiter`.
- The 1024-byte memory core starts as one free segment filled with dots (`.`).

What to observe:
- Scheduler panel: all four tasks are `READY`.
- Semaphore panel: all semaphores are free and have empty wait queues.
- IPC panel: all mailboxes are empty.
- MMU ledger: one free region from `0` to `1023`.

## Step 2 of 12: Task_A allocation + IPC ready

What happens:
- `Task_A_Writer` requests `130` bytes.
- The MMU rounds the request to `192` bytes using 64-byte blocks.
- The first-fit allocator places the block at the start of memory.
- Task A writes its payload into the partition.
- Task A sends an IPC text message to `Task_C_Reader`.

What to observe:
- Scheduler panel: Task A becomes `RUNNING`.
- IPC panel: Task C mailbox count increases.
- MMU ledger: a used segment appears at the front of memory.
- Core dump: Task A text appears in the first partition.

## Step 3 of 12: Task_B allocation + service message

What happens:
- `Task_B_Writer` requests `200` bytes.
- The MMU rounds the request to `256` bytes.
- First-fit places the second used segment immediately after Task A.
- Task B writes its payload into that middle partition.
- Task B sends a service-style IPC message to Task A.

What to observe:
- Scheduler panel: Task B becomes `RUNNING`.
- IPC panel: Task A mailbox now contains one message.
- MMU ledger: two used segments exist, leaving a trailing free segment.
- Core dump: Task B text appears in the middle partition.

## Step 4 of 12: Task_C single-byte write + mailbox receive

What happens:
- `Task_C_Reader` requests `100` bytes.
- The MMU rounds the request to `128` bytes.
- Task C writes its payload one character at a time using the single-byte `Mem_Write`.
- Single-byte writes advance `current_location`.
- Task C receives the earlier IPC text from Task A.

What to observe:
- Scheduler panel: Task C becomes `RUNNING`.
- IPC panel: Task C mailbox count drops after receive.
- MMU ledger: Task C current location advances into its partition.
- Event log: the mailbox receive and single-byte write evidence appear together.

## Step 5 of 12: Task_D blocked on insufficient contiguous memory

What happens:
- `Task_D_Waiter` requests `500` bytes.
- The MMU rounds the request to `512` bytes.
- Total free memory is not the issue; contiguous free memory is.
- The largest free segment is only `448` bytes, so allocation fails.
- Task D is blocked and placed into the wait path for the shark-pond retry behavior.

What to observe:
- Scheduler panel: Task D becomes `BLOCKED`.
- MMU ledger: no new used segment is created.
- Test panel: blocked allocation evidence is marked complete.
- Event log: the failed request records the largest available free segment.

## Step 6 of 12: Task_A read-back + mailbox receive

What happens:
- Task A performs a ranged read from its own partition.
- Task A verifies its own data can be read back successfully.
- Task A then receives Task B’s mailbox update.

What to observe:
- IPC panel: Task A mailbox count returns to zero after receive.
- Test panel: Task A readback and mailbox evidence fields populate.
- MMU state remains unchanged, proving read activity without reallocation.

## Step 7 of 12: Task_B free / first coalesce view

What happens:
- Task B frees its middle partition.
- The freed bytes are first overwritten with hashes (`#`).
- The MMU records the pre-coalesce dump.
- Because the freed partition is not yet adjacent to another free segment on both sides, only limited coalescing occurs at this point.

What to observe:
- MMU ledger: the middle segment becomes free.
- Dump/coalesce evidence: the “before” snapshot shows hash-filled freed memory.
- Scheduler panel: Task B is marked `DEAD`.

## Step 8 of 12: Task_C free + shark pond wake-up

What happens:
- Task C frees its trailing partition.
- The freed bytes are overwritten with hashes, then coalesced with adjacent free space.
- Coalescing merges the middle and trailing holes into a large free segment.
- Task D is unblocked so it can compete again for memory.

What to observe:
- Scheduler panel: Task D returns from `BLOCKED` to `READY`.
- MMU ledger: a large coalesced free region replaces the separate holes.
- Coalesce evidence: the “after” snapshot shows dots (`.`) in the merged free region.

## Step 9 of 12: Task_D acquired coalesced partition

What happens:
- Task D retries the same `500` byte request.
- The new contiguous free region is large enough, so allocation succeeds.
- Task D writes its payload into the coalesced partition.
- Task D sends a notification message to Task A.

What to observe:
- Scheduler panel: Task D becomes `RUNNING`.
- IPC panel: Task A mailbox count increases.
- MMU ledger: the coalesced free region becomes a used Task D partition.
- Test panel: shark-pond wake-up evidence is complete.

## Step 10 of 12: Task_A protection test + free

What happens:
- Task A attempts a single-byte write into Task D’s memory using Task D’s handle.
- The MMU rejects the write because the owner task does not match.
- This demonstrates base/limit-style protection and wrong-owner rejection.
- After the failed foreign write, Task A frees its own partition.

What to observe:
- Event log: segmentation-fault style rejection is recorded.
- Test panel: protection evidence flips to complete.
- MMU ledger: Task A’s partition becomes free.

## Step 11 of 12: Task_D final free

What happens:
- Task D frees the coalesced partition it acquired after waking.
- The MMU writes hashes before coalescing, then dots after coalescing.
- The system returns to one large free region.

What to observe:
- MMU ledger: one free region spans the full memory core again.
- Core dump: no task-owned text remains in active memory.
- Scheduler panel: Task D is marked `DEAD`.

## Step 12 of 12: Final state after garbage collection

What happens:
- The scheduler garbage collector removes all dead tasks.
- The cumulative Phase 3 demonstration ends in a clean system state.

What to observe:
- Scheduler panel: run queue is empty.
- IPC panel: all mailboxes are empty.
- Semaphore panel: all semaphores are free.
- MMU ledger: one free region from `0` to `1023`.

## Summary of what the full run proves

- First-fit allocation works with 64-byte block rounding.
- The linked-list ledger matches the core-memory dump.
- Single-byte and ranged reads/writes both work.
- A request can fail because no sufficiently large contiguous region exists.
- Freed memory shows hashes before coalescing and dots after coalescing.
- Shark-pond wake-up behavior returns waiting tasks to competition after free.
- Wrong-owner memory access is rejected.
- Phase 3 remains cumulative with scheduler, semaphore, and IPC visibility from earlier phases.
