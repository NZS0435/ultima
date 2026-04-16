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
#include <iomanip>
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
            std::size_t offset = 0;

            if (line.empty()) {
                if (color_pair > 0) {
                    wattron(win, COLOR_PAIR(color_pair));
                }
                mvwprintw(win, row, 1, "%-*.*s", usable_width, usable_width, "");
                if (color_pair > 0) {
                    wattroff(win, COLOR_PAIR(color_pair));
                }
                ++row;
                continue;
            }

            while (offset < line.size() && row < rows - 1) {
                std::size_t remaining = line.size() - offset;
                std::size_t chunk_size = std::min<std::size_t>(remaining, static_cast<std::size_t>(usable_width));

                if (remaining > static_cast<std::size_t>(usable_width)) {
                    const std::size_t break_at = line.rfind(' ', offset + chunk_size);
                    if (break_at != std::string::npos && break_at > offset) {
                        chunk_size = break_at - offset;
                    }
                }

                std::string chunk = line.substr(offset, chunk_size);
                if (color_pair > 0) {
                    wattron(win, COLOR_PAIR(color_pair));
                }
                mvwprintw(win, row, 1, "%-*.*s", usable_width, usable_width, chunk.c_str());
                if (color_pair > 0) {
                    wattroff(win, COLOR_PAIR(color_pair));
                }

                offset += chunk_size;
                while (offset < line.size() && line[offset] == ' ') {
                    ++offset;
                }
                ++row;
            }
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

static constexpr int kTotalWidth = 200;
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

static int panel_content_rows(const Panel& panel) {
    return std::max(1, panel.rows - 2);
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

static std::string replace_all(std::string text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return text;
    }

    std::size_t offset = 0;
    while ((offset = text.find(from, offset)) != std::string::npos) {
        text.replace(offset, from.size(), to);
        offset += to.size();
    }

    return text;
}

static std::vector<std::string> split_lines(const std::string& text, bool keep_empty = true) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;

    while (std::getline(input, line)) {
        if (!keep_empty && line.empty()) {
            continue;
        }
        lines.push_back(line);
    }

    return lines;
}

static std::string fit_panel_line(const std::string& line, int usable_width) {
    if (usable_width <= 0 || static_cast<int>(line.size()) <= usable_width) {
        return line;
    }

    if (usable_width <= 3) {
        return line.substr(0, static_cast<std::size_t>(usable_width));
    }

    const std::size_t dump_marker = line.find(" : ");
    if (dump_marker == std::string::npos) {
        return line.substr(0, static_cast<std::size_t>(usable_width - 3)) + "...";
    }

    const std::string prefix = line.substr(0, dump_marker + 3);
    const int body_width = usable_width - static_cast<int>(prefix.size());
    if (body_width <= 3) {
        return line.substr(0, static_cast<std::size_t>(usable_width - 3)) + "...";
    }

    return prefix + line.substr(dump_marker + 3, static_cast<std::size_t>(body_width - 3)) + "...";
}

static char task_letter_for_id(int task_id) {
    return (task_id >= 1 && task_id <= 4) ? static_cast<char>('A' + task_id - 1) : '\0';
}

static std::string task_id_code(int task_id) {
    const char letter = task_letter_for_id(task_id);
    if (letter != '\0') {
        return std::string(1, letter);
    }
    if (task_id < 0) {
        return "none";
    }
    return std::to_string(task_id);
}

static std::string task_label(int task_id) {
    const char letter = task_letter_for_id(task_id);
    if (letter != '\0') {
        return "Task " + std::string(1, letter);
    }
    if (task_id < 0) {
        return "none";
    }
    return "Task " + std::to_string(task_id);
}

static std::string owner_label(int task_id) {
    const char letter = task_letter_for_id(task_id);
    if (letter != '\0') {
        return std::string(1, letter);
    }
    return (task_id < 0) ? "none" : std::to_string(task_id);
}

