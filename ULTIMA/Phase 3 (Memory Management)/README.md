# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - phase architecture summary, integration notes, final merge
#   Zander Hayes   - allocator/free-space behavior summary
#   Nicholas Kobs  - dump/testing evidence summary

# ULTIMA 2.0 Phase 3 - Memory Management

This phase adds a cumulative memory-management unit on top of the existing Phase 2 scheduler and IPC stack.

What the demo shows:

- A 1024-byte MMU initialized with `.` and fixed 64-byte first-fit allocation.
- Linked-list tracking of free and used segments.
- Single-byte and ranged `Mem_Read` / `Mem_Write`.
- A blocked allocation request that later succeeds after free + coalesce.
- A core-memory semaphore alongside the mailbox semaphores from Phase 2.
- Base/limit-style protection that rejects a foreign-memory write attempt.
- Cumulative visibility of scheduler state, semaphore ownership, IPC traffic, and MMU state in ncurses windows.

Build from this directory:

```bash
make
```

Run the ncurses demonstration:

```bash
./phase3_test
```

Generate a transcript-only cumulative dump:

```bash
./phase3_test --transcript-only --output-file phase3_transcript.txt
```

Run the headless smoke test:

```bash
./phase3_smoke_test
```
