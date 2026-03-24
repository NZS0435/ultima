/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Creator: ZANDER HAYES - TEAM THUNDER */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "platform_curses.h"
#include "U2_Scheduler.h"
#include "Sema.h"
#include "U2_UI.h"
#include "U2_Window.h"

/**
 * ULTIMA 2.0 - Phase 1 Main Driver
 * The presentation layer intentionally keeps the scheduler, semaphore, state,
 * race-condition proof, and per-task traces visible at the same time so the
 * Phase 1 requirements are easy to explain in screenshots and a live demo.
 */

Scheduler sys_scheduler;
Semaphore printer_semaphore("Printer_Output", 1, &sys_scheduler);
U2_ui ui_manager;

U2_window* header_window = nullptr;
U2_window* scheduler_window = nullptr;
U2_window* semaphore_window = nullptr;
U2_window* state_window = nullptr;
U2_window* task_window_a = nullptr;
U2_window* task_window_b = nullptr;
U2_window* task_window_c = nullptr;
U2_window* log_window = nullptr;
U2_window* console_window = nullptr;

namespace {

constexpr int kMinimumTerminalRows = 38;
constexpr int kMinimumTerminalCols = 120;
constexpr int kHorizontalMargin = 1;
constexpr int kHorizontalGap = 1;
constexpr int kVerticalGap = 1;
constexpr int kStepPauseMs = 900;
constexpr int kDispatchPauseMs = 250;
constexpr int kDumpPauseMs = 1500;
constexpr int kCycleRestartPauseMs = 2200;
constexpr std::size_t kMaxHistoryLines = 18;
constexpr std::size_t kMaxStateTraceLines = 12;

struct WindowLayout {
    int header_height = 0;
    int header_width = 0;
    int header_y = 0;
    int header_x = 0;

    int class_height = 0;
    int class_y = 0;
    int class_width_left = 0;
    int class_width_middle = 0;
    int class_width_right = 0;
    int class_x_left = 0;
    int class_x_middle = 0;
    int class_x_right = 0;

    int task_height = 0;
    int task_y = 0;
    int task_width_left = 0;
    int task_width_middle = 0;
    int task_width_right = 0;
    int task_x_left = 0;
    int task_x_middle = 0;
    int task_x_right = 0;

