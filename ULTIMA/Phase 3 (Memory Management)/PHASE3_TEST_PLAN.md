# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - cumulative integration test plan and final smoke path
#   Zander Hayes   - allocator test matrix expectations
#   Nicholas Kobs  - screenshot/evidence package and dump-focused testing

# Phase 3 Cumulative Test Plan

## Goal

Verify Phase 3 without losing evidence from Phases 1 and 2.

The test surface must show, at multiple points in time:

- scheduler state transitions (`RUNNING`, `READY`, `BLOCKED`, `DEAD`)
- semaphore ownership and wait queues
- IPC mailbox counts and message routes
- MMU linked-list state
- core-memory content before and after coalescing
- allowed memory access by owner task
- rejected memory access by non-owner task

## Automated Coverage

Use:

```bash
make test
```

This runs:

1. `phase3_smoke_test`
   - validates successful allocation
   - validates blocked allocation
   - validates shark-pond wake-up
   - validates IPC still works in the cumulative build
   - validates protection fault rejection
   - validates full-memory return after all frees

2. `phase3_test --transcript-only`
   - captures cumulative snapshots after each visible transition
   - emits scheduler, semaphore, IPC, and MMU dumps in one transcript

## Interactive ncurses Walkthrough

Run:

```bash
./phase3_test
```

Advance one keypress at a time and capture screenshots at these moments:

1. Initial READY state with empty mailboxes and fully free memory.
2. First successful allocation by Task_A.
3. Middle allocation by Task_B.
4. Single-byte write sequence by Task_C.
5. Blocked request by Task_D.
6. Scheduler + mailbox state after Task_A read-back.
7. Task_B free with pre/post coalesce snapshot.
8. Task_C free with shark-pond wake-up.
9. Task_D successful retry after coalescing.
10. Protection fault rejection when Task_A attempts a foreign write.
11. Final full free state.
12. Post-garbage-collection scheduler cleanup.

## What to include in the write-up

- one screenshot set from the ncurses demo
- the generated transcript file
- the smoke-test result
- a short explanation of why the blocked request fails before coalescing and succeeds after it
- a short explanation of how ownership protection was verified
