/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Creator: NICHOLAS KOBS - TEAM THUNDER */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#include "Sema.h"
#include <cstring>
#include <pthread.h>
#include <ncurses.h>

#include "Sched.h"

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: NICHOLAS KOBS
 */

Semaphore::Semaphore(const char* name, int initial_value) {
    std::strncpy(resource_name, name, sizeof(resource_name) - 1);
    resource_name[sizeof(resource_name) - 1] = '\0';

    sema_value = (initial_value > 0) ? 1 : 0;
    sema_queue = new std::queue<int>();
}

Semaphore::~Semaphore() {
    delete sema_queue;
}

void Semaphore::down() {
    if (sema_value == 1) {
        sema_value = 0;
    } else {
        const int current_tid = sys_scheduler.get_current_task_id();
        sema_queue->push(current_tid);
        sys_scheduler.block_task(current_tid);
        sys_scheduler.yield();
    }
}

void Semaphore::up() {
    if (sema_queue->empty()) {
        sema_value = 1;
    } else {
        const int next_tid = sema_queue->front();
        sema_queue->pop();
        sys_scheduler.unblock_task(next_tid);
    }
}

void Semaphore::dump(int level) const {
    (void) level;

    std::cout << "\nSemaphore Dump:\n";
    std::cout << "Resource:    " << resource_name << "\n";
    std::cout << "Sema_value:  " << sema_value << "\n";

    std::cout << "Sema_queue:  ";
    if (sema_queue->empty()) {
        std::cout << "[Empty]";
    } else {
        std::queue<int> temp_queue = *sema_queue;
        while (!temp_queue.empty()) {
            std::cout << "T-" << temp_queue.front();
            temp_queue.pop();
            if (!temp_queue.empty()) {
                std::cout << " --> ";
            }
        }
    }
    std::cout << "\n\n";
}

bool Semaphore::has_waiters() const {
    return !sema_queue->empty();
}

int Semaphore::waiting_task_count() const {
    return static_cast<int>(sema_queue->size());
}
