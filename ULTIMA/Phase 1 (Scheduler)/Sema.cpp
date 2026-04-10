#include "Sema.h"

#include <iostream>
#include <sstream>

Semaphore::Semaphore(const std::string& resource, int init_val, Scheduler* sched) :
    resource_name(resource),
    initial_value(init_val),
    value(init_val),
    scheduler(sched),
    owner_task_id(-1),
    down_operations(0),
    up_operations(0),
    contention_events(0),
    last_transition("Semaphore initialized and available.") {}

void Semaphore::reset() {
    value = initial_value;
    wait_queue.clear();
    owner_task_id = -1;
    down_operations = 0;
    up_operations = 0;
    contention_events = 0;
    last_transition = "Semaphore reset and available.";
}

void Semaphore::down() {
    ++down_operations;

    const int current_task_id = (scheduler != nullptr)
        ? scheduler->get_current_task_id()
        : -1;

    if (value > 0 && owner_task_id < 0) {
        --value;
        owner_task_id = current_task_id;

        std::ostringstream transition_stream;
        transition_stream << resource_name << " acquired by T-" << current_task_id << ".";
        last_transition = transition_stream.str();
        return;
    }

    ++contention_events;

    if (current_task_id >= 0) {
        wait_queue.push_back(current_task_id);
    }

    std::ostringstream transition_stream;
    transition_stream << resource_name << " busy; queued T-" << current_task_id << '.';
    last_transition = transition_stream.str();

    if (scheduler != nullptr && current_task_id >= 0) {
        scheduler->block_current_task();
    }
}

void Semaphore::up() {
    ++up_operations;

    const int releasing_task_id = (scheduler != nullptr)
        ? scheduler->get_current_task_id()
        : -1;

    if (wait_queue.empty()) {
        owner_task_id = -1;
        value = initial_value;

        std::ostringstream transition_stream;
        transition_stream << resource_name << " released by T-" << releasing_task_id
                          << "; resource is now available.";
        last_transition = transition_stream.str();
        return;
    }

    const int next_task_id = wait_queue.front();
    wait_queue.pop_front();

    owner_task_id = next_task_id;
    value = 0;

    if (scheduler != nullptr) {
        scheduler->unblock_task(next_task_id);
    }

    std::ostringstream transition_stream;
    transition_stream << resource_name << " released by T-" << releasing_task_id
                      << "; woke T-" << next_task_id << '.';
    last_transition = transition_stream.str();
}

void Semaphore::P() {
    down();
}

void Semaphore::V() {
    up();
}

void Semaphore::dump(int level) const {
    std::cout << "Semaphore Dump\n";
    std::cout << "==============\n";
    std::cout << "Resource: " << resource_name << '\n';
    std::cout << "Value: " << value << '\n';
    std::cout << "Owner: " << owner_task_id << '\n';
    std::cout << "Wait Queue: " << describe_wait_queue() << '\n';
    std::cout << "Operations: down=" << down_operations
              << " up=" << up_operations
              << " contention=" << contention_events
              << '\n';

    if (level > 0) {
        std::cout << "Last Transition: " << last_transition << '\n';
    }

    std::cout << '\n';
}

int Semaphore::get_sema_value() const {
    return value;
}

int Semaphore::get_owner_task_id() const {
    return owner_task_id;
}

std::string Semaphore::get_resource_name() const {
    return resource_name;
}

int Semaphore::waiting_task_count() const {
    return static_cast<int>(wait_queue.size());
}

std::string Semaphore::describe_wait_queue() const {
    if (wait_queue.empty()) {
        return "[empty]";
    }

    std::ostringstream queue_stream;

    for (std::size_t index = 0; index < wait_queue.size(); ++index) {
        const int task_id = wait_queue[index];
        if (index > 0) {
            queue_stream << " -> ";
        }

        if (scheduler != nullptr) {
            queue_stream << scheduler->get_task_name(task_id)
                         << "(T-" << task_id << ")";
        } else {
            queue_stream << "T-" << task_id;
        }
    }

    return queue_stream.str();
}

int Semaphore::get_down_operations() const {
    return down_operations;
}

int Semaphore::get_up_operations() const {
    return up_operations;
}

int Semaphore::get_contention_events() const {
    return contention_events;
}

std::string Semaphore::get_last_transition() const {
    return last_transition;
}
