#include "Sched.h"
#include <iostream>
#include <iomanip>

Scheduler::Scheduler() {
    next_task_id = 1;
    current_task_it = process_table.end();
}

Scheduler::~Scheduler() {
    // Clean up dynamically allocated memory upon teardown
    for (auto* tcb : process_table) {
        delete tcb;
    }
    process_table.clear();
}

int Scheduler::create_task(const char* task_name) {
    // Dynamically allocate new TCB and insert into our scheduling ring
    TCB* new_task = new TCB(next_task_id++, std::string(task_name));
    process_table.push_back(new_task);

    // If this is the very first task, set our iterator to it
    if (process_table.size() == 1) {
        current_task_it = process_table.begin();
    }

    return new_task->task_id;
}

void Scheduler::kill_task(int t_id) {
    // Find the task and set its status to DEAD (actual removal happens in GC)
    for (auto* tcb : process_table) {
        if (tcb->task_id == t_id) {
            tcb->state = DEAD;
            break;
        }
    }
}

void Scheduler::yield() {
    if (process_table.empty()) return;

    // 1. Current task yields - if it was running, it is now just ready
    if (current_task_it != process_table.end() && (*current_task_it)->state == RUNNING) {
        (*current_task_it)->state = READY;
    }

    // 2. Strict round robin process switch
    auto start_it = current_task_it;
    bool found_next = false;

    do {
        // Advance iterator, wrap around to the beginning if we hit the end
        if (current_task_it != process_table.end()) {
            ++current_task_it;
        }
        if (current_task_it == process_table.end()) {
            current_task_it = process_table.begin();
        }

        // Look for the next task that is READY to run
        if ((*current_task_it)->state == READY) {
            (*current_task_it)->state = RUNNING;
            found_next = true;
            break;
        }
    } while (current_task_it != start_it);

    // Edge Case: If no other task is READY, but the yielding task is still READY,
    // let it immediately resume running.
    if (!found_next && (*start_it)->state == READY) {
         (*start_it)->state = RUNNING;
         current_task_it = start_it;
    }
}

void Scheduler::garbage_collect() {
    // Remove dead tasks, free their resources, etc.
    auto it = process_table.begin();
    while (it != process_table.end()) {
        if ((*it)->state == DEAD) {
            // Protect our current tracker from becoming a dangling pointer
            if (it == current_task_it) {
                current_task_it = process_table.end();
            }
            delete *it;
            it = process_table.erase(it);
        } else {
            ++it;
        }
    }

    // Re-calibrate the tracker if the list shifted
    if (current_task_it == process_table.end() && !process_table.empty()) {
         current_task_it = process_table.begin();
    }
}

void Scheduler::dump(int level) {
    // Debugging function that visually matches the "Sample Process Table Dump"
    std::cout << "\nTask Name\tTask ID\t\tState\t\tetc.\n";
    std::cout << "------------------------------------------------------------\n";

    for (auto* tcb : process_table) {
        std::string state_str;
        switch (tcb->state) {
            case RUNNING: state_str = "Running"; break;
            case READY:   state_str = "Ready";   break;
            case BLOCKED: state_str = "Blocked"; break;
            case DEAD:    state_str = "Dead";    break;
        }

        // Formatted to exactly match the PDF specification
        std::cout << std::left << std::setw(16) << tcb->task_name
                  << std::setw(16) << tcb->task_id
                  << std::setw(16) << state_str << "\n";
    }
    std::cout << "------------------------------------------------------------\n";
}

// --- Hooks for Semaphore Management ---

int Scheduler::get_current_task_id() {
    if (current_task_it != process_table.end()) {
        return (*current_task_it)->task_id;
    }
    return -1;
}

void Scheduler::block_current_task() {
    // Used by Semaphore down() to block a task instead of busy-waiting
    if (current_task_it != process_table.end()) {
        (*current_task_it)->state = BLOCKED;
        yield(); // Immediately force a context switch
    }
}

void Scheduler::unblock_task(int t_id) {
    // Used by Semaphore up() to wake a task up
    for (auto* tcb : process_table) {
        if (tcb->task_id == t_id && tcb->state == BLOCKED) {
            tcb->state = READY;
            break;
        }
    }
}