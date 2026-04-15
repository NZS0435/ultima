# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - deliverables inventory, integration summary, build/test narrative
#   Zander Hayes   - MMU/allocator deliverable descriptions
#   Nicholas Kobs  - evidence, transcript, dump, and demo-output descriptions

# Phase 3 Deliverables and What Was Created

This file is a single reference that explains the major files, features, documentation, and runnable outputs created or materially updated for the current Phase 3 delivery.

## 1. Phase 3 Goal

Phase 3 extends the earlier scheduler and IPC work by adding a visible memory-management system. The final deliverable demonstrates:

- fixed-block first-fit allocation
- read and write operations on allocated memory
- linked-list style segment tracking
- free and coalesce behavior
- blocked allocation and wake-up behavior
- protection against invalid ownership access
- cumulative runtime visibility for scheduler, semaphore, IPC, and MMU state

## 2. Core Source Files

### `mmu.h`

Purpose:
- Declares the Phase 3 MMU interface and data structures.

What it provides:
- MMU configuration
- segment metadata
- allocation, free, read, and write function declarations
- dump and reporting function declarations
- runtime links to the scheduler and core-memory semaphore

Why it matters:
- This header defines the public contract for the memory-management subsystem.

### `mmu.cpp`

Purpose:
- Implements the full Phase 3 memory-management behavior.

What it provides:
- 1024-byte memory core management
- 64-byte block rounding
- first-fit allocation
- used/free segment ledger maintenance
- single-byte `Mem_Write`
- ranged `Mem_Write`
- ranged `Mem_Read`
- memory free and coalescing
- wrong-owner and bounds protection checks
- character and hex dump generation
- event text for transcript and UI evidence

Why it matters:
- This is the actual Phase 3 engine.

### `MCB.h`

Purpose:
- Defines the Phase 3 master control block.

What it provides:
- one scheduler instance
- monitor/printer/core semaphores
- one IPC messenger
- one MMU instance

Why it matters:
- It is the integration point that makes the phase cumulative rather than isolated.

### `Phase3_main.cpp`

Purpose:
- Drives the live ncurses Phase 3 demo.

What it provides:
- panel layout and rendering
- compact-layout fallback behavior
- event log and cumulative status displays
- step-by-step demo flow
- task scripting for Task A, B, C, and D
- transcript-only execution mode
- bottom step/status bar
- compact mailbox visibility improvements for tight terminal windows

Why it matters:
- This is the presentation and orchestration layer for the phase.

### `phase3_smoke_test.cpp`

Purpose:
- Provides an automated headless verification path.

What it provides:
- quick validation of the major Phase 3 behaviors
- protection against accidental regressions
- a test path that does not require the live ncurses UI

Why it matters:
- It provides fast evidence that the phase still works after code changes.

## 3. Build and Run Files

### `CMakeLists.txt`

Purpose:
- Defines the CLion/CMake build path for Phase 3.

What it provides:
- `ultima_os_phase3_core`
- `ultima_os_phase3`
- `ultima_os_phase3_smoke`
- correct curses linking
- Phase 3-only dependency glue for the existing IPC/scheduler stack

Important note:
- The final fix keeps Phase 4 untouched and fixes the build only through Phase 3’s own CMake wiring.

### `Makefile`

Purpose:
- Provides a local command-line build path for Phase 3.

What it provides:
- `make`
- `make run`
- `make transcript`
- `make test`
- `make clean`

Why it matters:
- It gives a second build path outside the IDE and helps verification stay repeatable.

### `.run/Phase3_test_GUI.run.xml`

Purpose:
- Shared CLion run configuration for the ncurses demo.

What it provides:
- an IDE launch target that uses an emulated terminal so the curses UI renders correctly

Why it matters:
- It reduces setup friction for running the live demo in CLion.

## 4. Documentation Files

### `README.md`

Purpose:
- Short overview of what Phase 3 adds and how to run it.

What it covers:
- feature summary
- build command
- live demo command
- transcript command
- smoke-test command

### `PHASE3_TEST_PLAN.md`

Purpose:
- Explains the cumulative testing strategy.

What it covers:
- why the phase should be tested cumulatively
- what runtime states should remain visible
- what evidence should be captured during verification

### `PHASE3_STEP_WALKTHROUGH.md`

Purpose:
- Describes all 12 demo steps in order.

What it covers:
- what each step does
- what the viewer should watch in each panel
- how scheduler, IPC, semaphores, and MMU change across the run

### `PHASE3_PAPER_OUTLINE.md`

Purpose:
- Gives a paper/report outline for writing about the phase.

What it covers:
- abstract
- introduction
- architecture
- implementation
- testing
- evidence
- conclusions

### `PHASE3_CREATED_DELIVERABLES.md`