static std::string display_text(std::string text) {
    text = replace_all(text, "T-1", "Task A");
    text = replace_all(text, "T-2", "Task B");
    text = replace_all(text, "T-3", "Task C");
    text = replace_all(text, "T-4", "Task D");

    text = replace_all(text, "id=1", "id=A");
    text = replace_all(text, "id=2", "id=B");
    text = replace_all(text, "id=3", "id=C");
    text = replace_all(text, "id=4", "id=D");

    text = replace_all(text, "Current Task: -1", "Current Task: none");
    text = replace_all(text, "Task Number: 1", "Task: Task A");
    text = replace_all(text, "Task Number: 2", "Task: Task B");
    text = replace_all(text, "Task Number: 3", "Task: Task C");
    text = replace_all(text, "Task Number: 4", "Task: Task D");
    return text;
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

static const char* state_name(State state) {
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

static const char* state_code(State state) {
    switch (state) {
        case RUNNING:
            return "RUN";
        case READY:
            return "RDY";
        case BLOCKED:
            return "BLK";
        case DEAD:
            return "DED";
        default:
            return "UNK";
    }
}

static std::string format_handle_hex(int handle) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << handle;
    return out.str();
}

static std::string build_run_queue_display() {
    const auto tasks = sys_mcb.Swapper.snapshot_tasks();
    std::ostringstream out;
    bool wrote_any = false;

    for (const TaskSnapshot& task : tasks) {
        if (task.task_state == DEAD) {
            continue;
        }

        if (wrote_any) {
            out << " -> ";
        }

        out << task.task_name << '(' << task_label(task.task_id)
            << ", " << state_name(task.task_state) << ')';
        wrote_any = true;
    }

    return wrote_any ? out.str() : "[empty]";
}

static std::string build_run_queue_compact_display() {
    const auto tasks = sys_mcb.Swapper.snapshot_tasks();
    std::ostringstream out;
    bool wrote_any = false;

    for (const TaskSnapshot& task : tasks) {
        if (task.task_state == DEAD) {
            continue;
        }

        if (wrote_any) {
            out << " -> ";
        }

        out << task_id_code(task.task_id) << ' ' << state_code(task.task_state);
        wrote_any = true;
    }

    return wrote_any ? out.str() : "[empty]";
}

static std::string compact_scheduler_event_text(std::string text) {
    text = display_text(text);
    text = replace_all(text, "Task_A_Writer", "Task A");
    text = replace_all(text, "Task_B_Writer", "Task B");
    text = replace_all(text, "Task_C_Reader", "Task C");
    text = replace_all(text, "Task_D_Waiter", "Task D");
    text = replace_all(text, "Dispatching ", "Dispatch ");
    text = replace_all(text, " at tick ", " @ ");
    return text;
}

static std::string build_scheduler_dump(int level = 0) {
    std::ostringstream out;
    const auto tasks = sys_mcb.Swapper.snapshot_tasks();

    out << "Scheduler Dump\n";
    out << "==============\n";
    out << "Tick: " << sys_mcb.Swapper.get_scheduler_tick() << '\n';
    out << "Current Task: " << task_id_code(sys_mcb.Swapper.get_current_task_id()) << '\n';
    out << "Last Event: " << display_text(sys_mcb.Swapper.get_last_scheduler_event()) << '\n';
    out << "Run Queue: " << build_run_queue_display() << "\n\n";

    if (tasks.empty()) {
        out << "[Process table empty after garbage collection]\n\n";
        return out.str();
    }

    out << std::left
        << std::setw(8) << "Task"
        << std::setw(16) << "Name"
        << std::setw(10) << "State"
        << std::setw(8) << "Run"
        << std::setw(8) << "Yield"
        << std::setw(8) << "Block"
        << std::setw(8) << "Wake"
        << "Details\n";
    out << "--------------------------------------------------------------------------\n";

    for (const TaskSnapshot& task : tasks) {
        out << std::left
            << std::setw(8) << task_id_code(task.task_id)
            << std::setw(16) << task.task_name
            << std::setw(10) << state_name(task.task_state)
            << std::setw(8) << task.dispatch_count
            << std::setw(8) << task.yield_count
            << std::setw(8) << task.block_count
            << std::setw(8) << task.unblock_count
            << display_text(task.detail_note)
            << '\n';

        if (level > 0) {
            out << "         Last transition: " << display_text(task.last_transition) << '\n';
        }
    }

    out << '\n';
    return out.str();
}

static std::string build_semaphore_dump() {
    std::ostringstream out;
    const auto tasks = sys_mcb.Swapper.snapshot_tasks();

    out << "Semaphore Summary\n";
    out << "=================\n";
    out << "Core Memory Semaphore\n";
    out << "  value=" << sys_mcb.Core.get_sema_value()
        << " owner=" << owner_label(sys_mcb.Core.get_owner_task_id())
        << " wait=" << display_text(sys_mcb.Core.describe_wait_queue()) << '\n';
    out << "Printer Semaphore\n";
    out << "  value=" << sys_mcb.Printer.get_sema_value()
        << " owner=" << owner_label(sys_mcb.Printer.get_owner_task_id())
        << " wait=" << display_text(sys_mcb.Printer.describe_wait_queue()) << "\n\n";

    out << "Mailbox Semaphores\n";
    if (tasks.empty()) {
        out << "  [none]\n";
        return out.str();
    }

    for (const TaskSnapshot& task : tasks) {
        TCB* tcb = sys_mcb.Swapper.get_tcb(task.task_id);
        if (tcb == nullptr || tcb->mailbox_semaphore == nullptr) {
            continue;
        }

        out << task_label(task.task_id)
            << " owner=" << owner_label(tcb->mailbox_semaphore->get_owner_task_id())
            << " wait=" << tcb->mailbox_semaphore->waiting_task_count()
            << " down=" << tcb->mailbox_semaphore->get_down_operations()
            << " up=" << tcb->mailbox_semaphore->get_up_operations()
            << '\n';
    }

    return out.str();
}

static std::string build_ipc_dump() {
    std::ostringstream out;
    const auto tasks = sys_mcb.Swapper.snapshot_tasks();

    out << "IPC Mailboxes\n";
    out << "=============\n";
    out << "Queued: " << sys_mcb.Messenger.Message_Count()
        << "  Last: " << demo.last_ipc_summary << "\n\n";

    if (tasks.empty()) {
        out << "All mailboxes empty.\n";
        return out.str();
    }

    for (const TaskSnapshot& task : tasks) {
        std::queue<Message> mailbox_copy = sys_mcb.Messenger.get_mailbox_copy(task.task_id);
        out << task_label(task.task_id) << ": " << mailbox_copy.size() << " queued\n";

        if (mailbox_copy.empty()) {
            out << "  [empty]\n";
            continue;
        }

        while (!mailbox_copy.empty()) {
            const Message& message = mailbox_copy.front();
            out << "  From " << task_label(message.Source_Task_Id)
                << " [" << message.Msg_Type.Message_Type_Description << "] "
                << '"' << message.Msg_Text << '"'
                << '\n';
            mailbox_copy.pop();
        }
    }

    return out.str();
}

static std::string build_memory_ledger_dump() {
    std::ostringstream out;
    const auto segments = sys_mcb.MemMgr.snapshot_segments();

    out << "Memory Usage Ledger\n";
    out << "===================\n";
    out << "Core Bytes: " << sys_mcb.MemMgr.capacity()
        << " | Block Size: " << sys_mcb.MemMgr.page_size()
        << " | Free: " << sys_mcb.MemMgr.Mem_Left()
        << " | Largest: " << sys_mcb.MemMgr.Mem_Largest()
        << " | Smallest: " << sys_mcb.MemMgr.Mem_Smallest()
        << " | Waiting: " << sys_mcb.MemMgr.waiting_task_count() << "\n\n";

    out << std::left
        << std::setw(10) << "Status"
        << std::setw(12) << "Handle"
        << std::setw(10) << "Start"
        << std::setw(10) << "End"
        << std::setw(10) << "Bytes"
        << std::setw(12) << "Requested"
        << std::setw(12) << "Current"
        << "Task-ID\n";
    out << std::string(78, '-') << "\n";

    for (const mmu::SegmentSnapshot& segment : segments) {
        out << std::left
            << std::setw(10) << (segment.free ? "Free" : "Used")
            << std::setw(12) << (segment.free ? "MMU" : format_handle_hex(segment.handle))
            << std::setw(10) << segment.start
            << std::setw(10) << segment.end
            << std::setw(10) << segment.size
            << std::setw(12) << (segment.free ? "NA" : std::to_string(segment.requested_size))
            << std::setw(12)
            << (segment.free ? "NA" : std::to_string(std::max(0, segment.current_location)))
            << (segment.free ? "MMU" : task_id_code(segment.owner_task_id))
            << '\n';
    }

    out << '\n';
    return out.str();
}

static std::string build_core_dump_dump(int char_bytes_per_line, int char_bytes, int hex_bytes_per_line, int hex_bytes) {
    std::ostringstream out;
    out << sys_mcb.MemMgr.memory_char_dump_string(0, char_bytes, char_bytes_per_line);
    out << sys_mcb.MemMgr.memory_hex_dump_string(0, hex_bytes, hex_bytes_per_line);
    return out.str();
}

static std::string mailbox_queue_summary(int task_id) {
    std::queue<Message> mailbox_copy = sys_mcb.Messenger.get_mailbox_copy(task_id);
    if (mailbox_copy.empty()) {
        return "empty";
    }

    const Message& front = mailbox_copy.front();
    std::ostringstream out;
    out << "q=" << mailbox_copy.size()
        << " from " << task_label(front.Source_Task_Id)
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
            task_label(front.Source_Task_Id)
            + " -> " + task_label(front.Destination_Task_Id)
            + ' ' + short_message_type(front);
    }
}

