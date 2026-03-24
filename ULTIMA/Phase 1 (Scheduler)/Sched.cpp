/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Creator: STEWART PAWLEY - TEAM THUNDER */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#include "U2_Scheduler.h"
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <sstream>

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: STEWART PAWLEY
 */


Scheduler::Scheduler() :
    current_running_task(-1),
    next_task_id(1),
    dispatch_in_progress(false),
    scheduler_tick(0),
    last_scheduler_event("Scheduler constructed and awaiting tasks.") {
}

Scheduler::~Scheduler() {
    reset();
}

void Scheduler::reset() {
    for (TCB* task : process_table) {
        delete task;
    }
    process_table.clear();

    current_running_task = -1;
    next_task_id = 1;
    dispatch_in_progress = false;
    scheduler_tick = 0;
    last_scheduler_event = "Scheduler reset. Process table is empty.";
}

int Scheduler::find_task_index(int task_id) const {
    for (std::size_t index = 0; index < process_table.size(); ++index) {
        if (process_table[index]->task_id == task_id) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int Scheduler::find_next_ready_index() const {
    if (process_table.empty()) {
        return -1;
    }

    const int task_count = static_cast<int>(process_table.size());
    const int start_index = (current_running_task >= 0 && current_running_task < task_count)
        ? current_running_task
        : -1;

    for (int offset = 1; offset <= task_count; ++offset) {
        const int candidate_index = (start_index + offset + task_count) % task_count;
        if (process_table[candidate_index]->task_state == READY) {
            return candidate_index;
        }
    }

    return -1;
}

const char* Scheduler::state_to_string(State state) {
    switch (state) {
        case RUNNING:
            return "RUNNING";
        case READY:
            return "READY";
        case BLOCKED:
            return "BLOCKED";
        case DEAD:
            return "DEAD";
    }
    return "Unknown";
}

int Scheduler::create_task(const char* task_name, TaskEntryPoint func_ptr) {
    TCB* new_task = new TCB(next_task_id++, std::string(task_name), func_ptr);
    process_table.push_back(new_task);
    last_scheduler_event = new_task->task_name + " inserted into the process table as READY.";
    return new_task->task_id;
}

void Scheduler::kill_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0 && process_table[task_index]->task_state != DEAD) {
        TCB* task = process_table[task_index];
        task->task_state = DEAD;
        task->detail_note = "Task completed its work and is waiting for garbage collection.";
        task->last_transition = "kill_task() moved the task into DEAD.";
        last_scheduler_event = task->task_name + " finished execution and entered DEAD.";
        if (task_index == current_running_task) {
            current_running_task = task_index;
        }
    }
}

void Scheduler::yield() {
    if (process_table.empty()) {
        return;
    }

    if (dispatch_in_progress) {
        if (current_running_task >= 0
            && current_running_task < static_cast<int>(process_table.size())
            && process_table[current_running_task]->task_state == RUNNING) {
            TCB* running_task = process_table[current_running_task];
            running_task->task_state = READY;
            ++running_task->yield_count;
            running_task->last_transition = "yield() returned the task to READY.";
            last_scheduler_event = running_task->task_name + " yielded the processor cooperatively.";
        }
        return;
    }

    const int next_ready_index = find_next_ready_index();
    if (next_ready_index < 0) {
        last_scheduler_event = "No READY tasks were available for dispatch.";
        return;
    }

    current_running_task = next_ready_index;
    TCB* scheduled_task = process_table[current_running_task];
    scheduled_task->task_state = RUNNING;
    ++scheduled_task->dispatch_count;
    scheduled_task->last_transition = "Round-robin dispatcher selected this task.";
    ++scheduler_tick;

    {
        std::ostringstream event_stream;
        event_stream << "Tick " << scheduler_tick
                     << ": dispatching "
                     << scheduled_task->task_name
                     << " (T-" << scheduled_task->task_id << ").";
        last_scheduler_event = event_stream.str();
    }

    dispatch_in_progress = true;
    if (scheduled_task->task_entry != nullptr) {
        scheduled_task->task_entry();
    }
    dispatch_in_progress = false;

    if (current_running_task >= 0
        && current_running_task < static_cast<int>(process_table.size())
        && process_table[current_running_task]->task_state == RUNNING) {
        process_table[current_running_task]->task_state = READY;
        process_table[current_running_task]->last_transition =
            "Quantum ended without a block, so the task returned to READY.";
    }
}