    int bottom_height = 0;
    int bottom_y = 0;
    int log_width = 0;
    int log_x = 0;
    int console_width = 0;
    int console_x = 0;
};

WindowLayout current_layout;

bool transcript_only_mode = false;
bool stop_after_cycle = false;
bool demo_paused = false;

int demo_cycle_number = 1;

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

std::string console_status = "Phase 1 demo ready.";
std::string latest_flow_summary = "No scheduler activity has executed yet.";

std::vector<std::string> transcript_lines;
std::vector<std::string> state_trace_lines;
std::vector<std::string> task_a_history;
std::vector<std::string> task_b_history;
std::vector<std::string> task_c_history;

bool build_layout(WindowLayout& layout) {
    if (LINES < kMinimumTerminalRows || COLS < kMinimumTerminalCols) {
        return false;
    }

    layout.header_y = 0;
    layout.header_x = kHorizontalMargin;
    layout.header_height = 6;
    layout.header_width = COLS - (kHorizontalMargin * 2);

    const int usable_width = COLS - (kHorizontalMargin * 2) - (kHorizontalGap * 2);
    const int column_width = usable_width / 3;
    const int column_remainder = usable_width - (column_width * 3);

    layout.class_y = layout.header_height;
    layout.bottom_height = std::max(10, LINES / 3);

    const int middle_height = LINES - layout.header_height - layout.bottom_height - (kVerticalGap * 2);
    layout.class_height = std::max(10, middle_height / 2);
    layout.task_height = middle_height - layout.class_height;
    layout.task_y = layout.class_y + layout.class_height + kVerticalGap;
    layout.bottom_y = layout.task_y + layout.task_height + kVerticalGap;

    layout.class_width_left = column_width;
    layout.class_width_middle = column_width;
    layout.class_width_right = column_width + column_remainder;
    layout.class_x_left = kHorizontalMargin;
    layout.class_x_middle = layout.class_x_left + layout.class_width_left + kHorizontalGap;
    layout.class_x_right = layout.class_x_middle + layout.class_width_middle + kHorizontalGap;

    layout.task_width_left = layout.class_width_left;
    layout.task_width_middle = layout.class_width_middle;
    layout.task_width_right = layout.class_width_right;
    layout.task_x_left = layout.class_x_left;
    layout.task_x_middle = layout.class_x_middle;
    layout.task_x_right = layout.class_x_right;

    const int bottom_width = COLS - (kHorizontalMargin * 2) - kHorizontalGap;
    layout.console_width = std::clamp(bottom_width / 4, 28, 34);
    layout.log_width = bottom_width - layout.console_width;
    layout.log_x = kHorizontalMargin;
    layout.console_x = layout.log_x + layout.log_width + kHorizontalGap;

    return layout.class_height >= 10
        && layout.task_height >= 10
        && layout.bottom_height >= 10
        && layout.log_width >= 70;
}

std::string state_to_text(State state) {
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

void push_bounded_line(std::vector<std::string>& lines, const std::string& line, std::size_t max_lines) {
    lines.push_back(line);
    if (lines.size() > max_lines) {
        lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(lines.size() - max_lines));
    }
}

void append_transcript(const std::string& line) {
    transcript_lines.push_back(line);
}

void add_state_trace(const std::string& line) {
    push_bounded_line(state_trace_lines, line, kMaxStateTraceLines);
}

void add_task_history(std::vector<std::string>& history, const std::string& line) {
    push_bounded_line(history, line, kMaxHistoryLines);
}

std::string build_transcript_text() {
    std::ostringstream transcript_stream;
    transcript_stream << "ULTIMA 2.0 - Phase 1 transcript\n";
    transcript_stream << "================================\n";

    for (const std::string& line : transcript_lines) {
        transcript_stream << line << '\n';
    }

    return transcript_stream.str();
}

std::string capture_dump_text(const std::function<void()>& dump_function) {
    std::ostringstream capture_stream;
    std::streambuf* original_buffer = std::cout.rdbuf(capture_stream.rdbuf());
    dump_function();
    std::cout.rdbuf(original_buffer);
    return capture_stream.str();
}

const TaskSnapshot* find_snapshot(const std::vector<TaskSnapshot>& snapshots, int task_id) {
    const auto match = std::find_if(snapshots.begin(), snapshots.end(), [task_id](const TaskSnapshot& snapshot) {
        return snapshot.task_id == task_id;
    });

    return (match != snapshots.end()) ? &(*match) : nullptr;
}

std::string format_task_id(int task_id) {
    return (task_id >= 0) ? "T-" + std::to_string(task_id) : "[not created]";
}

std::string format_owner_label() {
    const int owner_task_id = printer_semaphore.get_owner_task_id();
    if (owner_task_id < 0) {
        return "[None]";
    }

    std::ostringstream owner_stream;
    owner_stream << sys_scheduler.get_task_name(owner_task_id)
                 << " (" << format_task_id(owner_task_id) << ")";
    return owner_stream.str();
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

void render_header() {
    if (header_window == nullptr) {
        return;
    }

    header_window->draw_lines({
        "ULTIMA 2.0 / Phase 1 - Scheduler and Semaphore",
        "Purpose: the scheduler scans READY tasks, dispatches one RUNNING task, and keeps BLOCKED tasks off the CPU.",
        "Purpose: the semaphore prevents a Printer_Output race by forcing FIFO blocking instead of busy waiting.",
        "Cycle " + std::to_string(demo_cycle_number) + " | Flow: " + latest_flow_summary
    });
}

void render_scheduler_panel() {
    if (scheduler_window == nullptr) {
        return;
    }

    const std::vector<TaskSnapshot> snapshots = sys_scheduler.snapshot_tasks();
    std::vector<std::string> lines {
        "Purpose: select the next READY task with strict round robin.",
        "Current: " + format_task_id(sys_scheduler.get_current_task_id()) + " | Tick: " + std::to_string(sys_scheduler.get_scheduler_tick()),
        "Counts: READY=" + std::to_string(sys_scheduler.get_ready_task_count())
            + " BLOCKED=" + std::to_string(sys_scheduler.get_blocked_task_count())
            + " DEAD=" + std::to_string(sys_scheduler.get_dead_task_count())
            + " ACTIVE=" + std::to_string(sys_scheduler.get_active_task_count()),
        "Queue: " + sys_scheduler.describe_run_queue(),
        "Last: " + sys_scheduler.get_last_scheduler_event()
    };

    for (const TaskSnapshot& snapshot : snapshots) {
        std::ostringstream row_stream;
        row_stream << snapshot.task_name
                   << " "
                   << format_task_id(snapshot.task_id)
                   << " "
                   << state_to_text(snapshot.task_state)
                   << " run=" << snapshot.dispatch_count
                   << " yld=" << snapshot.yield_count
                   << " blk=" << snapshot.block_count
                   << " wake=" << snapshot.unblock_count;
        lines.push_back(row_stream.str());
    }

    scheduler_window->draw_lines(lines);
}

void render_semaphore_panel() {
    if (semaphore_window == nullptr) {
        return;
    }

    std::ostringstream value_stream;
    value_stream << "Value: " << printer_semaphore.get_sema_value()
                 << (printer_semaphore.get_sema_value() == 1 ? " (AVAILABLE)" : " (BUSY)");

    std::vector<std::string> lines {
        "Purpose: protect Printer_Output and serialize access to the critical section.",
        "Resource: " + printer_semaphore.get_resource_name(),
        value_stream.str(),
        "Owner: " + format_owner_label(),
        "Waiters: " + std::to_string(printer_semaphore.waiting_task_count())
            + " | Queue: " + printer_semaphore.describe_wait_queue(),
        "Ops: down=" + std::to_string(printer_semaphore.get_down_operations())
            + " up=" + std::to_string(printer_semaphore.get_up_operations())
            + " contention=" + std::to_string(printer_semaphore.get_contention_events()),
        "Last: " + printer_semaphore.get_last_transition(),
        "Rule: only the owner may enter Printer_Output."
    };

    semaphore_window->draw_lines(lines);
}

void render_state_panel() {
    if (state_window == nullptr) {
        return;
    }

    std::vector<std::string> lines {
        "State legend: READY -> RUNNING -> BLOCKED -> READY -> DEAD",
        "Race control: ncurses writes are serialized with a pthread mutex.",
        "Resource control: the semaphore turns contention into queue-based blocking.",
        "Flow proof: " + latest_flow_summary,
        "Recent transitions:"
    };

    const int visible_trace_lines = std::max(0, state_window->inner_height() - static_cast<int>(lines.size()));
    const std::size_t start_index = (state_trace_lines.size() > static_cast<std::size_t>(visible_trace_lines))
        ? state_trace_lines.size() - static_cast<std::size_t>(visible_trace_lines)
        : 0;

    for (std::size_t index = start_index; index < state_trace_lines.size(); ++index) {
        lines.push_back(state_trace_lines[index]);
    }

    state_window->draw_lines(lines);
}

std::vector<std::string> build_task_panel_lines(
    const std::vector<TaskSnapshot>& snapshots,
    const std::string& role,
    int task_id,
    const std::vector<std::string>& history
) {
    const TaskSnapshot* snapshot = find_snapshot(snapshots, task_id);

    std::vector<std::string> lines {
        "Role: " + role,
        "Task ID: " + format_task_id(task_id),
        "State: " + (snapshot != nullptr ? state_to_text(snapshot->task_state) : "NOT PRESENT"),
        "Note: " + (snapshot != nullptr ? snapshot->detail_note : "Awaiting creation."),
        "Recent activity:"
    };

    const int visible_history_lines = std::max(0, current_layout.task_height - 2 - static_cast<int>(lines.size()));
    const std::size_t start_index = (history.size() > static_cast<std::size_t>(visible_history_lines))
        ? history.size() - static_cast<std::size_t>(visible_history_lines)
        : 0;

    for (std::size_t index = start_index; index < history.size(); ++index) {
        lines.push_back(history[index]);
    }

    return lines;
}

void render_task_panels() {
    if (task_window_a == nullptr || task_window_b == nullptr || task_window_c == nullptr) {
        return;
    }

    const std::vector<TaskSnapshot> snapshots = sys_scheduler.snapshot_tasks();

    task_window_a->draw_lines(build_task_panel_lines(
        snapshots,
        "First claimant / holds Printer_Output for one quantum.",
        task_a_id,
        task_a_history
    ));
    task_window_b->draw_lines(build_task_panel_lines(
        snapshots,
        "Second claimant / should block behind Task_A.",
        task_b_id,
        task_b_history
    ));
    task_window_c->draw_lines(build_task_panel_lines(
        snapshots,
        "Third claimant / should block behind Task_B.",
        task_c_id,
        task_c_history
    ));
}

void render_console() {
    if (console_window == nullptr) {
        return;
    }

    console_window->draw_lines({
        "Controls: d dump | h help",
        "          p pause | q stop",
        "Cycle: " + std::to_string(demo_cycle_number),
        "Paused: " + std::string(demo_paused ? "yes" : "no")
            + " | Stop: " + std::string(stop_after_cycle ? "yes" : "no"),
        "Active: " + std::to_string(sys_scheduler.get_active_task_count())
            + " | Waiters: " + std::to_string(printer_semaphore.waiting_task_count()),
        "Status:",
        console_status
    });
}

void sync_visuals() {
    if (transcript_only_mode || header_window == nullptr) {
        return;
    }

    render_header();
    render_scheduler_panel();
    render_semaphore_panel();
    render_state_panel();
    render_task_panels();
    render_console();
    ui_manager.refresh_all();
}

void set_console_status(const std::string& status_line) {
    console_status = status_line;
    sync_visuals();
}

void log_event(const std::string& message) {
    append_transcript(message);
    write_line(log_window, message);
}

void visual_pause(int total_ms) {
    if (transcript_only_mode) {
        return;
    }

    int elapsed_ms = 0;
    while (elapsed_ms < total_ms) {
        if (console_window != nullptr) {
            const int input = wgetch(console_window->get_win_ptr());
            if (input != ERR) {
                switch (input) {
                    case 'd':
                    case 'D':
                        log_event("");
                        log_event("--- ON-DEMAND SYSTEM DUMP ---");
                        write_block(log_window, capture_dump_text([] {
                            sys_scheduler.dump(1);
                        }));
                        write_block(log_window, capture_dump_text([] {
                            printer_semaphore.dump(1);
                        }));
                        log_event("");
                        console_status = "Manual dump captured.";
                        sync_visuals();
                        break;
                    case 'h':
                    case 'H':
                        console_status = "Help refreshed.";
                        sync_visuals();
                        break;
                    case 'p':
                    case 'P':
                        demo_paused = !demo_paused;
                        console_status = demo_paused ? "Scheduler paused for inspection." : "Scheduler resumed.";
                        sync_visuals();
                        break;
                    case 'q':
                    case 'Q':
                        stop_after_cycle = true;
                        console_status = "Continuous demo will stop after the current cycle.";
                        sync_visuals();
                        break;
                    case KEY_MOUSE: {
                        MEVENT event {};
                        if (getmouse(&event) == OK) {
                            std::ostringstream mouse_line;
                            mouse_line << "[Mouse] row=" << event.y << ", col=" << event.x;
                            log_event(mouse_line.str());
                            console_status = "Mouse event logged.";
                            sync_visuals();
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        if (demo_paused) {
            napms(120);
            continue;
        }

        const int slice_ms = std::min(120, total_ms - elapsed_ms);
        napms(slice_ms);
        elapsed_ms += slice_ms;
    }
}

void pace_step(const std::string& status_line, int pause_ms = kStepPauseMs) {
    console_status = status_line;
    sync_visuals();
    visual_pause(pause_ms);
}

void note_task_a(const std::string& line) {
    add_task_history(task_a_history, line);
    sync_visuals();
}

void note_task_b(const std::string& line) {
    add_task_history(task_b_history, line);
    sync_visuals();
}

void note_task_c(const std::string& line) {
    add_task_history(task_c_history, line);
    sync_visuals();
}

void show_system_snapshot(const std::string& title) {
    log_event("");
    log_event(title);

    const std::string scheduler_dump = capture_dump_text([] {
        sys_scheduler.dump(1);
    });
    append_transcript(scheduler_dump);
    write_block(log_window, scheduler_dump);

    const std::string semaphore_dump = capture_dump_text([] {
        printer_semaphore.dump(1);
    });
    append_transcript(semaphore_dump);
    write_block(log_window, semaphore_dump);
    log_event("");

    sync_visuals();
    visual_pause(kDumpPauseMs);
}

bool current_task_is_blocked() {
    const int current_task_id = sys_scheduler.get_current_task_id();
    return current_task_id >= 0 && sys_scheduler.is_task_blocked(current_task_id);
}

void reset_cycle_state() {
    sys_scheduler.reset();
    printer_semaphore.reset();

    transcript_lines.clear();
    state_trace_lines.clear();
    task_a_history.clear();
    task_b_history.clear();
    task_c_history.clear();

    task_a_id = -1;
    task_b_id = -1;
    task_c_id = -1;

    task_a_phase = 0;
    task_b_phase = 0;
    task_c_phase = 0;

    task_b_was_blocked = false;
    task_c_was_blocked = false;
    mid_execution_dump_written = false;
    post_release_dump_written = false;

    latest_flow_summary = "Printer_Output is free and the process table is empty.";
    console_status = "Preparing cycle " + std::to_string(demo_cycle_number) + ".";

    add_state_trace("Cycle " + std::to_string(demo_cycle_number) + ": scheduler reset; Printer_Output unlocked.");

    if (!transcript_only_mode) {
        ui_manager.clear_all();
    }
}

void create_demo_tasks() {
    task_a_id = sys_scheduler.create_task("Task_A", [] {
        switch (task_a_phase) {
            case 0: {
                sys_scheduler.set_task_note(task_a_id, "Attempting to claim Printer_Output.");
                note_task_a("Calling down(Printer_Output).");
                log_event("[Task_A] down(Printer_Output)");
                add_state_trace("Task_A moved READY -> RUNNING and requested Printer_Output.");
                latest_flow_summary = "Task_A is the first claimant and should acquire Printer_Output.";
                pace_step("Task_A is requesting Printer_Output.");

                printer_semaphore.down();
                task_a_phase = 1;

                if (current_task_is_blocked()) {
                    sys_scheduler.set_task_note(task_a_id, "Unexpected BLOCKED state detected.");
                    note_task_a("Unexpected BLOCKED state encountered.");
                    return;
                }

                sys_scheduler.set_task_note(task_a_id, "Holding Printer_Output inside the critical section.");
                note_task_a("Acquired Printer_Output and now owns the critical section.");
                note_task_a("Yielding once so Task_B and Task_C can contend for the resource.");
                log_event("[Task_A] acquired Printer_Output.");
                add_state_trace("Task_A now owns Printer_Output and yields while keeping the resource.");
                latest_flow_summary = "Task_A acquired Printer_Output; Task_B and Task_C must block if they contend.";
                pace_step("Task_A is RUNNING and holding Printer_Output.");
                sys_scheduler.yield();
                return;
            }
            case 1:
                sys_scheduler.set_task_note(task_a_id, "Releasing Printer_Output and preparing to exit.");
                note_task_a("Calling up(Printer_Output) to release the resource.");
                log_event("[Task_A] up(Printer_Output)");
                add_state_trace("Task_A is releasing Printer_Output and waking the next waiter.");
                pace_step("Task_A is releasing Printer_Output.");

                printer_semaphore.up();
                latest_flow_summary = "Task_A released Printer_Output; Task_B is the next READY owner.";

                if (!post_release_dump_written) {
                    show_system_snapshot("--- POST-RELEASE STATE DUMP ---");
                    post_release_dump_written = true;
                }

                sys_scheduler.kill_task(task_a_id);
                sys_scheduler.set_task_note(task_a_id, "DEAD after completing the first critical section.");
                note_task_a("kill_task() moved Task_A into DEAD.");
                log_event("[Task_A] marked DEAD.");
                add_state_trace("Task_A transitioned to DEAD.");
                latest_flow_summary = "Task_A is DEAD; the scheduler will now rotate through the awakened tasks.";
                pace_step("Task_A finished and entered DEAD.");
                task_a_phase = 2;
                return;
            default:
                return;
        }
    });

    task_b_id = sys_scheduler.create_task("Task_B", [] {
        switch (task_b_phase) {
            case 0: {
                sys_scheduler.set_task_note(task_b_id, "Attempting to claim Printer_Output behind Task_A.");
                note_task_b("Calling down(Printer_Output).");
                log_event("[Task_B] down(Printer_Output)");
                add_state_trace("Task_B moved READY -> RUNNING and contended for Printer_Output.");
                latest_flow_summary = "Task_B is testing contention against Task_A.";
                pace_step("Task_B is requesting Printer_Output.");

                printer_semaphore.down();
                task_b_phase = 1;

                if (current_task_is_blocked()) {
                    task_b_was_blocked = true;
                    sys_scheduler.set_task_note(task_b_id, "BLOCKED behind Task_A in the semaphore queue.");
                    note_task_b("BLOCKED and placed in the FIFO wait queue.");
                    log_event("[Task_B] transitioned to BLOCKED.");
                    add_state_trace("Task_B transitioned RUNNING -> BLOCKED.");
                    latest_flow_summary = "Task_A -> Task_B BLOCKED";
                    pace_step("Task_B is BLOCKED behind Task_A.");
                    return;
                }
            }
                [[fallthrough]];
            case 1:
                if (task_b_was_blocked) {
                    sys_scheduler.set_task_note(task_b_id, "Woken from the semaphore queue and ready to run.");
                    note_task_b("Woken from the semaphore queue and returned to READY.");
                    add_state_trace("Task_B transitioned BLOCKED -> READY -> RUNNING.");
                }

                sys_scheduler.set_task_note(task_b_id, "Holding Printer_Output after the FIFO wake-up.");
                note_task_b("Acquired Printer_Output after Task_A released it.");
                log_event("[Task_B] acquired Printer_Output.");
                latest_flow_summary = "Task_B owns Printer_Output after Task_A.";
                pace_step("Task_B is RUNNING with Printer_Output.");
                task_b_phase = 2;
                sys_scheduler.yield();
                return;
            case 2:
                sys_scheduler.set_task_note(task_b_id, "Releasing Printer_Output and preparing to exit.");
                note_task_b("Calling up(Printer_Output).");
                log_event("[Task_B] up(Printer_Output)");
                add_state_trace("Task_B released Printer_Output and should wake Task_C.");
                pace_step("Task_B is releasing Printer_Output.");

                printer_semaphore.up();
                sys_scheduler.kill_task(task_b_id);
                sys_scheduler.set_task_note(task_b_id, "DEAD after finishing the second critical section.");
                note_task_b("kill_task() moved Task_B into DEAD.");
                log_event("[Task_B] marked DEAD.");
                add_state_trace("Task_B transitioned to DEAD.");
                latest_flow_summary = "Task_B is DEAD; Task_C is next to acquire Printer_Output.";
                pace_step("Task_B finished and entered DEAD.");
                task_b_phase = 3;
                return;
            default:
                return;
        }
    });

    task_c_id = sys_scheduler.create_task("Task_C", [] {
        switch (task_c_phase) {
            case 0: {
                sys_scheduler.set_task_note(task_c_id, "Attempting to claim Printer_Output behind Task_B.");
                note_task_c("Calling down(Printer_Output).");
                log_event("[Task_C] down(Printer_Output)");
                add_state_trace("Task_C moved READY -> RUNNING and contended for Printer_Output.");
                latest_flow_summary = "Task_C is contending after Task_B.";
                pace_step("Task_C is requesting Printer_Output.");

                printer_semaphore.down();
                task_c_phase = 1;

                if (current_task_is_blocked()) {
                    task_c_was_blocked = true;
                    sys_scheduler.set_task_note(task_c_id, "BLOCKED behind Task_B in the semaphore queue.");
                    note_task_c("BLOCKED and placed in the FIFO wait queue.");
                    log_event("[Task_C] transitioned to BLOCKED.");
                    add_state_trace("Task_C transitioned RUNNING -> BLOCKED.");
                    latest_flow_summary = "Task_A -> Task_B BLOCKED -> Task_C BLOCKED";
                    log_event("FLOW: Task_A -> Task_B BLOCKED -> Task_C BLOCKED");
                    pace_step("Task_C is BLOCKED behind Task_B.");

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
                    sys_scheduler.set_task_note(task_c_id, "Woken from the semaphore queue and ready to run.");
                    note_task_c("Woken from the semaphore queue and returned to READY.");
                    add_state_trace("Task_C transitioned BLOCKED -> READY -> RUNNING.");
                }

                sys_scheduler.set_task_note(task_c_id, "Holding Printer_Output after the final FIFO wake-up.");
                note_task_c("Acquired Printer_Output after Task_B released it.");
                log_event("[Task_C] acquired Printer_Output.");
                latest_flow_summary = "Task_C owns Printer_Output after the queued wake-up.";
                pace_step("Task_C is RUNNING with Printer_Output.");
                task_c_phase = 2;
                sys_scheduler.yield();
                return;
            case 2:
                sys_scheduler.set_task_note(task_c_id, "Releasing Printer_Output and preparing to exit.");
                note_task_c("Calling up(Printer_Output).");
                log_event("[Task_C] up(Printer_Output)");
                add_state_trace("Task_C released Printer_Output and will exit.");
                pace_step("Task_C is releasing Printer_Output.");

                printer_semaphore.up();
                sys_scheduler.kill_task(task_c_id);
                sys_scheduler.set_task_note(task_c_id, "DEAD after finishing the final critical section.");
                note_task_c("kill_task() moved Task_C into DEAD.");
                log_event("[Task_C] marked DEAD.");
                add_state_trace("Task_C transitioned to DEAD.");
                latest_flow_summary = "All work completed; all three tasks have reached DEAD.";
                pace_step("Task_C finished and entered DEAD.");
                task_c_phase = 3;
                return;
            default:
                return;
        }
    });

    sys_scheduler.set_task_note(task_a_id, "READY: first claimant that should acquire Printer_Output.");
    sys_scheduler.set_task_note(task_b_id, "READY: second claimant that should block behind Task_A.");
    sys_scheduler.set_task_note(task_c_id, "READY: third claimant that should block behind Task_B.");

    add_task_history(task_a_history, "Created in READY state.");
    add_task_history(task_a_history, "Will acquire Printer_Output first and yield while holding it.");
    add_task_history(task_b_history, "Created in READY state.");
    add_task_history(task_b_history, "Should block when Task_A still owns Printer_Output.");
    add_task_history(task_c_history, "Created in READY state.");
    add_task_history(task_c_history, "Should block behind Task_B in the FIFO queue.");

    add_state_trace("Three tasks were inserted into the process table.");
    add_state_trace("The scheduler will rotate Task_A -> Task_B -> Task_C by READY order.");
}

void create_windows() {
    header_window = new U2_window(
        current_layout.header_height,
        current_layout.header_width,
        current_layout.header_y,
        current_layout.header_x,
        "ULTIMA 2.0 - Phase 1",
        false
    );
    scheduler_window = new U2_window(
        current_layout.class_height,
        current_layout.class_width_left,
        current_layout.class_y,
        current_layout.class_x_left,
        "Scheduler Class",
        false
    );
    semaphore_window = new U2_window(
        current_layout.class_height,
        current_layout.class_width_middle,
        current_layout.class_y,
        current_layout.class_x_middle,
        "Semaphore Class",
        false
    );
    state_window = new U2_window(
        current_layout.class_height,
        current_layout.class_width_right,
        current_layout.class_y,
        current_layout.class_x_right,
        "State + Race Proof",
        false
    );
    task_window_a = new U2_window(
        current_layout.task_height,
        current_layout.task_width_left,
        current_layout.task_y,
        current_layout.task_x_left,
        "Task_A",
        false
    );
    task_window_b = new U2_window(
        current_layout.task_height,
        current_layout.task_width_middle,
        current_layout.task_y,
        current_layout.task_x_middle,
        "Task_B",
        false
    );
    task_window_c = new U2_window(
        current_layout.task_height,
        current_layout.task_width_right,
        current_layout.task_y,
        current_layout.task_x_right,
        "Task_C",
        false
    );
    log_window = new U2_window(
        current_layout.bottom_height,
        current_layout.log_width,
        current_layout.bottom_y,
        current_layout.log_x,
        "Output Log + Dumps",
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
    ui_manager.add_window(scheduler_window);
    ui_manager.add_window(semaphore_window);
    ui_manager.add_window(state_window);
    ui_manager.add_window(task_window_a);
    ui_manager.add_window(task_window_b);
    ui_manager.add_window(task_window_c);
    ui_manager.add_window(log_window);
    ui_manager.add_window(console_window);

    keypad(console_window->get_win_ptr(), TRUE);
    nodelay(console_window->get_win_ptr(), TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0);
}

void run_demo_cycle() {
    reset_cycle_state();
    create_demo_tasks();

    sync_visuals();

    log_event("=== ULTIMA Phase 1 terminal demonstration / cycle " + std::to_string(demo_cycle_number) + " ===");
    log_event("Scheduler purpose: manage the dynamic process table and choose the next READY task to run.");
    log_event("Semaphore purpose: prevent a shared Printer_Output race by blocking and queueing contenders.");
    log_event("Pthread purpose: protect ncurses writes so the larger multi-window display stays coherent.");
    log_event("Continuous mode: the interactive demo will restart automatically until you press q.");
    show_system_snapshot("--- INITIAL SYSTEM STATE ---");
    log_event("--- BEGINNING EXECUTION ---");
    pace_step("Scheduler run started.");

    while (sys_scheduler.has_active_tasks()) {
        sys_scheduler.yield();
        sync_visuals();
        visual_pause(kDispatchPauseMs);
    }

    show_system_snapshot("--- FINAL STATE DUMP (Before Garbage Collection) ---");
    sys_scheduler.garbage_collect();
    sync_visuals();
    show_system_snapshot("--- POST-GARBAGE-COLLECTION DUMP ---");
    log_event("System shutting down safely.");
    add_state_trace("Cycle proof completed: tasks are DEAD and the process table has been collected.");
    latest_flow_summary = "Cycle complete. The scheduler proved READY/RUNNING/BLOCKED/DEAD and FIFO wake-up.";
    pace_step("Cycle complete.", 500);
}

} // namespace

int main(int argc, char* argv[]) {
    std::string output_file_path;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--transcript-only") {
            transcript_only_mode = true;
            continue;
        }

        if (argument == "--output-file") {
            if (index + 1 >= argc) {
                std::cerr << "Missing path after --output-file" << std::endl;
                return 1;
            }
            output_file_path = argv[++index];
            continue;
        }
    }

    if (!transcript_only_mode) {
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

        create_windows();
        sync_visuals();
    }

    do {
        run_demo_cycle();
        if (transcript_only_mode || stop_after_cycle) {
            break;
        }

        ++demo_cycle_number;
        latest_flow_summary = "Restarting the continuous scheduler demonstration.";
        pace_step("Restarting the continuous scheduler demonstration.", kCycleRestartPauseMs);
        if (stop_after_cycle) {
            break;
        }
    } while (true);

    if (!transcript_only_mode) {
        nodelay(console_window->get_win_ptr(), FALSE);
        set_console_status("Continuous scheduler stopped. Press any key to exit.");
        wgetch(console_window->get_win_ptr());
        ui_manager.close_ncurses_env();
    }

    const std::string transcript_text = build_transcript_text();

    if (!output_file_path.empty()) {
        std::ofstream output_file(output_file_path);
        if (!output_file.is_open()) {
            std::cerr << "Unable to write transcript file: " << output_file_path << std::endl;
            return 1;
        }
        output_file << transcript_text;
    }

    if (transcript_only_mode) {
        std::cout << transcript_text;
    }

    return 0;
}