Purpose:
- This file.

What it covers:
- a one-place inventory of the full Phase 3 delivery

## 5. Runtime Features Added for Phase 3

### First-Fit Allocation

Created behavior:
- allocation always searches for the first free segment large enough to satisfy the rounded request

Evidence:
- visible in the MMU ledger and transcript snapshots

### Fixed 64-Byte Block Rounding

Created behavior:
- user requests are rounded up to 64-byte boundaries before being reserved

Examples used in the demo:
- `130` becomes `192`
- `200` becomes `256`
- `100` becomes `128`
- `500` becomes `512`

### Linked-List Style Segment Ledger

Created behavior:
- the MMU tracks free and used partitions as ordered segments with start/end/size information

Evidence:
- visible in the MMU ledger panel and transcript tables

### Read / Write Operations

Created behavior:
- ranged reads
- ranged writes
- single-byte writes

Evidence:
- Task A ranged read-back
- Task C single-byte write sequence
- core dump contents changing after task writes

### Memory Protection

Created behavior:
- a task cannot write into memory owned by another task

Evidence:
- Task A attempts a write through Task D’s handle and the MMU rejects it

### Free + Coalesce

Created behavior:
- freed regions can merge into larger contiguous free segments

Evidence:
- Task B and Task C frees create the coalesced region Task D later allocates

### Blocked Allocation / Shark-Pond Wake-Up

Created behavior:
- a task can fail allocation because the largest contiguous segment is too small
- that task later becomes eligible again after free + coalesce

Evidence:
- Task D blocks at step 5 and succeeds later at step 9

## 6. Demo Tasks Created for the Cumulative Run

### `Task_A_Writer`

Responsibility:
- allocate first partition
- write payload
- send message to Task C
- read back its own memory
- receive Task B’s mailbox update
- attempt a protected foreign write
- free its partition

### `Task_B_Writer`

Responsibility:
- allocate middle partition
- create a later free-space hole
- send a service-style message to Task A
- free the middle block

### `Task_C_Reader`

Responsibility:
- allocate trailing partition
- perform single-byte writes
- receive Task A’s message
- free trailing memory to trigger coalescing

### `Task_D_Waiter`

Responsibility:
- request a too-large contiguous block
- become blocked
- wake after free + coalesce
- retry allocation successfully
- send a notification to Task A
- free the coalesced partition

## 7. Ncurses Windows Created for the Demo

### `SCHEDULER`

Shows:
- task states
- current task
- queue behavior
- runtime transitions

### `SEMAPHORES`

Shows:
- core-memory semaphore
- printer semaphore
- mailbox semaphore ownership and activity

### `IPC`

Shows:
- mailbox activity
- queued message counts
- compact mailbox status in smaller layouts
- last IPC action summary in the compact view

### `MMU LEDGER`

Shows:
- partition layout
- free/used segments
- bytes reserved vs requested
- waiting allocation state

### `CORE DUMP`

Shows:
- memory content in character view
- memory content in hex/ASCII view

### `EVENT LOG`

Shows:
- the latest important runtime events
- allocation, receive, free, coalesce, and protection messages

### `CUMULATIVE TEST`

Shows:
- testing checkpoints
- visible completion of major required behaviors
- key mailbox/readback/wake-up evidence strings

### `STEP STATUS`

Shows:
- current step number
- step title
- step description
- progress bar
- prompt to advance the demo

## 8. Verification Outputs Created

### `phase3_smoke_test`

Purpose:
- fast automated check of major Phase 3 behavior

### `phase3output.txt`

Purpose:
- generated cumulative transcript output at the repo root

What it contains:
- scheduler snapshots
- semaphore snapshots
- IPC state snapshots
- MMU tables and dumps
- event log output

### `Phase 3 transcript.txt`

Purpose:
- transcript artifact stored inside the Phase 3 directory

## 9. Root-Level / Shared Project Updates Related to Phase 3

These were updated to support the Phase 3 build and run flow:

- top-level `CMakeLists.txt`
- top-level `Makefile`
- `BUILD.md`
- shared CLion run configuration under `.run`

These support the Phase 3 experience, even when the primary implementation lives in the Phase 3 folder itself.

## 10. What the Completed Phase 3 Delivery Demonstrates

At the end of the current delivery, Phase 3 demonstrates:

- memory allocation and release
- correct block rounding
- cumulative scheduler + IPC + MMU visibility
- blocked and resumed allocation flow
- visible coalescing behavior
- protected memory ownership
- a live ncurses walkthrough
- transcript-only replay
- an automated smoke-test path

## 11. Recommended Use of This File

Use this document when you need to:

- explain the project to a teammate
- write the final paper
- prepare a presentation
- create screenshot captions
- justify what was actually built for Phase 3
