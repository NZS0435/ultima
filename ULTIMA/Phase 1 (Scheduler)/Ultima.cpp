/*
 * PHASE 1 - ULTIMA 2.0 - TEAM THUNDER
 *
 *                 .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *             __.'             ~.   .~             `.__
 *           .'//                 \./                 \\`.
 *         .'//   PHASE 1 CLOUD    |   CODE RAIN       \\`.
 *       .'//______________________|_____________________\\`.
 *              || 01 01 01 01 01 01 01 01 01 ||
 *              || 10 10 10 10 10 10 10 10 10 ||
 *              || 01 01 01 01 01 01 01 01 01 ||
 *
 * Creator: ZANDER HAYES - TEAM THUNDER
 * Phase Label: Scheduler and Semaphore
 */

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <ncurses.h>
#include "Sched.h"
#include "Sema.h"
#include "U2_UI.h"
#include "U2_Window.h"

/**
 * ULTIMA 2.0 - Phase 1 Main Driver
 * Tailored to the scheduler and semaphore assignment while preserving
 * a terminal-window presentation for screenshots and live demonstration.
 */

Scheduler sys_scheduler;
Semaphore printer_semaphore("Printer_Output", 1);
U2_ui ui_manager;

U2_window* header_window = nullptr;
U2_window* task_window_a = nullptr;
U2_window* task_window_b = nullptr;
U2_window* task_window_c = nullptr;
U2_window* log_window = nullptr;
U2_window* console_window = nullptr;

namespace {

constexpr int kMinimumTerminalRows = 24;
constexpr int kMinimumTerminalCols = 80;
constexpr int kHorizontalMargin = 1;
constexpr int kHorizontalGap = 1;
constexpr int kVerticalGap = 1;

struct WindowLayout {
    int header_height = 0;
    int header_width = 0;
    int header_y = 0;
    int header_x = 0;

    int task_height = 0;
    int task_y = 0;
    int task_width_a = 0;
    int task_width_b = 0;
    int task_width_c = 0;
    int task_x_a = 0;
    int task_x_b = 0;
    int task_x_c = 0;

