/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Creator: NICHOLAS KOBS - TEAM THUNDER */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#include "Sema.h"
#include <cstring>
#include <sstream>

#include "Sched.h"

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: NICHOLAS KOBS
 */

Semaphore::Semaphore(const char* name, int initial_value) {
    std::strncpy(resource_name, name, sizeof(resource_name) - 1);
    resource_name[sizeof(resource_name) - 1] = '\0';

    initial_sema_value = (initial_value > 0) ? 1 : 0;
    sema_queue = new std::queue<int>();
    reset();
}

Semaphore::~Semaphore() {
    delete sema_queue;
}

void Semaphore::reset() {
    while (!sema_queue->empty()) {
        sema_queue->pop();
    }

    sema_value = initial_sema_value;
    owner_task_id = -1;
    down_operations = 0;
    up_operations = 0;
    contention_events = 0;
    last_transition = "Semaphore reset. Resource is available.";
}

void Semaphore::down() {
    ++down_operations;

    const int current_tid = sys_scheduler.get_current_task_id();
    const std::string current_task_name = sys_scheduler.get_task_name(current_tid);

    if (sema_value == 1) {
        sema_value = 0;
        owner_task_id = current_tid;
        last_transition = current_task_name + " acquired the protected resource.";
    } else {
        ++contention_events;
        sema_queue->push(current_tid);
        sys_scheduler.block_task(current_tid);

        std::ostringstream transition_stream;
        transition_stream << current_task_name
                          << " encountered a busy resource and joined the FIFO wait queue: "
                          << describe_wait_queue() << ".";
        last_transition = transition_stream.str();

        sys_scheduler.yield();
    }
}

void Semaphore::up() {
    ++up_operations;

    if (sema_queue->empty()) {
        sema_value = 1;
        owner_task_id = -1;
        last_transition = std::string(resource_name) + " was released; no tasks were waiting.";
    } else {
        const int next_tid = sema_queue->front();
        sema_queue->pop();
        sys_scheduler.unblock_task(next_tid);
        owner_task_id = next_tid;

        std::ostringstream transition_stream;
        transition_stream << sys_scheduler.get_task_name(next_tid)
                          << " was woken from the wait queue and is now the next owner.";
        last_transition = transition_stream.str();
    }
}

void Semaphore::dump(int level) const {
    std::cout << "\nSemaphore Dump:\n";
    std::cout << "Resource:    " << resource_name << "\n";
    std::cout << "Purpose:     Prevent the Printer_Output race by allowing one owner at a time.\n";
    std::cout << "Sema_value:  " << sema_value << (sema_value == 1 ? " (AVAILABLE)" : " (BUSY)") << "\n";
    std::cout << "Owner:       ";

    if (owner_task_id >= 0) {
        std::cout << sys_scheduler.get_task_name(owner_task_id)
                  << " (T-" << owner_task_id << ")";
    } else {
        std::cout << "[None]";
    }
    std::cout << "\n";

    std::cout << "Waiters:     " << waiting_task_count() << "\n";
    std::cout << "Sema_queue:  " << describe_wait_queue() << "\n";

    if (level > 0) {
        std::cout << "Operations:  down=" << down_operations
                  << " up=" << up_operations
                  << " contention=" << contention_events << "\n";
        std::cout << "LastEvent:   " << last_transition << "\n";
    }

    std::cout << "\n";
}

bool Semaphore::has_waiters() const {
    return !sema_queue->empty();
}

int Semaphore::waiting_task_count() const {
    return static_cast<int>(sema_queue->size());
}

int Semaphore::get_owner_task_id() const {
    return owner_task_id;
}

int Semaphore::get_sema_value() const {
    return sema_value;
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

std::string Semaphore::describe_wait_queue() const {
    if (sema_queue->empty()) {
        return "[Empty]";
    }

    std::ostringstream queue_stream;
    std::queue<int> temp_queue = *sema_queue;

    while (!temp_queue.empty()) {
        const int task_id = temp_queue.front();
        temp_queue.pop();

        queue_stream << "T-" << task_id;
        if (!temp_queue.empty()) {
            queue_stream << " --> ";
        }
    }

    return queue_stream.str();
}

std::string Semaphore::get_resource_name() const {
    return resource_name;
}