static void note_mailbox_receive(int task_id, const Message& message) {
    if (task_id >= 1 && task_id < static_cast<int>(demo.mailbox_visuals.size())) {
        demo.mailbox_visuals[static_cast<std::size_t>(task_id)] =
            "empty after rx " + task_label(message.Source_Task_Id);
    }

    demo.last_ipc_summary =
        task_label(task_id)
        + " <= " + task_label(message.Source_Task_Id)
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
    out << build_scheduler_dump(1) << '\n';
    out << build_semaphore_dump() << "\n\n";
    out << build_ipc_dump() << "\n\n";
    out << build_memory_ledger_dump();
    out << build_core_dump_dump(64, 256, 16, 64);
    demo.snapshots.push_back(out.str());
}

static void update_coalesce_snapshot(const std::string& title, const std::string& body) {
    demo.latest_coalesce_title = title;
    demo.latest_coalesce_body = body;
    demo.saw_coalesce = true;
}

static void create_layout() {
    int term_rows = 0;
    int term_cols = 0;
    getmaxyx(stdscr, term_rows, term_cols);
    clear();
    refresh();

    const int total_width = std::min(kTotalWidth, std::max(40, term_cols));
    const int left_margin = std::max(0, (term_cols - total_width) / 2);
    const int header_height = 3;
    const int status_height = (term_rows >= 24) ? 5 : 4;
    const int body_height = std::max(10, term_rows - header_height - status_height);

    int row1_height = 3;
    int row2_height = 4;
    int row3_height = 3;
    int extra_rows = std::max(0, body_height - (row1_height + row2_height + row3_height));

    const int row1_preferred_extra = 3;
    const int row2_preferred_extra = 4;
    const int row3_preferred_extra = 2;

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
    std::ostringstream out;

    if (panel_content_rows(scheduler_panel) <= 4) {
        out << "Tick: " << sys_mcb.Swapper.get_scheduler_tick()
            << "  Current: " << task_label(sys_mcb.Swapper.get_current_task_id()) << '\n';
        out << "Last: "
            << compact_scheduler_event_text(sys_mcb.Swapper.get_last_scheduler_event()) << '\n';
        out << "Queue: " << build_run_queue_compact_display() << '\n';
        return out.str();
    }

    out << "Scheduler Dump\n";
    out << "==============\n";
    out << "Tick: " << sys_mcb.Swapper.get_scheduler_tick() << '\n';
    out << "Current Task: " << task_id_code(sys_mcb.Swapper.get_current_task_id()) << '\n';
    out << "Last Event: " << display_text(sys_mcb.Swapper.get_last_scheduler_event()) << '\n';
    out << "Run Queue: " << build_run_queue_display() << '\n';
    return out.str();
}