    int bottom_height = 0;
    int bottom_y = 0;
    int log_width = 0;
    int log_x = 0;
    int console_width = 0;
    int console_x = 0;
};

WindowLayout current_layout;

int task_a_id = -1;
int task_b_id = -1;
int task_c_id = -1;

int task_a_phase = 0;
int task_b_phase = 0;
int task_c_phase = 0;

bool task_b_was_blocked = false;
bool task_c_was_blocked = false;
bool mid_execution_dump_written = false;
bool post_release_dump_written = false;
bool close_when_demo_finishes = false;

std::string console_status = "Phase 1 demo ready.";

bool build_layout(WindowLayout& layout) {
    if (LINES < kMinimumTerminalRows || COLS < kMinimumTerminalCols) {
        return false;
    }

    layout.header_y = 0;
    layout.header_x = kHorizontalMargin;
    layout.header_height = 5;
    layout.header_width = COLS - (kHorizontalMargin * 2);

    layout.task_y = layout.header_y + layout.header_height;

    const int total_task_width = COLS - (kHorizontalMargin * 2) - (kHorizontalGap * 2);
    const int base_task_width = total_task_width / 3;
    const int task_width_remainder = total_task_width - (base_task_width * 3);

    layout.task_width_a = base_task_width;
    layout.task_width_b = base_task_width;
    layout.task_width_c = base_task_width + task_width_remainder;

    layout.task_x_a = kHorizontalMargin;
    layout.task_x_b = layout.task_x_a + layout.task_width_a + kHorizontalGap;
    layout.task_x_c = layout.task_x_b + layout.task_width_b + kHorizontalGap;

    const int rows_below_task_row = LINES - layout.task_y;
    layout.task_height = (rows_below_task_row - kVerticalGap) / 2;
    layout.bottom_y = layout.task_y + layout.task_height + kVerticalGap;
    layout.bottom_height = LINES - layout.bottom_y;

    const int total_bottom_width = COLS - (kHorizontalMargin * 2) - kHorizontalGap;
    layout.console_width = std::clamp(total_bottom_width / 4, 20, 24);
    layout.log_width = total_bottom_width - layout.console_width;
    layout.log_x = kHorizontalMargin;
    layout.console_x = layout.log_x + layout.log_width + kHorizontalGap;

    return layout.header_height >= 5
        && layout.task_height >= 8
        && layout.bottom_height >= 8
        && layout.log_width >= 40
        && layout.console_width >= 20;
}

void write_block(U2_window* target_window, const std::string& block) {
    if (target_window != nullptr) {
        target_window->write_text(block.c_str());
        ui_manager.refresh_all();
    }
}

void write_line(U2_window* target_window, const std::string& line) {
    write_block(target_window, line + "\n");
}

std::string capture_dump_text(const std::function<void()>& dump_function) {
    std::ostringstream capture_stream;
    std::streambuf* original_buffer = std::cout.rdbuf(capture_stream.rdbuf());
    dump_function();
    std::cout.rdbuf(original_buffer);
    return capture_stream.str();
}

int console_status_row() {
    return std::max(3, current_layout.bottom_height - 5);
}

int console_footer_row() {
    return std::max(4, current_layout.bottom_height - 4);
}

void render_header() {
    if (header_window == nullptr) {
        return;
    }

    header_window->clear_window();
    header_window->write_text_at(0, 0, "PHASE I - Scheduler and Semaphore");
    header_window->write_text_at(1, 0, "Dynamic process table, strict round robin, and binary semaphore queueing.");
    header_window->write_text_at(2, 0, "Terminal windows remain for screenshots while the Phase 1 proof runs inside them.");
    ui_manager.refresh_all();
}

void render_console() {
    if (console_window == nullptr) {
        return;
    }

    console_window->clear_window();
    console_window->write_text_at(0, 0, "h help | d dump");
    console_window->write_text_at(1, 0, "q close later");
    console_window->write_text_at(2, 0, "mouse logs");

    const std::string active_line = "Active: " + std::to_string(sys_scheduler.get_active_task_count());
    console_window->write_text_at(console_status_row(), 0, active_line.c_str());
    console_window->write_text_at(console_footer_row(), 0, console_status.c_str());
    ui_manager.refresh_all();
}

void set_console_status(const std::string& status_line) {
    console_status = status_line;
    render_console();
}

void log_event(const std::string& message) {
    write_line(log_window, message);
}

void show_system_snapshot(const std::string& title) {
    log_event("");
    log_event(title);
    write_block(log_window, capture_dump_text([] {
        sys_scheduler.dump(0);
    }));
    write_block(log_window, capture_dump_text([] {
        printer_semaphore.dump(0);
    }));
    log_event("");
}

bool current_task_is_blocked() {
    const int current_task_id = sys_scheduler.get_current_task_id();
    return current_task_id >= 0 && sys_scheduler.is_task_blocked(current_task_id);
}

void initialize_task_windows() {
    write_line(task_window_a, "Task_A created in READY state.");
    write_line(task_window_a, "Task ID = " + std::to_string(task_a_id));

    write_line(task_window_b, "Task_B created in READY state.");
    write_line(task_window_b, "Task ID = " + std::to_string(task_b_id));

    write_line(task_window_c, "Task_C created in READY state.");
    write_line(task_window_c, "Task ID = " + std::to_string(task_c_id));
}

void task_a_execution() {
    switch (task_a_phase) {
        case 0: {
            write_line(task_window_a, "Requesting Printer_Output.");
            log_event("[Task_A] down(Printer_Output)");
            set_console_status("Task_A is requesting the printer.");

            printer_semaphore.down();
            task_a_phase = 1;

            if (current_task_is_blocked()) {
                write_line(task_window_a, "Unexpected block encountered.");
                return;
            }

            write_line(task_window_a, "Acquired Printer_Output.");
            write_line(task_window_a, "Holding resource for one quantum.");
            log_event("[Task_A] acquired Printer_Output.");
            set_console_status("Task_A is RUNNING.");
            sys_scheduler.yield();
            return;
        }
        case 1:
            write_line(task_window_a, "Releasing Printer_Output.");
            log_event("[Task_A] up(Printer_Output)");
            printer_semaphore.up();

            if (!post_release_dump_written) {
                show_system_snapshot("--- POST-RELEASE STATE DUMP ---");
                post_release_dump_written = true;
            }

            sys_scheduler.kill_task(sys_scheduler.get_current_task_id());
            write_line(task_window_a, "Task_A marked DEAD.");
            log_event("[Task_A] marked DEAD.");
            set_console_status("Task_A finished.");
            task_a_phase = 2;
            return;
        default:
            return;
    }
}

void task_b_execution() {
    switch (task_b_phase) {
        case 0: {
            write_line(task_window_b, "Requesting Printer_Output.");
            log_event("[Task_B] down(Printer_Output)");
            set_console_status("Task_B is requesting the printer.");

            printer_semaphore.down();
            task_b_phase = 1;

            if (current_task_is_blocked()) {
                task_b_was_blocked = true;
                write_line(task_window_b, "Blocked and queued by semaphore.");
                log_event("[Task_B] transitioned to BLOCKED.");
                set_console_status("Task_B is BLOCKED.");
                return;
            }
        }
            [[fallthrough]];
        case 1:
            if (task_b_was_blocked) {
                write_line(task_window_b, "Woken from sema_queue.");
            }
            write_line(task_window_b, "Acquired Printer_Output.");
            log_event("[Task_B] acquired Printer_Output.");
            set_console_status("Task_B is RUNNING.");
            task_b_phase = 2;
            sys_scheduler.yield();
            return;
        case 2:
            write_line(task_window_b, "Releasing Printer_Output.");
            log_event("[Task_B] up(Printer_Output)");
            printer_semaphore.up();
            sys_scheduler.kill_task(sys_scheduler.get_current_task_id());
            write_line(task_window_b, "Task_B marked DEAD.");
            log_event("[Task_B] marked DEAD.");
            set_console_status("Task_B finished.");
            task_b_phase = 3;
            return;
        default:
            return;
    }
}

void task_c_execution() {
    switch (task_c_phase) {
        case 0: {
            write_line(task_window_c, "Requesting Printer_Output.");
            log_event("[Task_C] down(Printer_Output)");
            set_console_status("Task_C is requesting the printer.");

            printer_semaphore.down();
            task_c_phase = 1;

            if (current_task_is_blocked()) {
                task_c_was_blocked = true;
                write_line(task_window_c, "Blocked and queued by semaphore.");
                log_event("[Task_C] transitioned to BLOCKED.");
                set_console_status("Task_C is BLOCKED.");

                if (!mid_execution_dump_written) {
                    show_system_snapshot("--- MID-EXECUTION STATE DUMP (Proving Queueing) ---");
                    mid_execution_dump_written = true;
                }
                return;
            }
        }
            [[fallthrough]];
        case 1:
            if (task_c_was_blocked) {
                write_line(task_window_c, "Woken from sema_queue.");
            }
            write_line(task_window_c, "Acquired Printer_Output.");
            log_event("[Task_C] acquired Printer_Output.");
            set_console_status("Task_C is RUNNING.");
            task_c_phase = 2;
            sys_scheduler.yield();
            return;
        case 2:
            write_line(task_window_c, "Releasing Printer_Output.");
            log_event("[Task_C] up(Printer_Output)");
            printer_semaphore.up();
            sys_scheduler.kill_task(sys_scheduler.get_current_task_id());
            write_line(task_window_c, "Task_C marked DEAD.");
            log_event("[Task_C] marked DEAD.");
            set_console_status("Task_C finished.");
            task_c_phase = 3;
            return;
        default:
            return;
    }
}

void handle_console_input() {
    if (console_window == nullptr) {
        return;
    }

    const int input = wgetch(console_window->get_win_ptr());
    if (input == ERR) {
        return;
    }

    switch (input) {
        case 'h':
        case 'H':
            set_console_status("Help refreshed.");
            break;
        case 'd':
        case 'D':
            show_system_snapshot("--- ON-DEMAND SYSTEM DUMP ---");
            set_console_status("Manual dump captured.");
            break;
        case 'q':
        case 'Q':
            close_when_demo_finishes = true;
            set_console_status("Window will close when the demo ends.");
            break;
        case KEY_MOUSE: {
            MEVENT event {};
            if (getmouse(&event) == OK) {
                std::ostringstream mouse_line;
                mouse_line << "[Mouse] row=" << event.y << ", col=" << event.x;
                log_event(mouse_line.str());
                set_console_status("Mouse event logged.");
            }
            break;
        }
        default:
            break;
    }
}

} // namespace

