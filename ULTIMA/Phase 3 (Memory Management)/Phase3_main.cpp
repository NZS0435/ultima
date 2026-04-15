/* =========================================================================
 * Phase3_main.cpp — ULTIMA 2.0 Phase 3 ncurses Windowed Demonstration
 * =========================================================================
 * Team Thunder #001
 *
 * Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label : Phase 3 - Memory Management
 *
 * Workflow Ownership:
 *   Stewart Pawley - main integration flow, scheduler/semaphore/IPC/MMU
 *                    coordination, final console narrative, final merge
 *   Zander Hayes   - allocator/free-space state presentation in the
 *                    cumulative run
 *   Nicholas Kobs  - read/write demonstration, dump visibility,
 *                    failure-case evidence, screenshot-oriented output
 * =========================================================================
 */

#include "MCB.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <curses.h>
#else
#include <ncurses.h>
#endif

MCB sys_mcb;

namespace {

struct Panel {
    WINDOW* win = nullptr;
    int rows = 0;
    int cols = 0;
    int y = 0;
    int x = 0;
    int accent_pair = 1;
    std::string title;

    void create(int r, int c, int py, int px, const std::string& panel_title, int accent = 1) {
        rows = r;
        cols = c;
        y = py;
        x = px;
        accent_pair = accent;
        title = panel_title;
        win = newwin(rows, cols, y, x);
        draw_frame();
        wrefresh(win);
    }

    void draw_frame() {
        if (win == nullptr) {
            return;
        }

        if (accent_pair > 0) {
            wattron(win, COLOR_PAIR(accent_pair));
        }
        box(win, 0, 0);
        if (accent_pair > 0) {
            wattroff(win, COLOR_PAIR(accent_pair));
        }

        if (!title.empty()) {
            wattron(win, A_BOLD | COLOR_PAIR(accent_pair));
            mvwprintw(win, 0, 2, " %s ", title.c_str());
            wattroff(win, A_BOLD | COLOR_PAIR(accent_pair));
        }
    }

    void clear_content() {
        if (win == nullptr) {
            return;
        }
        werase(win);
        draw_frame();
    }

    int write_text(const std::string& text, int start_row = 1, int color_pair = 0) {
        if (win == nullptr) {
            return start_row;
        }

        const int usable_width = std::max(1, cols - 2);
        int row = start_row;
        std::istringstream input(text);
        std::string line;

        while (std::getline(input, line) && row < rows - 1) {
            if (color_pair > 0) {
                wattron(win, COLOR_PAIR(color_pair));
            }
            mvwprintw(win, row, 1, "%-*.*s", usable_width, usable_width, line.c_str());
            if (color_pair > 0) {
                wattroff(win, COLOR_PAIR(color_pair));
            }
            ++row;
        }

        return row;
    }

    void write_highlight(const std::string& text, int row, int color_pair) {
        if (win == nullptr || row >= rows - 1) {
            return;
        }

        const int usable_width = std::max(1, cols - 2);
        wattron(win, A_BOLD | COLOR_PAIR(color_pair));
        mvwprintw(win, row, 1, "%-*.*s", usable_width, usable_width, text.c_str());
        wattroff(win, A_BOLD | COLOR_PAIR(color_pair));
    }

    void refresh_panel() {
        if (win == nullptr) {
            return;
        }
        draw_frame();
        wrefresh(win);
    }

    void destroy() {
        if (win != nullptr) {
            delwin(win);
            win = nullptr;
        }
    }
};

struct DemoState {
    int writer_a_id = -1;
    int writer_b_id = -1;
    int reader_c_id = -1;
    int waiter_d_id = -1;

    int writer_a_phase = 0;
    int writer_b_phase = 0;
    int reader_c_phase = 0;
    int waiter_d_phase = 0;

    int writer_a_handle = -1;
    int writer_b_handle = -1;
    int reader_c_handle = -1;
    int waiter_d_handle = -1;

    std::string writer_a_readback;
    std::string writer_a_mailbox_text;
    std::string reader_c_mailbox_text;
    std::string waiter_status;
    std::string latest_coalesce_title;
    std::string latest_coalesce_body;
    std::string current_step_title = "Phase 3 demo ready";
    std::string current_step_detail = "Waiting for the first visible transition.";
    std::string status_line = "Phase 3 demo ready.";
    int current_step = 1;
    std::array<std::string, 5> mailbox_visuals {"n/a", "empty", "empty", "empty", "empty"};
    std::string last_ipc_summary = "none";

    bool saw_scheduler = false;
    bool saw_semaphores = false;
    bool saw_ipc = false;
    bool saw_alloc_success = false;
    bool saw_blocked_request = false;
    bool saw_shark_pond = false;
    bool saw_coalesce = false;
    bool saw_protection = false;