static std::string format_semaphore_panel() {
    demo.saw_semaphores = true;

    std::ostringstream out;
    if (semaphore_panel.rows <= 6) {
        out << "Core v=" << sys_mcb.Core.get_sema_value()
            << " own=" << owner_label(sys_mcb.Core.get_owner_task_id()) << "\n";
        out << "Printer v=" << sys_mcb.Printer.get_sema_value()
            << " own=" << owner_label(sys_mcb.Printer.get_owner_task_id()) << "\n";
        out << "Mailbox down/up\n";

        bool first = true;
        for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
            TCB* tcb = sys_mcb.Swapper.get_tcb(task.task_id);
            if (tcb == nullptr || tcb->mailbox_semaphore == nullptr) {
                continue;
            }

            if (!first) {
                out << "  ";
            }
            out << task_id_code(task.task_id)
                << ' ' << tcb->mailbox_semaphore->get_down_operations()
                << '/' << tcb->mailbox_semaphore->get_up_operations();
            first = false;
        }
        out << '\n';
        return out.str();
    }

    out << "Core Memory Semaphore\n";
    out << "  value=" << sys_mcb.Core.get_sema_value()
        << " owner=" << owner_label(sys_mcb.Core.get_owner_task_id())
        << " wait=" << display_text(sys_mcb.Core.describe_wait_queue()) << "\n";
    out << "Printer Semaphore\n";
    out << "  value=" << sys_mcb.Printer.get_sema_value()
        << " owner=" << owner_label(sys_mcb.Printer.get_owner_task_id())
        << " wait=" << display_text(sys_mcb.Printer.describe_wait_queue()) << "\n\n";

    out << "Mailbox Semaphores\n";
    for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
        TCB* tcb = sys_mcb.Swapper.get_tcb(task.task_id);
        if (tcb == nullptr || tcb->mailbox_semaphore == nullptr) {
            continue;
        }

        out << task_label(task.task_id)
            << " owner=" << owner_label(tcb->mailbox_semaphore->get_owner_task_id())
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

    if (panel_content_rows(ipc_panel) <= 4) {
        out << "Queued:" << sys_mcb.Messenger.Message_Count()
            << "  Last:" << demo.last_ipc_summary << '\n';

        std::vector<std::string> entries;
        for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
            const std::size_t task_index = static_cast<std::size_t>(task.task_id);
            const std::string visual =
                task_index < demo.mailbox_visuals.size()
                    ? demo.mailbox_visuals[task_index]
                    : mailbox_queue_summary(task.task_id);
            entries.push_back(task_label(task.task_id) + ": " + visual);
        }

        if (entries.empty()) {
            out << "All mailboxes empty.\n";
            return out.str();
        }

        for (std::size_t index = 0; index < entries.size(); index += 2) {
            out << entries[index];
            if (index + 1 < entries.size()) {
                out << "  " << entries[index + 1];
            }
            out << '\n';
        }

        return out.str();
    }

    out << "Queued:" << sys_mcb.Messenger.Message_Count()
        << "  Last:" << demo.last_ipc_summary << "\n";
    bool wrote_any_mailbox = false;
    for (const TaskSnapshot& task : sys_mcb.Swapper.snapshot_tasks()) {
        const std::size_t task_index = static_cast<std::size_t>(task.task_id);
        const std::string visual =
            task_index < demo.mailbox_visuals.size()
                ? demo.mailbox_visuals[task_index]
                : mailbox_queue_summary(task.task_id);
        out << "Mailbox " << task_label(task.task_id) << ": " << visual << '\n';
        wrote_any_mailbox = true;
    }
    if (!wrote_any_mailbox) {
        out << "All mailboxes empty.\n";
    }
    return out.str();
}

