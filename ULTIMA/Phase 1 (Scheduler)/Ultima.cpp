#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <ncurses.h>

#include "Sched.h"
#include "Sema.h"
#include "U2_UI.h"
#include "U2_Window.h"

/**
 * ULTIMA 2.0 - Phase 1 Main Driver
 * Designed by: Zander Hayes
 * Integrates Stewart's Scheduler and Nick's Semaphore components.
 * UI enhancements added for U2_UI and U2_Window demonstration.
 */

Scheduler sys_scheduler;
Semaphore screen_mutex("Printer_Output", 1);
U2_ui ui_manager;
U2_window* log_window = nullptr;

namespace {

int task_1_phase = 0;
int task_2_phase = 0;
int task_3_phase = 0;

void log_to_window(const std::string& message) {
    if (log_window) {
        log_window->write_text((message + "\n").c_str());
    }
    // Also log to stdout for debugging if needed, but ncurses might hide it.
}

void task_1_execution() {
    switch (task_1_phase) {
        case 0: {
            log_to_window("[Task 1] Starting execution. Attempting to acquire Printer...");
            screen_mutex.down();
            task_1_phase = 1;
            if (sys_scheduler.is_task_blocked(sys_scheduler.get_current_task_id())) {
                return;
            }
        }
            [[fallthrough]];
        case 1:
            log_to_window("[Task 1] Acquired Printer! Writing data...");
            task_1_phase = 2;
            sys_scheduler.yield();
            return;
        case 2:
            log_to_window("[Task 1] Finished writing. Releasing Printer...");
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
            log_to_window("[Task 2] Starting execution. Attempting to acquire Printer...");
            screen_mutex.down();
            task_2_phase = 1;
            if (sys_scheduler.is_task_blocked(sys_scheduler.get_current_task_id())) {
                return;
            }
        }
            [[fallthrough]];
        case 1:
            log_to_window("[Task 2] Acquired Printer! Writing data...");
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
            log_to_window("[Task 3] Starting execution. Attempting to acquire Printer...");
            screen_mutex.down();
            task_3_phase = 1;
            if (sys_scheduler.is_task_blocked(sys_scheduler.get_current_task_id())) {
                return;
            }
        }
            [[fallthrough]];
        case 1:
            log_to_window("[Task 3] Acquired Printer! Writing data...");
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
    ui_manager.init_ncurses_env();

    // Create a window for our execution log
    // Parameters: height, width, starty, startx, title, scrollable
    log_window = new U2_window(20, 60, 2, 5, "ULTIMA 2.0 - OS LOG", true);
    ui_manager.add_window(log_window);

    log_to_window("=========================================");
    log_to_window("      ULTIMA 2.0 OS - INITIALIZING       ");
    log_to_window("=========================================");

    const int t1 = sys_scheduler.create_task("Task_A", task_1_execution);
    const int t2 = sys_scheduler.create_task("Task_B", task_2_execution);
    const int t3 = sys_scheduler.create_task("Task_C", task_3_execution);
    (void) t1;
    (void) t2;
    (void) t3;

    log_to_window("--- INITIAL SYSTEM STATE SET ---");
    log_to_window("--- BEGINNING EXECUTION ---");

    while (sys_scheduler.has_active_tasks()) {
        sys_scheduler.yield();
        // Brief pause to allow human reading of the UI
        napms(200);
    }

    log_to_window("--- EXECUTION COMPLETE ---");
    sys_scheduler.garbage_collect();
    log_to_window("System shutting down safely. Press any key to exit.");
    
    // Refresh to show final state
    ui_manager.refresh_all();
    
    // Wait for user input before closing UI
    getch();

    ui_manager.close_ncurses_env();
    return 0;
}