    int snapshot_count = 0;
    std::vector<std::string> snapshots;
};

static constexpr int kTotalWidth = 170;
static constexpr int kPanelGap = 1;
static constexpr int kTotalDemoSteps = 12;

static Panel header_panel;
static Panel scheduler_panel;
static Panel semaphore_panel;
static Panel ipc_panel;
static Panel memory_panel;
static Panel dump_panel;
static Panel log_panel;
static Panel test_panel;
static Panel status_panel;

static DemoState demo;

static std::string mark(bool value) {
    return value ? "[OK]" : "[--]";
}

static std::string take_first_lines(const std::string& text, int line_limit) {
    std::istringstream input(text);
    std::ostringstream out;
    std::string line;
    int line_count = 0;

    while (std::getline(input, line) && line_count < line_limit) {
        out << line << '\n';
        ++line_count;
    }

    if (std::getline(input, line)) {
        out << "...\n";
    }

    return out.str();
}

static std::string build_step_progress_bar(int step, int total, int width) {
    const int usable_width = std::max(10, width);
    const int filled = std::clamp((usable_width * std::max(step, 0)) / std::max(total, 1), 0, usable_width);

    std::string bar;
    bar.reserve(static_cast<std::size_t>(usable_width + 2));
    bar.push_back('[');
    for (int index = 0; index < usable_width; ++index) {
        bar.push_back(index < filled ? '#' : '-');
    }
    bar.push_back(']');
    return bar;
}

static void push_event(const std::string& entry) {
    EventLog::instance().add(entry);
}

static void push_memory_event(const std::string& prefix = "[MMU] ") {
    push_event(prefix + sys_mcb.MemMgr.get_last_event());
}

static std::string short_message_type(const Message& message) {
    const std::string type_name = message.Msg_Type.Message_Type_Description;
    if (type_name == "Text") {
        return "text";
    }
    if (type_name == "Service") {
        return "svc";
    }
    if (type_name == "Notification") {
        return "note";
    }
    return "msg";
}

static std::string mailbox_queue_summary(int task_id) {
    std::queue<Message> mailbox_copy = sys_mcb.Messenger.get_mailbox_copy(task_id);
    if (mailbox_copy.empty()) {
        return "empty";
    }

    const Message& front = mailbox_copy.front();
    std::ostringstream out;
    out << "q=" << mailbox_copy.size()
        << " from T-" << front.Source_Task_Id
        << ' ' << short_message_type(front);
    return out.str();
}

static void note_mailbox_enqueue(int task_id) {
    if (task_id < 1 || task_id >= static_cast<int>(demo.mailbox_visuals.size())) {
        return;
    }

    demo.mailbox_visuals[static_cast<std::size_t>(task_id)] = mailbox_queue_summary(task_id);

    std::queue<Message> mailbox_copy = sys_mcb.Messenger.get_mailbox_copy(task_id);
    if (!mailbox_copy.empty()) {
        const Message& front = mailbox_copy.front();
        demo.last_ipc_summary =
            "T-" + std::to_string(front.Source_Task_Id)
            + "->T-" + std::to_string(front.Destination_Task_Id)
            + ' ' + short_message_type(front);
    }
}

static void note_mailbox_receive(int task_id, const Message& message) {
    if (task_id >= 1 && task_id < static_cast<int>(demo.mailbox_visuals.size())) {
        demo.mailbox_visuals[static_cast<std::size_t>(task_id)] =
            "empty after rx T-" + std::to_string(message.Source_Task_Id);
    }

    demo.last_ipc_summary =
        "T-" + std::to_string(task_id)
        + "<=T-" + std::to_string(message.Source_Task_Id)
        + ' ' + short_message_type(message);
}

static void set_demo_step(int step, const std::string& title, const std::string& detail) {
    demo.current_step = std::clamp(step, 1, kTotalDemoSteps);
    demo.current_step_title = title;
    demo.current_step_detail = detail;
    demo.status_line = detail;

    std::ostringstream step_entry;
    step_entry << "--- STEP " << demo.current_step << '/' << kTotalDemoSteps
               << ": " << title << " ---";
    push_event(step_entry.str());
}

static void record_snapshot(const std::string& title) {
    std::ostringstream out;
    out << "=== Snapshot " << (++demo.snapshot_count) << ": " << title << " ===\n\n";
    out << sys_mcb.Swapper.dump_string(1) << '\n';
    out << "Semaphore Summary\n";
    out << "=================\n";
    out << "Core: value=" << sys_mcb.Core.get_sema_value()
        << " owner=" << sys_mcb.Core.get_owner_task_id()
        << " wait=" << sys_mcb.Core.describe_wait_queue() << '\n';
    out << "Printer: value=" << sys_mcb.Printer.get_sema_value()
        << " owner=" << sys_mcb.Printer.get_owner_task_id()
        << " wait=" << sys_mcb.Printer.describe_wait_queue() << "\n\n";
    out << sys_mcb.Messenger.ipc_Message_Dump_String() << '\n';
    out << sys_mcb.MemMgr.layout_table_string();
    out << sys_mcb.MemMgr.memory_char_dump_string(0, 256, 64);
    demo.snapshots.push_back(out.str());
}

static void update_coalesce_snapshot(const std::string& title) {
    demo.latest_coalesce_title = title;
    demo.latest_coalesce_body =
        "Before Mem_Coalesce\n"
        + take_first_lines(sys_mcb.MemMgr.get_last_pre_coalesce_dump(), 10)
        + "\nAfter Mem_Coalesce\n"
        + take_first_lines(sys_mcb.MemMgr.get_last_post_coalesce_dump(), 10);
    demo.saw_coalesce = true;
}

static void create_layout() {
    int term_rows = 0;
    int term_cols = 0;
    getmaxyx(stdscr, term_rows, term_cols);
    clear();
    refresh();

    const int total_width = std::min(kTotalWidth, term_cols);
    const int left_margin = std::max(0, (term_cols - total_width) / 2);
    const int header_height = 3;
    const int status_height = (term_rows >= 24) ? 6 : 5;
    const int body_height = std::max(10, term_rows - header_height - status_height);

    int row1_height = 3;
    int row2_height = 4;
    int row3_height = 3;
    int extra_rows = std::max(0, body_height - (row1_height + row2_height + row3_height));

    const int row1_preferred_extra = 3;
    const int row2_preferred_extra = 5;
    const int row3_preferred_extra = 3;

    const int row1_add = std::min(extra_rows, row1_preferred_extra);
    row1_height += row1_add;
    extra_rows -= row1_add;

    const int row2_add = std::min(extra_rows, row2_preferred_extra);
    row2_height += row2_add;
    extra_rows -= row2_add;

    const int row3_add = std::min(extra_rows, row3_preferred_extra);
    row3_height += row3_add;
    extra_rows -= row3_add;

    while (extra_rows > 0) {
        ++row2_height;
        --extra_rows;
        if (extra_rows == 0) {
            break;
        }
        ++row1_height;
        --extra_rows;
        if (extra_rows == 0) {
            break;
        }
        ++row3_height;
        --extra_rows;
    }

    const int third_total = total_width - (2 * kPanelGap);
    const int third_w1 = third_total / 3;
    const int third_w2 = third_total / 3;
    const int third_w3 = third_total - third_w1 - third_w2;

    const int half_total = total_width - kPanelGap;
    const int half_w1 = (half_total * 3) / 5;
    const int half_w2 = half_total - half_w1;

    header_panel.create(header_height, total_width, 0, left_margin, "ULTIMA 2.0 - PHASE 3 MEMORY MANAGEMENT", 1);

    int current_y = header_height;
    scheduler_panel.create(row1_height, third_w1, current_y, left_margin, "SCHEDULER", 2);
    semaphore_panel.create(row1_height, third_w2, current_y, left_margin + third_w1 + kPanelGap, "SEMAPHORES", 3);
    ipc_panel.create(row1_height, third_w3, current_y, left_margin + third_w1 + kPanelGap + third_w2 + kPanelGap, "IPC", 4);

    current_y += row1_height;
    memory_panel.create(row2_height, half_w1, current_y, left_margin, "MMU LEDGER", 5);
    dump_panel.create(row2_height, half_w2, current_y, left_margin + half_w1 + kPanelGap, "CORE DUMP", 6);

    current_y += row2_height;
    log_panel.create(row3_height, half_w1, current_y, left_margin, "EVENT LOG", 7);
    test_panel.create(row3_height, half_w2, current_y, left_margin + half_w1 + kPanelGap, "CUMULATIVE TEST", 8);

    current_y += row3_height;
    status_panel.create(status_height, total_width, current_y, left_margin, "STEP STATUS", 9);
}

static void destroy_layout() {
    header_panel.destroy();
    scheduler_panel.destroy();
    semaphore_panel.destroy();
    ipc_panel.destroy();
    memory_panel.destroy();
    dump_panel.destroy();
    log_panel.destroy();
    test_panel.destroy();
    status_panel.destroy();
}

static void wait_key() {
    doupdate();
    flushinp();
    while (true) {
        const int key = getch();
        if (key == KEY_RESIZE) {
            continue;
        }
        if (key != ERR) {
            break;
        }
    }
}

static std::string format_header_panel() {
    std::ostringstream out;
    out << "Team Thunder #001 | Phase 3 adds MMU + first-fit + protection + shark-pond wake-up\n";
    out << "Press any key to advance one cumulative step. The bottom bar tracks the current step and action.";
    return out.str();
}

static std::string format_scheduler_panel() {
    demo.saw_scheduler = true;
    return sys_mcb.Swapper.dump_string(1);
}

static std::string format_semaphore_panel() {
    demo.saw_semaphores = true;

    std::ostringstream out;
    if (semaphore_panel.rows <= 6) {
        out << "Core v=" << sys_mcb.Core.get_sema_value()
            << " own=" << sys_mcb.Core.get_owner_task_id() << "\n";
        out << "Printer v=" << sys_mcb.Printer.get_sema_value()
            << " own=" << sys_mcb.Printer.get_owner_task_id() << "\n";
        out << "Mailbox sema down/up\n";

        bool first = true;
        for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
            TCB* tcb = sys_mcb.Swapper.get_tcb(task.task_id);
            if (tcb == nullptr || tcb->mailbox_semaphore == nullptr) {
                continue;
            }

            if (!first) {
                out << "  ";
            }
            out << "T-" << task.task_id
                << ' ' << tcb->mailbox_semaphore->get_down_operations()
                << '/' << tcb->mailbox_semaphore->get_up_operations();
            first = false;
        }
        out << '\n';
        return out.str();
    }