void Scheduler::garbage_collect() {
    int removed_tasks = 0;

    for (std::size_t index = 0; index < process_table.size();) {
        if (process_table[index]->task_state == DEAD) {
            delete process_table[index];
            process_table.erase(process_table.begin() + static_cast<std::ptrdiff_t>(index));
            ++removed_tasks;
            if (current_running_task == static_cast<int>(index)) {
                current_running_task = -1;
            } else if (current_running_task > static_cast<int>(index)) {
                --current_running_task;
            }
            continue;
        }
        ++index;
    }

    if (process_table.empty()) {
        current_running_task = -1;
    } else if (current_running_task >= static_cast<int>(process_table.size())) {
        current_running_task = 0;
    }

    std::ostringstream event_stream;
    event_stream << "Garbage collector removed " << removed_tasks << " DEAD task(s).";
    last_scheduler_event = event_stream.str();
}

void Scheduler::block_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0
        && process_table[task_index]->task_state != DEAD
        && process_table[task_index]->task_state != BLOCKED) {
        TCB* task = process_table[task_index];
        task->task_state = BLOCKED;
        ++task->block_count;
        task->last_transition = "Task was blocked and removed from the run queue.";
        last_scheduler_event = task->task_name + " entered BLOCKED while waiting on a resource.";
    }
}

void Scheduler::unblock_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0 && process_table[task_index]->task_state == BLOCKED) {
        TCB* task = process_table[task_index];
        task->task_state = READY;
        ++task->unblock_count;
        task->last_transition = "Task was unblocked and returned to READY.";
        last_scheduler_event = task->task_name + " left BLOCKED and rejoined the READY queue.";
    }
}

void Scheduler::dump(int level) const {
    std::cout << "\nScheduler Dump:\n";
    std::cout << "Policy:        Strict cooperative round robin\n";
    std::cout << "SchedulerTick: " << scheduler_tick << "\n";
    std::cout << "CurrentTask:   ";

    if (current_running_task >= 0 && current_running_task < static_cast<int>(process_table.size())) {
        const TCB* task = process_table[current_running_task];
        std::cout << task->task_name << " (T-" << task->task_id << ")\n";
    } else {
        std::cout << "[None]\n";
    }

    std::cout << "Counts:        READY=" << get_ready_task_count()
              << " BLOCKED=" << get_blocked_task_count()
              << " DEAD=" << get_dead_task_count()
              << " ACTIVE=" << get_active_task_count() << "\n";
    std::cout << "RunQueue:      " << describe_run_queue() << "\n";
    std::cout << "LastEvent:     " << last_scheduler_event << "\n\n";

    std::cout << std::left
              << std::setw(12) << "Task"
              << std::setw(8) << "ID"
              << std::setw(10) << "State"
              << std::setw(10) << "Run#"
              << std::setw(10) << "Yield#"
              << std::setw(10) << "Block#"
              << std::setw(10) << "Wake#"
              << "Note\n";
    std::cout << "------------------------------------------------------------------------------------------------\n";

    for (const TCB* task : process_table) {
        std::cout << std::left
                  << std::setw(12) << task->task_name
                  << std::setw(8) << task->task_id
                  << std::setw(10) << state_to_string(task->task_state)
                  << std::setw(10) << task->dispatch_count
                  << std::setw(10) << task->yield_count
                  << std::setw(10) << task->block_count
                  << std::setw(10) << task->unblock_count
                  << task->detail_note
                  << "\n";

        if (level > 0) {
            std::cout << "  Transition: " << task->last_transition << "\n";
        }

        if (level > 1 && task->context_pointer != nullptr) {
            std::cout << "  ContextPtr: " << task->context_pointer << "\n";
        }
    }

    std::cout << "------------------------------------------------------------------------------------------------\n";
}

