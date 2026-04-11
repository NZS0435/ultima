/* =========================================================================
 * Sched.cpp — ULTIMA 2.0 Phase 2 scheduler integration
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Primary Author : Stewart Pawley
 * Co-Authors     : Zander Hayes   (IPC-facing scheduler hooks)
 *                  Nicholas Kobs  (Phase 2 demo alignment)
 * =========================================================================
 */

#include "Sched.h"
#include "../Phase 1 (Scheduler)/Sema.h"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
std::string format_task_label(const TCB* task) {
    if (task == nullptr) {
        return "[none]";
    }

    std::ostringstream label_stream;
    label_stream << task->task_name << " (T-" << task->task_id << ")";
    return label_stream.str();
}
} // namespace

Scheduler::Scheduler() :
    current_running_task(-1),
    next_task_id(1),
    dispatch_in_progress(false),
    scheduler_tick(0),
    last_scheduler_event("Scheduler initialized.") {}

Scheduler::~Scheduler() {
    reset();
}

int Scheduler::find_task_index(int task_id) const {
    for (std::size_t index = 0; index < process_table.size(); ++index) {
        if (process_table[index] != nullptr && process_table[index]->task_id == task_id) {
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
    const int start_index = (current_running_task >= 0)
        ? (current_running_task + 1) % task_count
        : 0;

    for (int offset = 0; offset < task_count; ++offset) {
        const int candidate_index = (start_index + offset) % task_count;
        const TCB* candidate = process_table[static_cast<std::size_t>(candidate_index)];
        if (candidate != nullptr && candidate->task_state == READY) {
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
        default:
            return "UNKNOWN";
    }
}

void Scheduler::reset() {
    for (TCB* task : process_table) {
        // Phase 2 Cleanup: Delete the allocated mailbox semaphore
        if (task != nullptr && task->mailbox_semaphore != nullptr) {
            delete task->mailbox_semaphore;
            task->mailbox_semaphore = nullptr;
        }

        delete task;
    }

    process_table.clear();
    current_running_task = -1;
    next_task_id = 1;
    dispatch_in_progress = false;
    scheduler_tick = 0;
    last_scheduler_event = "Scheduler reset; process table cleared.";
}

int Scheduler::create_task(const char* task_name, TaskEntryPoint func_ptr) {
    const std::string safe_name = (task_name != nullptr) ? task_name : "Unnamed_Task";
    TCB* new_task = new TCB(next_task_id++, safe_name, func_ptr);

    // --- PHASE 2 INTEGRATION (Stewart Pawley) ---
    // Instantiate a unique semaphore to protect this task's specific mailbox queue.
    std::string sema_name = "Mailbox_Sema_T" + std::to_string(new_task->task_id);
    new_task->mailbox_semaphore = new Semaphore(sema_name, 1, this);

    process_table.push_back(new_task);

    std::ostringstream event_stream;
    event_stream << "Created " << format_task_label(new_task) << ".";
    last_scheduler_event = event_stream.str();
    return new_task->task_id;
}

void Scheduler::kill_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index < 0) {
        return;
    }

    TCB* task = process_table[static_cast<std::size_t>(task_index)];
    task->task_state = DEAD;
    task->last_transition = "Entered DEAD state.";

    std::ostringstream event_stream;
    event_stream << "Marked " << format_task_label(task) << " DEAD.";
    last_scheduler_event = event_stream.str();
}

void Scheduler::yield() {
    if (dispatch_in_progress) {
        last_scheduler_event = "Nested yield request deferred until the active dispatch returns.";
        return;
    }

    if (process_table.empty()) {
        last_scheduler_event = "Yield requested with an empty process table.";
        return;
    }

    if (current_running_task >= 0 && current_running_task < static_cast<int>(process_table.size())) {
        TCB* current_task = process_table[static_cast<std::size_t>(current_running_task)];
        if (current_task != nullptr && current_task->task_state == RUNNING) {
            current_task->task_state = READY;
            ++current_task->yield_count;
            current_task->last_transition = "Yielded CPU and returned to READY.";
        }
    }

    const int next_ready_index = find_next_ready_index();
    if (next_ready_index < 0) {
        current_running_task = -1;
        last_scheduler_event = "No READY tasks were available to dispatch.";
        return;
    }

    current_running_task = next_ready_index;
    TCB* next_task = process_table[static_cast<std::size_t>(next_ready_index)];
    next_task->task_state = RUNNING;
    ++next_task->dispatch_count;
    ++scheduler_tick;

    std::ostringstream transition_stream;
    transition_stream << "Entered RUNNING at scheduler tick " << scheduler_tick << ".";
    next_task->last_transition = transition_stream.str();

    std::ostringstream event_stream;
    event_stream << "Dispatching " << format_task_label(next_task)
                 << " at tick " << scheduler_tick << ".";
    last_scheduler_event = event_stream.str();

    if (next_task->task_entry == nullptr) {
        return;
    }

    const bool was_dispatching = dispatch_in_progress;
    dispatch_in_progress = true;
    next_task->task_entry();
    dispatch_in_progress = was_dispatching;
}

void Scheduler::garbage_collect() {
    bool removed_any_dead_tasks = false;

    for (std::size_t index = 0; index < process_table.size();) {
        TCB* task = process_table[index];
        if (task != nullptr && task->task_state == DEAD) {
            // Phase 2 Cleanup: Prevent memory leaks from mailbox semaphores
            if (task->mailbox_semaphore != nullptr) {
                delete task->mailbox_semaphore;
                task->mailbox_semaphore = nullptr;
            }

            delete task;
            process_table.erase(process_table.begin() + static_cast<std::ptrdiff_t>(index));
            removed_any_dead_tasks = true;

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
    }

    last_scheduler_event = removed_any_dead_tasks
        ? "Garbage collection removed DEAD tasks from the process table."
        : "Garbage collection found no DEAD tasks.";
}

void Scheduler::block_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index < 0) {
        return;
    }

    TCB* task = process_table[static_cast<std::size_t>(task_index)];
    if (task->task_state == DEAD || task->task_state == BLOCKED) {
        return;
    }

    task->task_state = BLOCKED;
    ++task->block_count;
    task->last_transition = "Entered BLOCKED state.";

    std::ostringstream event_stream;
    event_stream << "Blocked " << format_task_label(task) << ".";
    last_scheduler_event = event_stream.str();
}