    out << "Core Memory Semaphore\n";
    out << "  value=" << sys_mcb.Core.get_sema_value()
        << " owner=" << sys_mcb.Core.get_owner_task_id()
        << " wait=" << sys_mcb.Core.describe_wait_queue() << "\n";
    out << "Printer Semaphore\n";
    out << "  value=" << sys_mcb.Printer.get_sema_value()
        << " owner=" << sys_mcb.Printer.get_owner_task_id()
        << " wait=" << sys_mcb.Printer.describe_wait_queue() << "\n\n";

    out << "Mailbox Semaphores\n";
    for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
        TCB* tcb = sys_mcb.Swapper.get_tcb(task.task_id);
        if (tcb == nullptr || tcb->mailbox_semaphore == nullptr) {
            continue;
        }

        out << "T-" << task.task_id
            << " owner=" << tcb->mailbox_semaphore->get_owner_task_id()
            << " wait=" << tcb->mailbox_semaphore->waiting_task_count()
            << " down=" << tcb->mailbox_semaphore->get_down_operations()
            << " up=" << tcb->mailbox_semaphore->get_up_operations()
            << '\n';
    }

    return out.str();
}

static std::string format_ipc_panel() {
    demo.saw_ipc = true;

    std::ostringstream out;
    out << "Queued:" << sys_mcb.Messenger.Message_Count()
        << "  Last:" << demo.last_ipc_summary << "\n";
    for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
        const std::size_t task_index = static_cast<std::size_t>(task.task_id);
        const std::string visual =
            task_index < demo.mailbox_visuals.size()
                ? demo.mailbox_visuals[task_index]
                : mailbox_queue_summary(task.task_id);
        out << "Mailbox T-" << task.task_id << ": " << visual << '\n';
    }
    if (ipc_panel.rows >= 9) {
        out << "\n";
        out << take_first_lines(sys_mcb.Messenger.ipc_Message_Dump_String(), 8);
    }
    return out.str();
}