static std::string format_memory_panel() {
    if (panel_content_rows(memory_panel) <= 6) {
        std::ostringstream out;
        out << "Core:" << sys_mcb.MemMgr.capacity()
            << "B Block:" << sys_mcb.MemMgr.page_size()
            << " Free:" << sys_mcb.MemMgr.Mem_Left()
            << " Wait:" << sys_mcb.MemMgr.waiting_task_count() << '\n';

        for (const mmu::SegmentSnapshot& segment : sys_mcb.MemMgr.snapshot_segments()) {
            if (segment.free) {
                out << "MMU free " << segment.start << '-' << segment.end
                    << "  " << segment.size << "B\n";
                continue;
            }

            out << task_id_code(segment.owner_task_id)
                << ' ' << format_handle_hex(segment.handle)
                << ' ' << segment.start << '-' << segment.end
                << ' ' << segment.size << "B"
                << " req" << segment.requested_size
                << " cur" << std::max(0, segment.current_location)
                << '\n';
        }

        return out.str();
    }

    return build_memory_ledger_dump();
}

static std::string format_dump_panel() {
    const int content_rows = panel_content_rows(dump_panel);
    const int usable_width = std::max(8, dump_panel.cols - 2);
    const int char_bytes_per_line = 64;
    const int hex_bytes_per_line = (dump_panel.cols >= 82) ? 16 : 8;

    if (content_rows <= 6) {
        const auto char_lines = split_lines(
            sys_mcb.MemMgr.memory_char_dump_string(0, 256, char_bytes_per_line)
        );
        const auto hex_lines = split_lines(
            sys_mcb.MemMgr.memory_hex_dump_string(0, hex_bytes_per_line, hex_bytes_per_line)
        );

        std::ostringstream out;
        out << "Character View\n";
        for (std::size_t index = 2; index < char_lines.size() && index < 6; ++index) {
            out << fit_panel_line(char_lines[index], usable_width) << '\n';
        }

        out << "HEX / ASCII";
        if (content_rows >= 7 && hex_lines.size() > 2) {
            out << '\n' << fit_panel_line(hex_lines[2], usable_width);
        } else {
            out << '\n';
        }

        return out.str();
    }

    const int char_line_budget = std::max(5, content_rows - 3);
    const int hex_line_budget = std::max(2, content_rows - char_line_budget);
    const int char_body_lines = std::max(1, char_line_budget - 2);
    const int hex_body_lines = std::max(1, hex_line_budget - 2);
    const int char_bytes = std::min(256, char_bytes_per_line * char_body_lines);
    const int hex_bytes = std::min(64, hex_bytes_per_line * hex_body_lines);

    std::ostringstream out;
    out << take_first_lines(
        sys_mcb.MemMgr.memory_char_dump_string(0, char_bytes, char_bytes_per_line),
        char_line_budget
    ) << '\n';
    out << take_first_lines(
        sys_mcb.MemMgr.memory_hex_dump_string(0, hex_bytes, hex_bytes_per_line),
        hex_line_budget
    );
    return out.str();
}

