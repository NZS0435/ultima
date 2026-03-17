#include "U2_Scheduler.h"
#include <iomanip>
#include <iostream>

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: STEWART PAWLEY
 */


Scheduler::Scheduler() :
    current_running_task(-1),
    next_task_id(1),
    dispatch_in_progress(false) {
}

Scheduler::~Scheduler() {
    for (TCB* task : process_table) {
        delete task;
    }
    process_table.clear();
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

const char* Scheduler::state_to_string(State state) const {
    switch (state) {
        case RUNNING:
            return "Running";
        case READY:
            return "Ready";
        case BLOCKED:
            return "Blocked";
        case DEAD:
            return "Dead";
    }
    return "Unknown";
}

int Scheduler::create_task(const char* task_name, TaskEntryPoint func_ptr) {
    TCB* new_task = new TCB(next_task_id++, std::string(task_name), func_ptr);
    process_table.push_back(new_task);
    return new_task->task_id;
}

void Scheduler::kill_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0) {
        process_table[task_index]->task_state = DEAD;
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
            process_table[current_running_task]->task_state = READY;
        }
        return;
    }

    const int next_ready_index = find_next_ready_index();
    if (next_ready_index < 0) {
        return;
    }

    current_running_task = next_ready_index;
    TCB* scheduled_task = process_table[current_running_task];
    scheduled_task->task_state = RUNNING;

    dispatch_in_progress = true;
    if (scheduled_task->task_entry != nullptr) {
        scheduled_task->task_entry();
    }
    dispatch_in_progress = false;

    if (current_running_task >= 0
        && current_running_task < static_cast<int>(process_table.size())
        && process_table[current_running_task]->task_state == RUNNING) {
        process_table[current_running_task]->task_state = READY;
    }
}

void Scheduler::garbage_collect() {
    for (std::size_t index = 0; index < process_table.size();) {
        if (process_table[index]->task_state == DEAD) {
            delete process_table[index];
            process_table.erase(process_table.begin() + static_cast<std::ptrdiff_t>(index));
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
}

void Scheduler::block_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0 && process_table[task_index]->task_state != DEAD) {
        process_table[task_index]->task_state = BLOCKED;
    }
}

void Scheduler::unblock_task(int task_id) {
    const int task_index = find_task_index(task_id);
    if (task_index >= 0 && process_table[task_index]->task_state == BLOCKED) {
        process_table[task_index]->task_state = READY;
    }
}

void Scheduler::dump(int level) const {
    std::cout << "\nTask Name\tTask ID\t\tState\t\tetc.\n";
    std::cout << "------------------------------------------------------------\n";

    for (const TCB* task : process_table) {
        std::cout << std::left << std::setw(16) << task->task_name
                  << std::setw(16) << task->task_id
                  << std::setw(16) << state_to_string(task->task_state);

        if (level > 0 && task->context_pointer != nullptr) {
            std::cout << "ctx=" << task->context_pointer;
        }

        std::cout << "\n";
    }
    std::cout << "------------------------------------------------------------\n";
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

bool Scheduler::has_active_tasks() const {
    for (const TCB* task : process_table) {
        if (task->task_state != DEAD) {
            return true;
        }
    }
    return false;
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