static std::string format_memory_panel() {
    return sys_mcb.MemMgr.layout_table_string();
}

static std::string format_dump_panel() {
    std::ostringstream out;
    out << take_first_lines(sys_mcb.MemMgr.memory_char_dump_string(0, 256, 64), 8) << '\n';
    out << take_first_lines(sys_mcb.MemMgr.memory_hex_dump_string(0, 128, 16), 10);
    return out.str();
}

static std::string format_event_panel() {
    std::ostringstream out;
    auto entries = EventLog::instance().get_all();
    const int max_entries = 18;
    const int start = std::max(0, static_cast<int>(entries.size()) - max_entries);

    for (int index = start; index < static_cast<int>(entries.size()); ++index) {
        out << entries[static_cast<std::size_t>(index)] << '\n';
    }

    if (entries.empty()) {
        out << "Waiting for events...\n";
    }

    return out.str();
}

static std::string format_test_panel() {
    std::ostringstream out;
    out << "Testing Approach\n";
    out << "1. Step the same task graph through alloc/write/read/free.\n";
    out << "2. Capture cumulative snapshots after each visible transition.\n";
    out << "3. Keep scheduler, semaphores, IPC, and MMU visible together.\n\n";

    out << mark(demo.saw_scheduler) << " Scheduler dump visible\n";
    out << mark(demo.saw_semaphores) << " Semaphore owners/waiters visible\n";
    out << mark(demo.saw_ipc) << " IPC send/receive visible\n";
    out << mark(demo.saw_alloc_success) << " Successful allocation visible\n";
    out << mark(demo.saw_blocked_request) << " Blocked allocation visible\n";
    out << mark(demo.saw_shark_pond) << " Shark-pond wake-up visible\n";
    out << mark(demo.saw_coalesce) << " Pre/post coalesce visible\n";
    out << mark(demo.saw_protection) << " Segmentation fault protection visible\n\n";

    out << "Task A Readback: "
        << (demo.writer_a_readback.empty() ? "[pending]" : demo.writer_a_readback) << '\n';
    out << "Task A Mailbox:  "
        << (demo.writer_a_mailbox_text.empty() ? "[pending]" : demo.writer_a_mailbox_text) << '\n';
    out << "Task C Mailbox:  "
        << (demo.reader_c_mailbox_text.empty() ? "[pending]" : demo.reader_c_mailbox_text) << '\n';
    out << "Task D Status:   "
        << (demo.waiter_status.empty() ? "[pending]" : demo.waiter_status) << "\n\n";

    if (!demo.latest_coalesce_title.empty()) {
        out << demo.latest_coalesce_title << '\n';
        out << take_first_lines(demo.latest_coalesce_body, 10);
    } else {
        out << "Latest Coalesce: waiting for first free.\n";
    }

    out << "\nSnapshots captured: " << demo.snapshot_count << '\n';
    return out.str();
}

