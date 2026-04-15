# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - paper structure, systems narrative, cumulative integration framing
#   Zander Hayes   - allocator/coalescing sections and MMU behavior framing
#   Nicholas Kobs  - testing/evidence sections, dump/read-write explanation, presentation flow

# Phase 3 Paper Outline

This outline is designed for a course paper or technical report that explains the full Phase 3 memory-management deliverable and its cumulative relationship to the earlier scheduler and IPC phases.

## Suggested Title

- `ULTIMA 2.0 Phase 3: A Cumulative Memory-Management Demonstration with Scheduler, IPC, Semaphore, and MMU Visibility`

## 1. Abstract

- State that Phase 3 extends the existing ULTIMA scheduler and IPC system with a memory-management unit.
- Summarize the main additions: first-fit allocation, fixed-size block rounding, linked-list segment tracking, coalescing, blocked allocation, protection checks, and ncurses visualization.
- Mention that the system is cumulative, so scheduler, semaphores, IPC, and memory are all visible at once.
- Briefly state the test/evidence approach and the main outcome.

## 2. Introduction

- Explain the purpose of the ULTIMA project and where Phase 3 fits in the overall sequence.
- Describe why memory management is a natural next step after scheduler and IPC functionality.
- State the instructional goal of the phase: demonstrate both implementation correctness and runtime behavior clearly.
- Introduce the use of ncurses windows as the presentation layer for the live demo.

## 3. Problem Statement

- Define the challenge: memory must be allocated, used, freed, and protected while earlier phase behavior remains visible.
- Explain why the design must support cumulative testing instead of isolated function tests.
- Describe the need to show blocked tasks, mailbox activity, semaphore ownership, and MMU layout in one run.

## 4. Phase 3 Requirements

- 1024-byte memory core initialized to a default fill character.
- Fixed 64-byte block rounding for allocation.
- First-fit allocation strategy.
- Linked-list style ledger of used and free partitions.
- Single-byte and ranged `Mem_Write` support.
- Ranged `Mem_Read` support.
- Free-space recovery and coalescing.
- Protection against invalid or foreign-owner access.
- Runtime handling for blocked allocation requests.
- A cumulative demonstration that still exposes scheduler, semaphore, and IPC activity.

## 5. System Architecture

### 5.1 Master Control Block

- Describe the `MCB` as the point where the scheduler, semaphores, IPC messenger, and MMU are brought together.
- Explain the role of the core-memory semaphore versus mailbox semaphores.

### 5.2 Memory-Management Unit

- Explain the MMU memory core, block size, and allocation policy.
- Describe the internal segment ledger and the relationship between handles, owners, and offsets.

### 5.3 Task Model

- Introduce the four demo tasks:
- `Task_A_Writer`
- `Task_B_Writer`
- `Task_C_Reader`
- `Task_D_Waiter`
- Explain how each task was chosen to exercise a different part of the MMU and cumulative runtime.

### 5.4 Ncurses Interface

- Describe the windowed layout:
- Scheduler
- Semaphores
- IPC
- MMU Ledger
- Core Dump
- Event Log
- Cumulative Test panel
- Step Status bar
- Explain how the UI supports smaller CLion/debug consoles with compact rendering.

## 6. Implementation Details

### 6.1 Allocation Logic

- Explain first-fit search.
- Explain 64-byte rounding.
- Explain how requested bytes differ from reserved bytes.

### 6.2 Read and Write Behavior

- Explain ranged writes and ranged reads.
- Explain single-byte writes and `current_location` advancement.
- Explain how memory contents appear in the character/hex dump.

### 6.3 Free and Coalesce Logic

- Explain how freed regions are marked.
- Explain pre-coalesce and post-coalesce evidence.
- Explain when adjacent free segments merge and why that matters for blocked allocation recovery.

### 6.4 Protection and Ownership

- Explain task ownership of handles.
- Explain why a foreign write attempt is rejected.
- Relate this to base/limit-style protection behavior.

### 6.5 Blocked Allocation and Shark-Pond Wake-Up

- Explain why a request can fail even when total free memory exists.
- Explain the difference between total free memory and largest contiguous segment.
- Explain how a blocked task becomes eligible again after free + coalesce.

## 7. Cumulative Demonstration Flow

- Present the 12-step runtime sequence.
- Reference the dedicated step walkthrough file.
- For each major step, identify:
- scheduler change
- IPC change
- semaphore effect
- MMU effect
- This section can summarize rather than fully repeat the step-by-step file.

## 8. Testing Strategy

- Explain why the testing strategy is cumulative.
- Describe the smoke test.
- Describe transcript-only mode.
- Explain why screenshots and transcript snapshots are both useful evidence.
- Highlight the specific behaviors verified:
- successful allocations
- blocked allocation
- mailbox send/receive
- semaphore activity
- coalescing
- protection failure
- cleanup/garbage collection

## 9. Results and Evidence

- Summarize what the transcript proves.
- Summarize what the ncurses demo proves visually.
- Show that the system returns to a clean final state.
- Mention the step bar and cumulative test panel as presentation evidence.

## 10. Challenges and Fixes

- Compact terminal layout issues and the bottom step bar fix.
- Mailbox visibility mismatch in compact IPC views and the later fix.
- CLion/OpenSSL build dependency issue and the Phase 3-only build wiring correction.
- Any mismatch between transcript wording and visible runtime state that had to be aligned.

## 11. Lessons Learned

- The value of cumulative testing over isolated tests.
- The importance of visual instrumentation in systems projects.
- Why allocation correctness must be paired with visibility and cleanup behavior.
- How UI and build-system details can affect the credibility of the final demo.

## 12. Conclusion

- Re-state the Phase 3 contribution.
- Emphasize that the phase demonstrates not only memory allocation, but coordinated runtime state across multiple OS subsystems.
- Conclude with the project value of a cumulative, observable systems implementation.

## 13. Appendices

- Appendix A: Build and run commands
- Appendix B: Phase 3 transcript excerpts
- Appendix C: Screenshot sequence
- Appendix D: File inventory
- Appendix E: Step-by-step walkthrough

## Recommended Figures and Tables

- Figure 1: Ncurses full-window layout.
- Figure 2: MMU ledger before coalescing.
- Figure 3: MMU ledger after coalescing.
- Figure 4: Step 6 mailbox receive state.
- Table 1: Task responsibilities in the demo.
- Table 2: Allocation requests, rounded sizes, and resulting partitions.
- Table 3: Test cases and expected outcomes.

## Recommended Source Material To Cite From This Repo

- `README.md`
- `PHASE3_STEP_WALKTHROUGH.md`
- `PHASE3_TEST_PLAN.md`
- `Phase3_main.cpp`
- `mmu.cpp`
- `mmu.h`
- `phase3_smoke_test.cpp`
