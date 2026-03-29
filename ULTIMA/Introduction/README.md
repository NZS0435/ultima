ULTIMA 2.0 - Phase 1: Scheduler, Semaphore, and Memory Manager
Accelerated Team Execution Plan
Project Deadline: March 14, 2026 Primary Collaboration Hub: Microsoft Teams Channel
Team Members: * Stewart Pawley (Group Leader - Scheduler, Memory Manager, & Integration)
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
●	Agenda Item 3: Outline a deterministic memory-manager interface so later phases can grow beyond task scheduling alone.
●	Agenda Item 4 (Zander): Following the meeting, draft the initial Design Diagram (UML/Class hierarchy) using a drawing tool and commit it to both the Introduction and Phase 1 folders.
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
3. Memory Manager (Stewart)
   ●	Data Structure: Implement a deterministic first-fit heap layout with explicit free and used segments.
   ●	Core Methods: Write allocate(), free_block(), write(), read(), and dump().
   ●	Demo Contract: Reserve one task buffer per demo task and prove that the heap returns to a single free segment by the end of the run.
4. Main Driver & Environment (Zander)
   ●	Write Ultima.cpp to tie the modules together.
   ●	Design a multi-tasking test scenario. Create 2-3 mock tasks that attempt to acquire a shared resource (like printing to a screen) while also claiming and releasing task buffers from the memory manager.
   ●	Prepare the logic to demonstrate one task acquiring the resource while others queue and change to BLOCKED/READY and the heap transitions through allocation and free states.
   Phase C: Integration & Testing (March 12)
   March 12: Live Integration Session (All Members via Teams)
   ●	Merge Sched.cpp/.h, Sema.cpp/.h, and Mem_mgr.cpp/.h into Zander's main driver.
   ●	Run the mock tasks and aggressively test for deadlocks.
   ●	Verify memory leaks are prevented via garbage_collect() and Mem_mgr free_block().
   ●	Ensure the console output properly displays the transitioning states, accurate queue lists, and the current heap layout in the dump functions.
   Phase D: Deliverables & Media (March 13)
1. Output Generation & Word Document (Zander & Nicholas)
   ●	Compile the required Word document. Must include:
   ○	Cleanly formatted source code snippets (with internal documentation).
   ○	Screenshots of the console output highlighting the dump() tables.
   ○	A written test plan explaining how and why the system works.
   ○	The Phase Abstract.
2. Video Demonstration (Stewart)
   ●	Record a short, narrated video showing the program execution.
   ●	Visually highlight tasks executing, changing states, the semaphore queueing proof, and the Mem_mgr dump() output proving deterministic allocation and release.
   ●	Export as .mp4 and place in the Phase 1 folder.
   Phase E: Final Audit & Submission (March 14)
   Morning Audit (Stewart)
   ●	Verify the file structure exactly matches Prof. Hakimzadeh's requirements:
   ○	Introduction Folder: Cover page, overall abstract, combined diagrams, team resumes.
   ○	Phase 1 Folder: Assignment copy, phase abstract, phase diagram, source code (separated by modules, including Mem_mgr.h / Mem_mgr.cpp), external docs, Word output doc, Makefile, and the narrated video.
   Afternoon Finalization (All Members)
   ●	Each member must individually complete the "Self / Peer Evaluation Form" (PDF Pages 5-6).
   ●	Note: Do not upload this to the shared Teams folder. This is a private evaluation.
   Final Submission (By 11:59 PM - Stewart)
   ●	Create Introduction.zip.
   ●	Create Phase1_Scheduler.zip.
   ●	Submit both zip files and Stewart's individual peer evaluation form to Canvas.
   ●	Confirm with Nicholas and Zander that they have submitted their peer evaluations independently.
   Directory for Introduction phase.