static void refresh_header() {
    header_panel.clear_content();
    header_panel.write_text(format_header_panel(), 1, 1);
    header_panel.refresh_panel();
}

static void refresh_scheduler() {
    scheduler_panel.clear_content();
    scheduler_panel.write_text(format_scheduler_panel(), 1, 2);
    scheduler_panel.refresh_panel();
}

static void refresh_semaphores() {
    semaphore_panel.clear_content();
    semaphore_panel.write_text(format_semaphore_panel(), 1, 3);
    semaphore_panel.refresh_panel();
}

static void refresh_ipc() {
    ipc_panel.clear_content();
    ipc_panel.write_text(format_ipc_panel(), 1, 4);
    ipc_panel.refresh_panel();
}

static void refresh_memory() {
    memory_panel.clear_content();
    memory_panel.write_text(format_memory_panel(), 1, 5);
    memory_panel.refresh_panel();
}

static void refresh_dump() {
    dump_panel.clear_content();
    dump_panel.write_text(format_dump_panel(), 1, 6);
    dump_panel.refresh_panel();
}

static void refresh_log() {
    log_panel.clear_content();
    log_panel.write_text(format_event_panel(), 1, 7);
    log_panel.refresh_panel();
}

static void refresh_tests() {
    test_panel.clear_content();
    test_panel.write_text(format_test_panel(), 1, 8);
    test_panel.refresh_panel();
}

static void refresh_status() {
    status_panel.clear_content();
    std::ostringstream step_line;
    step_line << "STEP " << demo.current_step << '/' << kTotalDemoSteps
              << " : " << demo.current_step_title;

    status_panel.write_highlight(step_line.str(), 1, 2);
    if (status_panel.rows >= 6) {
        status_panel.write_text(demo.current_step_detail, 2, 9);

        const int progress_width = std::max(12, status_panel.cols - 28);
        const std::string progress_bar =
            build_step_progress_bar(demo.current_step, kTotalDemoSteps, progress_width);
        status_panel.write_text(
            "Progress " + progress_bar + "  " + std::to_string(demo.current_step)
            + "/" + std::to_string(kTotalDemoSteps),
            3,
            3
        );
    } else {
        status_panel.write_text(
            demo.current_step_detail + " | "
            + std::to_string(demo.current_step) + "/" + std::to_string(kTotalDemoSteps),
            2,
            9
        );
    }
    status_panel.write_highlight("[ Press any key ]", status_panel.rows - 2, 2);
    status_panel.refresh_panel();
}

static void refresh_all() {
    refresh_header();
    refresh_scheduler();
    refresh_semaphores();
    refresh_ipc();
    refresh_memory();
    refresh_dump();
    refresh_log();
    refresh_tests();
    refresh_status();
}

static void writer_a_task();
static void writer_b_task();
static void reader_c_task();
static void waiter_d_task();