static std::string format_event_panel() {
    std::ostringstream out;
    auto entries = EventLog::instance().get_all();
    const int max_entries = std::max(6, log_panel.rows * 2);
    const int start = std::max(0, static_cast<int>(entries.size()) - max_entries);

    for (int index = start; index < static_cast<int>(entries.size()); ++index) {
        out << display_text(entries[static_cast<std::size_t>(index)]) << '\n';
    }

    if (entries.empty()) {
        out << "Waiting for events...\n";
    }

    return out.str();
}

static std::string format_test_panel() {
    std::ostringstream out;

    if (panel_content_rows(test_panel) <= 2) {
        out << "1. Step the same task graph through alloc/write/read/free.\n";
        out << "2. Snapshot each transition. 3. Keep all windows visible.\n";
        return out.str();
    }

    if (panel_content_rows(test_panel) <= 3) {
        out << "1. Step the same task graph through alloc/write/read/free.\n";
        out << "2. Capture cumulative snapshots after each visible transition.\n";
        out << "3. Keep scheduler, semaphores, IPC, and MMU visible together.\n";
        return out.str();
    }

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
        out << take_first_lines(display_text(demo.latest_coalesce_body), 10);
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
            set_demo_step(2, "Task A allocation and IPC ready", "Task A failed unexpectedly during initial allocation.");
            sys_mcb.Swapper.kill_task(demo.writer_a_id);
            record_snapshot("Task A unexpected allocation failure");
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
        push_event("Task A sent IPC text to Task C after its first-fit allocation.");
        sys_mcb.Swapper.set_task_note(
            demo.writer_a_id,
            "Allocated 130 bytes (192 reserved) and published readiness."
        );
        set_demo_step(
            2,
            "Task A allocation and IPC ready",
            "Task A allocated 130 bytes, reserved 192 bytes, wrote its payload, and sent IPC to Task C."
        );
        demo.writer_a_phase = 1;
        record_snapshot("Task A allocation and IPC ready");
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
                "Task A received mailbox text from "
                + task_label(incoming.Source_Task_Id)
                + ": \"" + incoming.Msg_Text + "\""
            );
        }

        sys_mcb.Swapper.set_task_note(
            demo.writer_a_id,
            "Validated read-back from its own partition and inspected mailbox."
        );
        set_demo_step(
            6,
            "Task A read-back and mailbox receive",
            "Task A read back its own partition and emptied its mailbox by receiving Task B's update."
        );
        demo.writer_a_phase = 2;
        record_snapshot("Task A read-back and mailbox receive");
        return;
    }

    if (demo.writer_a_phase == 2) {
        if (demo.waiter_d_handle >= 0) {
            const int rc = sys_mcb.MemMgr.Mem_Write(demo.waiter_d_handle, '!');
            push_memory_event();
            if (rc == -1) {
                demo.saw_protection = true;
                push_event("Task A attempted a foreign write into Task D memory and the MMU rejected it.");
            }
        }

        sys_mcb.MemMgr.Mem_Free(demo.writer_a_handle);
        push_memory_event();
        update_coalesce_snapshot(
            "Task A free snapshot",
            "Task A released the front partition. The freed bytes turned into hashes first, and the coalescer could only merge with adjacent free space after ownership barriers were removed."
        );
        sys_mcb.Swapper.kill_task(demo.writer_a_id);
        set_demo_step(
            10,
            "Task A protection test and free",
            "Task A proved base-and-limit protection with a blocked foreign write, then freed its own partition and exited."
        );
        demo.writer_a_phase = 3;
        record_snapshot("Task A protection test and free");
    }
}

