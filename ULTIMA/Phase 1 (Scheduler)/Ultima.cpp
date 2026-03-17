#include <iostream>
#include <string>

#include "Sched.h"
#include "Sema.h"

/**
 * ULTIMA 2.0 - Phase 1 Main Driver
 * Designed by: Zander Hayes
 * Integrates Stewart's Scheduler and Nick's Semaphore components.
 */

Scheduler sys_scheduler;
Semaphore screen_mutex("Printer_Output", 1);

namespace {

int task_1_phase = 0;
int task_2_phase = 0;
int task_3_phase = 0;

void task_1_execution() {
    switch (task_1_phase) {
        case 0: {
            std::cout << "[Task 1] Starting execution. Attempting to acquire Printer...\n";
            screen_mutex.down();
            task_1_phase = 1;
            if (sys_scheduler.is_task_blocked(sys_scheduler.get_current_task_id())) {
                return;
            }
        }
            [[fallthrough]];
        case 1:
            std::cout << "[Task 1] Acquired Printer! Writing data...\n";
            task_1_phase = 2;
            sys_scheduler.yield();
            return;
        case 2:
            std::cout << "[Task 1] Finished writing. Releasing Printer...\n";
            screen_mutex.up();
            sys_scheduler.kill_task(sys_scheduler.get_current_task_id());
            task_1_phase = 3;
            return;
        default:
            return;
    }
}

void task_2_execution() {
    switch (task_2_phase) {
        case 0: {
            std::cout << "[Task 2] Starting execution. Attempting to acquire Printer...\n";
            screen_mutex.down();
            task_2_phase = 1;
            if (sys_scheduler.is_task_blocked(sys_scheduler.get_current_task_id())) {
                return;
            }
        }
            [[fallthrough]];
        case 1:
            std::cout << "[Task 2] Acquired Printer! Writing data...\n";
            screen_mutex.up();
            sys_scheduler.kill_task(sys_scheduler.get_current_task_id());
            task_2_phase = 2;
            return;
        default:
            return;
    }
}

void task_3_execution() {
    switch (task_3_phase) {
        case 0: {
            std::cout << "[Task 3] Starting execution. Attempting to acquire Printer...\n";
            screen_mutex.down();
            task_3_phase = 1;
            if (sys_scheduler.is_task_blocked(sys_scheduler.get_current_task_id())) {
                return;
            }
        }
            [[fallthrough]];
        case 1:
            std::cout << "[Task 3] Acquired Printer! Writing data...\n";
            screen_mutex.up();
            sys_scheduler.kill_task(sys_scheduler.get_current_task_id());
            task_3_phase = 2;
            return;
        default:
            return;
    }
}

} // namespace

int main() {
    std::cout << "=========================================\n";
    std::cout << "      ULTIMA 2.0 OS - INITIALIZING       \n";
    std::cout << "=========================================\n\n";

    const int t1 = sys_scheduler.create_task("Task_A", task_1_execution);
    const int t2 = sys_scheduler.create_task("Task_B", task_2_execution);
    const int t3 = sys_scheduler.create_task("Task_C", task_3_execution);
    (void) t1;
    (void) t2;
    (void) t3;

    std::cout << "--- INITIAL SYSTEM STATE ---\n";
    sys_scheduler.dump(1);
    screen_mutex.dump(1);

    std::cout << "--- BEGINNING EXECUTION ---\n";

    bool queue_dump_emitted = false;
    while (sys_scheduler.has_active_tasks()) {
        sys_scheduler.yield();

        if (!queue_dump_emitted && screen_mutex.waiting_task_count() >= 2) {
            std::cout << "\n--- MID-EXECUTION STATE DUMP (Proving Queueing) ---\n";
            sys_scheduler.dump(1);
            screen_mutex.dump(1);
            queue_dump_emitted = true;
        }
    }

    std::cout << "--- EXECUTION COMPLETE ---\n";
    sys_scheduler.garbage_collect();
    sys_scheduler.dump(1);
    screen_mutex.dump(1);

    std::cout << "System shutting down safely.\n";
    return 0;
}