static void writer_a_task() {
    if (demo.writer_a_phase == 0) {
        demo.writer_a_handle = sys_mcb.MemMgr.Mem_Alloc(130);
        push_memory_event();
        if (demo.writer_a_handle < 0) {
            set_demo_step(2, "Task_A allocation + IPC ready", "Task_A failed unexpectedly during initial allocation.");
            sys_mcb.Swapper.kill_task(demo.writer_a_id);
            record_snapshot("Task_A unexpected allocation failure");
            return;
        }

        demo.saw_alloc_success = true;
        const std::string payload = "Task_A owns the first partition and announces readiness.";
        sys_mcb.MemMgr.Mem_Write(
            demo.writer_a_handle,
            0,
            static_cast<int>(payload.size()),
            payload.c_str()
        );
        push_memory_event();
        sys_mcb.Messenger.Message_Send(demo.writer_a_id, demo.reader_c_id, "Task_A buffer ready", 0);
        note_mailbox_enqueue(demo.reader_c_id);
        push_event("Task_A sent IPC text to Task_C after its first-fit allocation.");
        sys_mcb.Swapper.set_task_note(
            demo.writer_a_id,
            "Allocated 130 bytes (192 reserved) and published readiness."
        );
        set_demo_step(
            2,
            "Task_A allocation + IPC ready",
            "Task_A allocated 130 bytes (rounded to 192), wrote its payload, and sent IPC to Task_C."
        );
        demo.writer_a_phase = 1;
        record_snapshot("Task_A allocation + IPC ready");
        return;
    }

    if (demo.writer_a_phase == 1) {
        char readback[33] = {0};
        sys_mcb.MemMgr.Mem_Read(demo.writer_a_handle, 0, 32, readback);
        push_memory_event();
        demo.writer_a_readback = readback;

        Message incoming {};
        if (sys_mcb.Messenger.Message_Receive(demo.writer_a_id, &incoming) == 1) {
            demo.writer_a_mailbox_text = incoming.Msg_Text;
            note_mailbox_receive(demo.writer_a_id, incoming);
            push_event(
                "Task_A received mailbox text from T-"
                + std::to_string(incoming.Source_Task_Id)
                + ": \"" + incoming.Msg_Text + "\""
            );
        }

        sys_mcb.Swapper.set_task_note(
            demo.writer_a_id,
            "Validated read-back from its own partition and inspected mailbox."
        );
        set_demo_step(
            6,
            "Task_A read-back + mailbox receive",
            "Task_A read back its own partition and emptied its mailbox by receiving Task_B's update."
        );
        demo.writer_a_phase = 2;
        record_snapshot("Task_A read-back + mailbox receive");
        return;
    }

    if (demo.writer_a_phase == 2) {
        if (demo.waiter_d_handle >= 0) {
            const int rc = sys_mcb.MemMgr.Mem_Write(demo.waiter_d_handle, '!');
            push_memory_event();
            if (rc == -1) {
                demo.saw_protection = true;
                push_event("Task_A attempted a foreign write into Task_D memory and the MMU rejected it.");
            }
        }

        sys_mcb.MemMgr.Mem_Free(demo.writer_a_handle);
        push_memory_event();
        update_coalesce_snapshot("Task_A free snapshot");
        sys_mcb.Swapper.kill_task(demo.writer_a_id);
        set_demo_step(
            10,
            "Task_A protection test + free",
            "Task_A proved base/limit protection by attempting a foreign write, then freed its partition and exited."
        );
        demo.writer_a_phase = 3;
        record_snapshot("Task_A protection test + free");
    }
}

static void writer_b_task() {
    if (demo.writer_b_phase == 0) {
        demo.writer_b_handle = sys_mcb.MemMgr.Mem_Alloc(200);
        push_memory_event();
        if (demo.writer_b_handle < 0) {
            set_demo_step(3, "Task_B allocation + service message", "Task_B failed unexpectedly during initial allocation.");
            sys_mcb.Swapper.kill_task(demo.writer_b_id);
            record_snapshot("Task_B unexpected allocation failure");
            return;
        }

        demo.saw_alloc_success = true;
        const std::string payload = "Task_B occupies the middle partition to create a later hole.";
        sys_mcb.MemMgr.Mem_Write(
            demo.writer_b_handle,
            0,
            static_cast<int>(payload.size()),
            payload.c_str()
        );
        push_memory_event();
        sys_mcb.Messenger.Message_Send(demo.writer_b_id, demo.writer_a_id, "Task_B middle block allocated", 1);
        note_mailbox_enqueue(demo.writer_a_id);
        push_event("Task_B sent a service-style IPC note to Task_A.");
        sys_mcb.Swapper.set_task_note(
            demo.writer_b_id,
            "Allocated 200 bytes (256 reserved) in the middle of core memory."
        );
        set_demo_step(
            3,
            "Task_B allocation + service message",
            "Task_B allocated the middle partition and sent a service-style IPC update to Task_A."
        );
        demo.writer_b_phase = 1;
        record_snapshot("Task_B allocation + service message");
        return;
    }

    if (demo.writer_b_phase == 1) {
        sys_mcb.MemMgr.Mem_Free(demo.writer_b_handle);
        push_memory_event();
        update_coalesce_snapshot("Task_B free snapshot");
        sys_mcb.Swapper.kill_task(demo.writer_b_id);
        set_demo_step(
            7,
            "Task_B free / first coalesce view",
            "Task_B freed the middle partition, exposing hashes before coalesce and the first coalesce view."
        );
        demo.writer_b_phase = 2;
        record_snapshot("Task_B free");
    }
}