int main() {
    ui_manager.init_ncurses_env();

    if (!build_layout(current_layout)) {
        ui_manager.close_ncurses_env();
        std::cerr << "Resize the terminal to at least "
                  << kMinimumTerminalCols
                  << " columns by "
                  << kMinimumTerminalRows
                  << " rows and rerun ultima_os."
                  << std::endl;
        return 1;
    }

    header_window = new U2_window(
        current_layout.header_height,
        current_layout.header_width,
        current_layout.header_y,
        current_layout.header_x,
        "ULTIMA 2.0 - Phase 1",
        false
    );
    task_window_a = new U2_window(
        current_layout.task_height,
        current_layout.task_width_a,
        current_layout.task_y,
        current_layout.task_x_a,
        "Task_A",
        true
    );
    task_window_b = new U2_window(
        current_layout.task_height,
        current_layout.task_width_b,
        current_layout.task_y,
        current_layout.task_x_b,
        "Task_B",
        true
    );
    task_window_c = new U2_window(
        current_layout.task_height,
        current_layout.task_width_c,
        current_layout.task_y,
        current_layout.task_x_c,
        "Task_C",
        true
    );
    log_window = new U2_window(
        current_layout.bottom_height,
        current_layout.log_width,
        current_layout.bottom_y,
        current_layout.log_x,
        "Scheduler + Semaphore Log",
        true
    );
    console_window = new U2_window(
        current_layout.bottom_height,
        current_layout.console_width,
        current_layout.bottom_y,
        current_layout.console_x,
        "Controls",
        false
    );

    ui_manager.add_window(header_window);
    ui_manager.add_window(task_window_a);
    ui_manager.add_window(task_window_b);
    ui_manager.add_window(task_window_c);
    ui_manager.add_window(log_window);
    ui_manager.add_window(console_window);
    ui_manager.refresh_all();

    keypad(console_window->get_win_ptr(), TRUE);
    nodelay(console_window->get_win_ptr(), TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0);

    render_header();
    render_console();

    task_a_id = sys_scheduler.create_task("Task_A", task_a_execution);
    task_b_id = sys_scheduler.create_task("Task_B", task_b_execution);
    task_c_id = sys_scheduler.create_task("Task_C", task_c_execution);

    initialize_task_windows();
    log_event("=== ULTIMA Phase 1 terminal demonstration ===");
    log_event("Three tasks compete for Printer_Output through a binary semaphore.");
    show_system_snapshot("--- INITIAL SYSTEM STATE ---");
    log_event("--- BEGINNING EXECUTION ---");
    set_console_status("Scheduler run started.");

    while (sys_scheduler.has_active_tasks()) {
        handle_console_input();
        sys_scheduler.yield();
        ui_manager.refresh_all();
        napms(220);
    }

    show_system_snapshot("--- FINAL STATE DUMP (Before Garbage Collection) ---");
    sys_scheduler.garbage_collect();
    show_system_snapshot("--- POST-GARBAGE-COLLECTION DUMP ---");
    log_event("System shutting down safely.");

    if (close_when_demo_finishes) {
        ui_manager.close_ncurses_env();
        return 0;
    }

    nodelay(console_window->get_win_ptr(), FALSE);
    set_console_status("Phase 1 complete. Press any key to exit.");
    wgetch(console_window->get_win_ptr());

    ui_manager.close_ncurses_env();
    return 0;
}