static void writer_b_task() {
    if (demo.writer_b_phase == 0) {
        demo.writer_b_handle = sys_mcb.MemMgr.Mem_Alloc(200);
        push_memory_event();
        if (demo.writer_b_handle < 0) {
            set_demo_step(3, "Task B allocation and service message", "Task B failed unexpectedly during initial allocation.");
            sys_mcb.Swapper.kill_task(demo.writer_b_id);
            record_snapshot("Task B unexpected allocation failure");
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
        push_event("Task B sent a service-style IPC note to Task A.");
        sys_mcb.Swapper.set_task_note(
            demo.writer_b_id,
            "Allocated 200 bytes (256 reserved) in the middle of core memory."
        );
        set_demo_step(
            3,
            "Task B allocation and service message",
            "Task B allocated the middle partition and sent a service-style IPC update to Task A."
        );
        demo.writer_b_phase = 1;
        record_snapshot("Task B allocation and service message");
        return;
    }

    if (demo.writer_b_phase == 1) {
        sys_mcb.MemMgr.Mem_Free(demo.writer_b_handle);
        push_memory_event();
        update_coalesce_snapshot(
            "Task B free snapshot",
            "Task B freed the middle partition. The just-released region is visible as hashes, and Mem_Coalesce performed zero merges because Task A and Task C still bound the hole."
        );
        sys_mcb.Swapper.kill_task(demo.writer_b_id);
        set_demo_step(
            7,
            "Task B free and first coalesce view",
            "Task B freed the middle partition, exposing a visible hash band while the ledger kept that middle hole as a separate free segment."
        );
        demo.writer_b_phase = 2;
        record_snapshot("Task B free and first coalesce view");
    }
}

static void reader_c_task() {
    if (demo.reader_c_phase == 0) {
        demo.reader_c_handle = sys_mcb.MemMgr.Mem_Alloc(100);
        push_memory_event();
        if (demo.reader_c_handle < 0) {
            set_demo_step(4, "Task C single-byte write and mailbox receive", "Task C failed unexpectedly during initial allocation.");
            sys_mcb.Swapper.kill_task(demo.reader_c_id);
            record_snapshot("Task C unexpected allocation failure");
            return;
        }

        demo.saw_alloc_success = true;
        const std::string payload = "Task_C streams single-byte writes into its partition.";
        for (char ch : payload) {
            sys_mcb.MemMgr.Mem_Write(demo.reader_c_handle, ch);
        }
        push_event("Task C completed single-byte Mem_Write calls across its partition.");

        Message incoming {};
        if (sys_mcb.Messenger.Message_Receive(demo.reader_c_id, &incoming) == 1) {
            demo.reader_c_mailbox_text = incoming.Msg_Text;
            note_mailbox_receive(demo.reader_c_id, incoming);
            push_event(
                "Task C received mailbox text from "
                + task_label(incoming.Source_Task_Id)
                + ": \"" + incoming.Msg_Text + "\""
            );
        }

        sys_mcb.Swapper.set_task_note(
            demo.reader_c_id,
            "Allocated 100 bytes (128 reserved), streamed writes, and consumed IPC text."
        );
        set_demo_step(
            4,
            "Task C single-byte write and mailbox receive",
            "Task C proved current_location advances through single-byte writes, then consumed Task A's mailbox text."
        );
        demo.reader_c_phase = 1;
        record_snapshot("Task C single-byte write and mailbox receive");
        return;
    }

    if (demo.reader_c_phase == 1) {
        sys_mcb.MemMgr.Mem_Free(demo.reader_c_handle);
        push_memory_event();
        update_coalesce_snapshot(
            "Task C free snapshot",
            "Task C freed the trailing partition, letting the middle hole, the trailing block, and the tail free space merge into one 832-byte coalesced region. The hashes were replaced with dots after coalescing."
        );

        if (sys_mcb.Swapper.get_task_state(demo.waiter_d_id) == READY) {
            demo.saw_shark_pond = true;
            push_event("Shark-pond wake-up: Task D returned to READY after Task C freed memory.");
        }

        sys_mcb.Swapper.kill_task(demo.reader_c_id);
        set_demo_step(
            8,
            "Task C free and shark-pond wake-up",
            "Task C freed the trailing partition, coalescing the free space and returning Task D from BLOCKED to READY."
        );
        demo.reader_c_phase = 2;
        record_snapshot("Task C free and shark-pond wake-up");
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
                "Task D blocked on insufficient contiguous memory",
                "Task D requested 500 bytes, but the largest contiguous free region was only 448 bytes, so it became BLOCKED."
            );
            record_snapshot("Task D blocked on insufficient contiguous memory");
            return;
        }

        if (demo.saw_blocked_request) {
            demo.saw_shark_pond = true;
            demo.waiter_status = "Woke from shark pond and acquired the coalesced partition.";
            push_event("Task D woke after the shark-pond retry path and won the coalesced segment.");
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
        push_event("Task D sent a notification to Task A after acquiring coalesced memory.");
        sys_mcb.Swapper.set_task_note(
            demo.waiter_d_id,
            "Acquired 500 bytes (512 reserved) after the shark-pond wake-up."
        );
        set_demo_step(
            9,
            "Task D acquired coalesced partition",
            "Task D retried Mem_Alloc after coalescing, acquired 512 reserved bytes, and wrote into the coalesced region."
        );
        demo.waiter_d_phase = 1;
        record_snapshot("Task D acquired coalesced partition");
        return;
    }

    if (demo.waiter_d_phase == 1) {
        sys_mcb.MemMgr.Mem_Free(demo.waiter_d_handle);
        push_memory_event();
        update_coalesce_snapshot(
            "Task D free snapshot",
            "Task D released the coalesced partition. The final free path merged the remaining regions back into one 1024-byte segment filled with dots."
        );
        sys_mcb.Swapper.kill_task(demo.waiter_d_id);
        set_demo_step(
            11,
            "Task D final free",
            "Task D freed the coalesced partition, and the full core returned to one contiguous free region."
        );
        demo.waiter_d_phase = 2;
        record_snapshot("Task D final free");
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

    push_event("Created Task_A_Writer id=" + task_id_code(demo.writer_a_id));
    push_event("Created Task_B_Writer id=" + task_id_code(demo.writer_b_id));
    push_event("Created Task_C_Reader id=" + task_id_code(demo.reader_c_id));
    push_event("Created Task_D_Waiter id=" + task_id_code(demo.waiter_d_id));

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
        out << "  " << display_text(entry) << '\n';
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