void Scheduler::unblock_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index < 0) {
        return;
    }

    TCB* task = process_table[static_cast<std::size_t>(task_index)];
    if (task->task_state != BLOCKED) {
        return;
    }

    task->task_state = READY;
    ++task->unblock_count;
    task->last_transition = "Woken from BLOCKED and returned to READY.";

    std::ostringstream event_stream;
    event_stream << "Unblocked " << format_task_label(task) << ".";
    last_scheduler_event = event_stream.str();
}

void Scheduler::block_current_task() {
    const int task_id = get_current_task_id();
    if (task_id >= 0) {
        block_task(task_id);
    }
}

void Scheduler::dump(int level) const {
    std::cout << "Scheduler Dump\n";
    std::cout << "==============\n";
    std::cout << "Tick: " << scheduler_tick << '\n';
    std::cout << "Current Task: " << get_current_task_id() << '\n';
    std::cout << "Last Event: " << last_scheduler_event << '\n';
    std::cout << "Run Queue: " << describe_run_queue() << "\n\n";

    std::cout << std::left
              << std::setw(8) << "Task ID"
              << std::setw(16) << "Name"
              << std::setw(10) << "State"
              << std::setw(8) << "Run"
              << std::setw(8) << "Yield"
              << std::setw(8) << "Block"
              << std::setw(8) << "Wake"
              << "Details\n";
    std::cout << "--------------------------------------------------------------------------\n";

    for (const TCB* task : process_table) {
        std::cout << std::left
                  << std::setw(8) << task->task_id
                  << std::setw(16) << task->task_name
                  << std::setw(10) << state_to_string(task->task_state)
                  << std::setw(8) << task->dispatch_count
                  << std::setw(8) << task->yield_count
                  << std::setw(8) << task->block_count
                  << std::setw(8) << task->unblock_count
                  << task->detail_note
                  << '\n';

        if (level > 0) {
            std::cout << "         Last transition: " << task->last_transition << '\n';
        }
    }

    std::cout << '\n';
}