int Scheduler::get_current_task_id() const {
    if (current_running_task >= 0 && current_running_task < static_cast<int>(process_table.size())) {
        return process_table[current_running_task]->task_id;
    }
    return -1;
}

int Scheduler::get_active_task_count() const {
    int active_task_count = 0;
    for (const TCB* task : process_table) {
        if (task->task_state != DEAD) {
            ++active_task_count;
        }
    }
    return active_task_count;
}

int Scheduler::get_ready_task_count() const {
    return static_cast<int>(std::count_if(process_table.begin(), process_table.end(), [](const TCB* task) {
        return task->task_state == READY;
    }));
}

int Scheduler::get_blocked_task_count() const {
    return static_cast<int>(std::count_if(process_table.begin(), process_table.end(), [](const TCB* task) {
        return task->task_state == BLOCKED;
    }));
}

int Scheduler::get_dead_task_count() const {
    return static_cast<int>(std::count_if(process_table.begin(), process_table.end(), [](const TCB* task) {
        return task->task_state == DEAD;
    }));
}

bool Scheduler::has_active_tasks() const {
    return std::any_of(process_table.begin(), process_table.end(), [](const TCB* task) {
        return task->task_state != DEAD;
    });
}

bool Scheduler::is_task_blocked(int task_id) const {
    const int task_index = find_task_index(task_id);
    return task_index >= 0 && process_table[task_index]->task_state == BLOCKED;
}

void Scheduler::block_current_task() {
    const int current_task_id = get_current_task_id();
    if (current_task_id >= 0) {
        block_task(current_task_id);
    }
}

State Scheduler::get_task_state(int task_id) const {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0) {
        return process_table[task_index]->task_state;
    }
    return DEAD;
}

const char* Scheduler::get_task_state_name(int task_id) const {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0) {
        return state_to_string(process_table[task_index]->task_state);
    }
    return "MISSING";
}

std::string Scheduler::get_task_name(int task_id) const {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0) {
        return process_table[task_index]->task_name;
    }
    return "[None]";
}

void Scheduler::set_task_note(int task_id, const std::string& note) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0) {
        process_table[task_index]->detail_note = note;
    }
}

std::vector<TaskSnapshot> Scheduler::snapshot_tasks() const {
    std::vector<TaskSnapshot> snapshots;
    snapshots.reserve(process_table.size());

    for (const TCB* task : process_table) {
        snapshots.push_back(TaskSnapshot {
            task->task_id,
            task->task_name,
            task->task_state,
            task->dispatch_count,
            task->yield_count,
            task->block_count,
            task->unblock_count,
            task->detail_note,
            task->last_transition
        });
    }

    return snapshots;
}

std::string Scheduler::describe_run_queue() const {
    if (process_table.empty()) {
        return "Process table empty.";
    }

    std::ostringstream queue_stream;
    bool wrote_any_task = false;
    const int task_count = static_cast<int>(process_table.size());
    const int start_index = (current_running_task >= 0 && current_running_task < task_count)
        ? current_running_task
        : -1;

    for (int offset = 1; offset <= task_count; ++offset) {
        const int candidate_index = (start_index + offset + task_count) % task_count;
        const TCB* task = process_table[candidate_index];
        if (task->task_state == READY) {
            if (wrote_any_task) {
                queue_stream << " -> ";
            }
            queue_stream << task->task_name << "(T-" << task->task_id << ")";
            wrote_any_task = true;
        }
    }

    if (!wrote_any_task) {
        queue_stream << "[No READY tasks]";
    }

    return queue_stream.str();
}

int Scheduler::get_scheduler_tick() const {
    return scheduler_tick;
}

std::string Scheduler::get_last_scheduler_event() const {
    return last_scheduler_event;
}