static void reader_c_task() {
    if (demo.reader_c_phase == 0) {
        demo.reader_c_handle = sys_mcb.MemMgr.Mem_Alloc(100);
        push_memory_event();
        if (demo.reader_c_handle < 0) {
            set_demo_step(4, "Task_C single-byte write + mailbox receive", "Task_C failed unexpectedly during initial allocation.");
            sys_mcb.Swapper.kill_task(demo.reader_c_id);
            record_snapshot("Task_C unexpected allocation failure");
            return;
        }

        demo.saw_alloc_success = true;
        const std::string payload = "Task_C streams single-byte writes into its partition.";
        for (char ch : payload) {
            sys_mcb.MemMgr.Mem_Write(demo.reader_c_handle, ch);
        }
        push_event("Task_C completed single-byte Mem_Write calls across its partition.");

        Message incoming {};
        if (sys_mcb.Messenger.Message_Receive(demo.reader_c_id, &incoming) == 1) {
            demo.reader_c_mailbox_text = incoming.Msg_Text;
            note_mailbox_receive(demo.reader_c_id, incoming);
            push_event(
                "Task_C received mailbox text from T-"
                + std::to_string(incoming.Source_Task_Id)
                + ": \"" + incoming.Msg_Text + "\""
            );
        }

        sys_mcb.Swapper.set_task_note(
            demo.reader_c_id,
            "Allocated 100 bytes (128 reserved), streamed writes, and consumed IPC text."
        );
        set_demo_step(
            4,
            "Task_C single-byte write + mailbox receive",
            "Task_C proved current_location advances by streaming single-byte writes, then consumed Task_A's mailbox text."
        );
        demo.reader_c_phase = 1;
        record_snapshot("Task_C single-byte write + mailbox receive");
        return;
    }

    if (demo.reader_c_phase == 1) {
        sys_mcb.MemMgr.Mem_Free(demo.reader_c_handle);
        push_memory_event();
        update_coalesce_snapshot("Task_C free snapshot");

        if (sys_mcb.Swapper.get_task_state(demo.waiter_d_id) == READY) {
            demo.saw_shark_pond = true;
            push_event("Shark-pond wake-up: Task_D returned to READY after Task_C freed memory.");
        }

        sys_mcb.Swapper.kill_task(demo.reader_c_id);
        set_demo_step(
            8,
            "Task_C free + shark pond wake-up",
            "Task_C freed the trailing partition; the shark-pond wake-up lets Task_D compete again."
        );
        demo.reader_c_phase = 2;
        record_snapshot("Task_C free + shark pond wake-up");
    }
}

static void waiter_d_task() {
    if (demo.waiter_d_phase == 0) {
        demo.waiter_d_handle = sys_mcb.MemMgr.Mem_Alloc(500);
        push_memory_event();

        if (demo.waiter_d_handle < 0) {
            demo.saw_blocked_request = true;
            demo.waiter_status = "Blocked waiting for a 512-byte contiguous segment.";
            sys_mcb.Swapper.set_task_note(
                demo.waiter_d_id,
                "Blocked in Mem_Alloc because the largest free segment is too small."
            );
            set_demo_step(
                5,
                "Task_D blocked on insufficient contiguous memory",
                "Task_D requested 500 bytes (512 reserved), but the largest free segment was too small, so it became BLOCKED."
            );
            record_snapshot("Task_D blocked on insufficient contiguous memory");
            return;
        }

        if (demo.saw_blocked_request) {
            demo.saw_shark_pond = true;
            demo.waiter_status = "Woke from shark pond and acquired the coalesced partition.";
            push_event("Task_D woke after the shark-pond retry path and won the coalesced segment.");
        } else {
            demo.waiter_status = "Allocated without blocking.";
        }

        const std::string payload = "Task_D now owns the coalesced partition after waiting.";
        sys_mcb.MemMgr.Mem_Write(
            demo.waiter_d_handle,
            0,
            static_cast<int>(payload.size()),
            payload.c_str()
        );
        push_memory_event();
        sys_mcb.Messenger.Message_Send(demo.waiter_d_id, demo.writer_a_id, "Task_D acquired coalesced memory", 2);
        note_mailbox_enqueue(demo.writer_a_id);
        push_event("Task_D sent a notification to Task_A after acquiring coalesced memory.");
        sys_mcb.Swapper.set_task_note(
            demo.waiter_d_id,
            "Acquired 500 bytes (512 reserved) after the shark-pond wake-up."
        );
        set_demo_step(
            9,
            "Task_D acquired coalesced partition",
            "Task_D retried Mem_Alloc successfully after coalescing and now owns the coalesced partition."
        );
        demo.waiter_d_phase = 1;
        record_snapshot("Task_D acquired coalesced partition");
        return;
    }

    if (demo.waiter_d_phase == 1) {
        sys_mcb.MemMgr.Mem_Free(demo.waiter_d_handle);
        push_memory_event();
        update_coalesce_snapshot("Task_D free snapshot");
        sys_mcb.Swapper.kill_task(demo.waiter_d_id);
        set_demo_step(
            11,
            "Task_D final free",
            "Task_D freed the coalesced partition, proving the system can return to one large free region."
        );
        demo.waiter_d_phase = 2;
        record_snapshot("Task_D final free");
    }
}