std::string Scheduler::dump_string(int level) const {
    std::ostringstream out;

    out << "Scheduler Dump\n";
    out << "==============\n";
    out << "Tick: " << scheduler_tick << '\n';
    out << "Current Task: " << get_current_task_id() << '\n';
    out << "Last Event: " << last_scheduler_event << '\n';
    out << "Run Queue: " << describe_run_queue() << "\n\n";

    out << std::left
        << std::setw(8) << "Task ID"
        << std::setw(16) << "Name"
        << std::setw(10) << "State"
        << std::setw(8) << "Run"
        << std::setw(8) << "Yield"
        << std::setw(8) << "Block"
        << std::setw(8) << "Wake"
        << "Details\n";
    out << "--------------------------------------------------------------------------\n";

    for (const TCB* task : process_table) {
        if (task == nullptr) {
            continue;
        }

        out << std::left
            << std::setw(8) << task->task_id
            << std::setw(16) << task->task_name
            << std::setw(10) << state_to_string(task->task_state)
            << std::setw(8) << task->dispatch_count
            << std::setw(8) << task->yield_count
            << std::setw(8) << task->block_count
            << std::setw(8) << task->unblock_count
            << task->detail_note
            << '\n';

        if (level > 0) {
            out << "         Last transition: " << task->last_transition << '\n';
        }
    }

    out << '\n';
    return out.str();
}

int Scheduler::get_current_task_id() const {
    if (current_running_task < 0 || current_running_task >= static_cast<int>(process_table.size())) {
        return -1;
    }

    const TCB* task = process_table[static_cast<std::size_t>(current_running_task)];
    return (task != nullptr) ? task->task_id : -1;
}

int Scheduler::get_active_task_count() const {
    int count = 0;
    for (const TCB* task : process_table) {
        if (task != nullptr && task->task_state != DEAD) {
            ++count;
        }
    }
    return count;
}

int Scheduler::get_ready_task_count() const {
    int count = 0;
    for (const TCB* task : process_table) {
        if (task != nullptr && task->task_state == READY) {
            ++count;
        }
    }
    return count;
}

int Scheduler::get_blocked_task_count() const {
    int count = 0;
    for (const TCB* task : process_table) {
        if (task != nullptr && task->task_state == BLOCKED) {
            ++count;
        }
    }
    return count;
}

int Scheduler::get_dead_task_count() const {
    int count = 0;
    for (const TCB* task : process_table) {
        if (task != nullptr && task->task_state == DEAD) {
            ++count;
        }
    }
    return count;
}

bool Scheduler::has_active_tasks() const {
    return get_active_task_count() > 0;
}

bool Scheduler::is_task_blocked(int task_id) const {
    return get_task_state(task_id) == BLOCKED;
}

State Scheduler::get_task_state(int task_id) const {
    const int task_index = find_task_index(task_id);
    if (task_index < 0) {
        return DEAD;
    }

    return process_table[static_cast<std::size_t>(task_index)]->task_state;
}

const char* Scheduler::get_task_state_name(int task_id) const {
    return state_to_string(get_task_state(task_id));
}

std::string Scheduler::get_task_name(int task_id) const {
    const int task_index = find_task_index(task_id);
    if (task_index < 0) {
        return "[Unknown Task]";
    }

    return process_table[static_cast<std::size_t>(task_index)]->task_name;
}

void Scheduler::set_task_note(int task_id, const std::string& note) {
    const int task_index = find_task_index(task_id);
    if (task_index < 0) {
        return;
    }

    process_table[static_cast<std::size_t>(task_index)]->detail_note = note;
}

std::vector<TaskSnapshot> Scheduler::snapshot_tasks() const {
    std::vector<TaskSnapshot> snapshots;
    snapshots.reserve(process_table.size());

    for (const TCB* task : process_table) {
        if (task == nullptr) {
            continue;
        }

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
        return "[empty]";
    }

    std::ostringstream queue_stream;
    bool wrote_any_task = false;

    for (const TCB* task : process_table) {
        if (task == nullptr || task->task_state == DEAD) {
            continue;
        }

        if (wrote_any_task) {
            queue_stream << " -> ";
        }

        queue_stream << task->task_name << "(T-" << task->task_id << ", "
                     << state_to_string(task->task_state) << ")";
        wrote_any_task = true;
    }

    return wrote_any_task ? queue_stream.str() : "[empty]";
}

int Scheduler::get_scheduler_tick() const {
    return scheduler_tick;
}

std::string Scheduler::get_last_scheduler_event() const {
    return last_scheduler_event;
}

// --- PHASE 2 EXPOSURE ---
TCB* Scheduler::get_tcb(int task_id) const {
    const int index = find_task_index(task_id);
    if (index >= 0) {
        return process_table[static_cast<std::size_t>(index)];
    }

    return nullptr;
}
