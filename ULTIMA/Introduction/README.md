![Team Thunder Banner](</Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg>)

ULTIMA 2.0 - Phase 1: Scheduler and Semaphore
Accelerated Team Execution Plan
Project Deadline: March 14, 2026 Primary Collaboration Hub: Microsoft Teams Channel
Team Members: * Stewart Pawley (Group Leader - Scheduler & Integration)
●	Nicholas Kobs (Semaphore Management)
●	Zander Hayes (Driver, Simulation & Documentation)
Phase A: Architecture & Setup (March 8 - March 9)
March 8: Pre-Meeting Prep (Stewart)
●	Initialize the shared cloud workspace in the Teams channel.
●	Create the exact required directory structure:
○	/ULTIMA
■	/Introduction
■	/Phase 1 (Scheduler)
●	Set up the base C++ environment and an empty makefile.
March 9 @ 6:00 PM EST: Initial Teams Kickoff Meeting (All Members)
●	Agenda Item 1: Define the Task Control Block (TCB). Agree on exact fields (Task ID, Name, State: RUNNING, READY, BLOCKED, DEAD).
●	Agenda Item 2: Outline header definitions for Class scheduler and Class semaphore based strictly on the assignment prompt.
●	Agenda Item 3 (Zander): Following the meeting, draft the initial Design Diagram (UML/Class hierarchy) using a drawing tool and commit it to both the Introduction and Phase 1 folders.
Phase B: Core Implementation (March 10 - March 11)
Given the tight timeline, development must happen in parallel.
1. Scheduler and Process Table (Stewart)
   ●	TCB Management: Build the dynamic thread table (linked list or scheduling ring).
   ●	State Logic: Implement transitions for the 4 states.
   ●	Core Methods: Write create_task(), kill_task(), yield(), and garbage_collect().
   ●	Algorithm: Implement strict round-robin process switching.
   ●	Debugging: Build the dump(int level) function. It must visually match the "Sample Process Table Dump" provided in the PDF.
2. Semaphore Implementation (Nicholas)
   ●	Data Structure: Implement resource_name, sema_value (binary 0 or 1), and sema_queue (queue for blocked tasks).
   ●	Core Methods: Write down() and up().
   ●	Crucial Constraint: down() cannot use busy waiting. It must block the task and interact with the scheduler to change the state to BLOCKED.
   ●	Debugging: Build the dump(int level) function to match the "Sample Semaphore Dump" in the PDF.
3. Main Driver & Environment (Zander)
   ●	Write Ultima.cpp to tie the modules together.
   ●	Design a multi-tasking test scenario. Create 2-3 mock tasks that attempt to acquire a shared resource (like printing to a screen).
   ●	Prepare the logic to demonstrate one task acquiring the resource while others queue and change to BLOCKED/READY.
   Phase C: Integration & Testing (March 12)
   March 12: Live Integration Session (All Members via Teams)
   ●	Merge Sched.cpp/.h and Sema.cpp/.h into Zander's main driver.
   ●	Run the mock tasks and aggressively test for deadlocks.
   ●	Verify memory leaks are prevented via garbage_collect().
   ●	Ensure the console output properly displays the transitioning states and accurate queue lists in the dump functions.
   Phase D: Deliverables & Media (March 13)
1. Output Generation & Word Document (Zander & Nicholas)
   ●	Compile the required Word document. Must include:
   ○	Cleanly formatted source code snippets (with internal documentation).
   ○	Screenshots of the console output highlighting the dump() tables.
   ○	A written test plan explaining how and why the system works.
   ○	The Phase Abstract.
2. Video Demonstration (Stewart)
   ●	Record a short, narrated video showing the program execution.
   ●	Visually highlight tasks executing, changing states, and the dump() outputs proving that semaphores queue tasks correctly.
   ●	Export as .mp4 and place in the Phase 1 folder.
   Phase E: Final Audit & Submission (March 14)
   Morning Audit (Stewart)
   ●	Verify the file structure exactly matches Prof. Hakimzadeh's requirements:
   ○	Introduction Folder: Cover page, overall abstract, combined diagrams, team resumes.
   ○	Phase 1 Folder: Assignment copy, phase abstract, phase diagram, source code (separated by modules), external docs, Word output doc, Makefile, and the narrated video.
   Afternoon Finalization (All Members)
   ●	Each member must individually complete the "Self / Peer Evaluation Form" (PDF Pages 5-6).
   ●	Note: Do not upload this to the shared Teams folder. This is a private evaluation.
   Final Submission (By 11:59 PM - Stewart)
   ●	Create Introduction.zip.
   ●	Create Phase1_Scheduler.zip.
   ●	Submit both zip files and Stewart's individual peer evaluation form to Canvas.
   ●	Confirm with Nicholas and Zander that they have submitted their peer evaluations independently.
   Directory for Introduction phase.



Phase 1: Scheduler and Semaphore

Phase Abstract

ULTIMA 2.0 requires a robust foundation for process management and resource synchronization. In Phase 1, we implemented a dynamic Task Control Block (TCB) infrastructure, a round-robin cooperative scheduler, and a binary semaphore mechanism. The scheduler maintains task states (RUNNING, READY, BLOCKED, DEAD) and handles context switching and garbage collection to prevent memory leaks. Concurrently, the binary semaphore safely manages access to shared resources without relying on inefficient busy-waiting loops. By directly interfacing with the scheduler, the semaphore effectively transitions blocked tasks into a queued BLOCKED state and wakes them into a READY state upon resource release. This unified system permanently resolves race conditions and mutual exclusion conflicts during shared I/O operations.

System Test Plan & Expected Output

Objective: Prove that the integrated scheduler and semaphore handle shared resource contention flawlessly without deadlocks.

Testing Scenario:
We designed a cooperative multi-tasking simulation via the main driver (Ultima.cpp). The simulation features a global semaphore, screen_mutex (representing a shared printer/console), and three distinct mock tasks (Task_A, Task_B, and Task_C).

Step 1 (Acquisition): Task A initiates, calls down(), and successfully acquires the semaphore (sema_value changes from 1 to 0). Task A then yields control back to the scheduler to simulate ongoing background work while holding the resource lock.

Step 2 (Contention & Queuing): The scheduler passes execution to Task B, and then Task C. Both attempt to call down() on screen_mutex. Because the value is 0, the semaphore delegates to the scheduler, changing Task B and Task C's states from READY to BLOCKED, and pushes their task IDs into the sema_queue.

Step 3 (Proof of Queue): A mid-execution dump() is triggered. The output clearly shows:

Scheduler Table: Task A is RUNNING/READY, while Task B and Task C display as BLOCKED.

Semaphore Data: Sema_value is 0, and Sema_queue displays T-2 --> T-3.

Step 4 (Release & Wake): Task A resumes, finishes its logic, and calls up(). The semaphore pops Task B from the queue and instructs the scheduler to transition Task B to READY. Task B subsequently runs, finishes, and passes the resource to Task C.

This test conclusively demonstrates that mutual exclusion is maintained, blocked tasks are successfully preserved in memory via the TCBs, and system stability is preserved through coordinated scheduling.