static void setup_demo() {
    EventLog::instance().clear();
    demo = DemoState {};
    sys_mcb.Swapper.reset();
    sys_mcb.Messenger.init(32, &sys_mcb.Swapper);
    sys_mcb.Monitor.reset();
    sys_mcb.Printer.reset();
    sys_mcb.Core.reset();
    sys_mcb.MemMgr.init(&sys_mcb.Swapper, &sys_mcb.Core);
    sys_mcb.MemMgr.reset();

    push_event("Phase 3 initialization: scheduler, semaphores, IPC, and MMU reset.");
    push_event("Testing strategy: capture cumulative dumps after each visible state transition.");

    demo.writer_a_id = sys_mcb.Swapper.create_task("Task_A_Writer", writer_a_task);
    demo.writer_b_id = sys_mcb.Swapper.create_task("Task_B_Writer", writer_b_task);
    demo.reader_c_id = sys_mcb.Swapper.create_task("Task_C_Reader", reader_c_task);
    demo.waiter_d_id = sys_mcb.Swapper.create_task("Task_D_Waiter", waiter_d_task);

    push_event("Created Task_A_Writer id=" + std::to_string(demo.writer_a_id));
    push_event("Created Task_B_Writer id=" + std::to_string(demo.writer_b_id));
    push_event("Created Task_C_Reader id=" + std::to_string(demo.reader_c_id));
    push_event("Created Task_D_Waiter id=" + std::to_string(demo.waiter_d_id));

    sys_mcb.Swapper.set_task_note(demo.writer_a_id, "Will allocate first-fit memory, publish readiness, and validate protection.");
    sys_mcb.Swapper.set_task_note(demo.writer_b_id, "Will allocate the middle partition, then free it to create a hole.");
    sys_mcb.Swapper.set_task_note(demo.reader_c_id, "Will allocate the trailing partition, perform single-byte writes, then free it.");
    sys_mcb.Swapper.set_task_note(demo.waiter_d_id, "Will request a large block, block, wake, retry, and then free.");

    set_demo_step(
        1,
        "Initial READY state",
        "All tasks are READY, core memory is empty, and cumulative capture is armed before the first visible transition."
    );
    record_snapshot("Initial READY state");
}

static std::string build_transcript() {
    std::ostringstream out;
    out << "=== ULTIMA 2.0 Phase 3 - Memory Management Transcript ===\n\n";

    for (const std::string& snapshot : demo.snapshots) {
        out << snapshot << '\n';
    }

    out << "=== Event Log ===\n";
    for (const std::string& entry : EventLog::instance().get_all()) {
        out << "  " << entry << '\n';
    }

    out << "\n=== End Transcript ===\n";
    return out.str();
}

} // namespace

int main(int argc, char* argv[]) {
    std::string output_file;
    bool transcript_only = false;

    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--transcript-only") {
            transcript_only = true;
        } else if (arg == "--output-file" && (index + 1) < argc) {
            output_file = argv[++index];
        }
    }

    setup_demo();

    if (!transcript_only) {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);

        if (has_colors()) {
            start_color();
            use_default_colors();
            init_pair(1, COLOR_YELLOW, -1);
            init_pair(2, COLOR_CYAN, -1);
            init_pair(3, COLOR_GREEN, -1);
            init_pair(4, COLOR_MAGENTA, -1);
            init_pair(5, COLOR_BLUE, -1);
            init_pair(6, COLOR_WHITE, -1);
            init_pair(7, COLOR_RED, -1);
            init_pair(8, COLOR_GREEN, -1);
            init_pair(9, COLOR_WHITE, -1);
        }

        create_layout();
        refresh_all();
        wait_key();
    }

    int guard = 0;
    while (sys_mcb.Swapper.has_active_tasks() && guard < 16) {
        ++guard;
        sys_mcb.Swapper.yield();
        if (!transcript_only) {
            refresh_all();
            wait_key();
        }
    }

    push_event("Phase 3 cleanup: invoking garbage collector on DEAD tasks.");
    sys_mcb.Swapper.garbage_collect();
    set_demo_step(
        12,
        "Final state after garbage collection",
        "All DEAD tasks were removed from the scheduler; Phase 3 cumulative demonstration is complete."
    );
    record_snapshot("Final state after garbage collection");

    if (!transcript_only) {
        refresh_all();
        wait_key();
        destroy_layout();
        endwin();
    }

    const std::string transcript = build_transcript();
    std::cout << transcript;

    if (!output_file.empty()) {
        std::ofstream output(output_file);
        output << transcript;
    }

    return 0;
}
