/* =========================================================================
 * Phase2_main.cpp — ULTIMA 2.0 Phase 2 ncurses Windowed Demonstration
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Primary Author : Stewart Pawley (MCB, Integration, ncurses UI, simulation)
 * Co-Authors     : Nicholas Kobs  (Test scenarios, utility demonstrations)
 *                  Zander Hayes   (IPC core referenced here)
 *
 * Phase Label    : Phase 2 – Message Passing (IPC)
 *
 * BUILD:
 *   make          (links ncurses automatically)
 *   ./phase2_test
 *   ./phase2_test --headless-transcript phase2_transcript.txt
 *
 * The UI shows windowed panels for the Scheduler, IPC Mailboxes, and an
 * Event Log.  Execution advances one step per keypress.
 * =========================================================================
 */

#include <iostream>
#include <array>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>
#include <algorithm>
#include <queue>

#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

/* ---- ncurses ---- */
#ifdef __APPLE__
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include "ipc.h"
#include "Sched.h"
#include "../Phase 1 (Scheduler)/Sema.h"

/* =========================================================================
 *  MCB — Master Control Block  (Stewart Pawley)
 * ========================================================================= */
struct MCB {
    Scheduler  Swapper;
    ipc        Messenger;
    Semaphore  Monitor;
    Semaphore  Printer;
    MCB() : Monitor("IPC_Monitor", 1, &Swapper),
            Printer("Printer_Output", 1, &Swapper) {}
};

static MCB       mcb;
static constexpr int MAX_IPC_TASKS = 16;
static constexpr int TOTAL_WIDTH = 156;
static const std::string kProfessorGreeting =
    "Hello Professor Hakimzadeh!!!";
static const std::string kTeamGreeting =
    "Hello Team Thunder!";
static const std::string kTeamCelebration =
    "Team Thunder GETS AN A+";
static const std::string kSecurityTheme =
    "Security | Privacy | Encryption";
static const std::string kSecureSecret =
    "THUNDER-SECURE-IPC";
static const std::string kSecurePayload =
    "Extra credit secure payload";
static constexpr int TOTAL_DEMO_STEPS = 17;
static bool g_headless_transcript_mode = false;
static std::string g_transcript_output_path;
static int g_current_demo_step = 0;
static std::string g_current_demo_label = "Awaiting first demo step";
static bool g_use_terminal_renderer = true;
static std::string g_status_message = "Phase 2 demo ready.";

/* =========================================================================
 *  ncurses Panel Abstraction  (Stewart Pawley)
 * ========================================================================= */
struct Panel {
    WINDOW* win   = nullptr;
    int     rows  = 0;
    int     cols  = 0;
    int     y     = 0;
    int     x     = 0;
    int     accent_pair = 1;
    std::string title;

    void create(int r, int c, int py, int px, const std::string& t, int accent = 1) {
        rows = r; cols = c; y = py; x = px; title = t; accent_pair = accent;
        win = newwin(rows, cols, y, x);
        draw_frame();
        wrefresh(win);
    }

    void draw_frame() {
        if (!win) return;
        if (accent_pair > 0) wattron(win, COLOR_PAIR(accent_pair));
        box(win, 0, 0);
        if (accent_pair > 0) wattroff(win, COLOR_PAIR(accent_pair));
        draw_title();
    }

    void draw_title() {
        if (!win || title.empty()) return;
        wattron(win, A_BOLD | COLOR_PAIR(accent_pair));
        mvwprintw(win, 0, 2, " %s ", title.c_str());
        wattroff(win, A_BOLD | COLOR_PAIR(accent_pair));
    }

    void clear_content() {
        if (!win) return;
        werase(win);
        draw_frame();
    }

    /* Write multi-line text starting at row offset inside the panel */
    int write_text(const std::string& text, int start_row = 1, int color_pair = 0) {
        if (!win) return start_row;
        int r = start_row;
        int usable_w = cols - 2;
        if (usable_w < 1) return r;
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line) && r < rows - 1) {
            if (color_pair > 0) wattron(win, COLOR_PAIR(color_pair));
            mvwprintw(win, r, 1, "%-*.*s", usable_w, usable_w, line.c_str());
            if (color_pair > 0) wattroff(win, COLOR_PAIR(color_pair));
            ++r;
        }
        return r;
    }

    /* Write a single highlighted line */
    void write_highlight(const std::string& text, int row, int pair) {
        if (!win) return;
        int usable_w = cols - 2;
        if (usable_w < 1) return;
        wattron(win, A_BOLD | COLOR_PAIR(pair));
        mvwprintw(win, row, 1, "%-*.*s", usable_w, usable_w, text.c_str());
        wattroff(win, A_BOLD | COLOR_PAIR(pair));
    }

    void refresh_panel() {
        if (!win) return;
        draw_frame();
        wrefresh(win);
    }

    void destroy() {
        if (win) { delwin(win); win = nullptr; }
    }
};

/* =========================================================================
 *  Layout  (Stewart Pawley)
 * ========================================================================= */
static Panel header_panel;
static Panel scheduler_panel;
static Panel ipc_panel;
static Panel dump_panel;
static Panel mail1_panel;
static Panel mail2_panel;
static Panel mail3_panel;
static Panel log_panel;
static Panel system_panel;
static Panel status_panel;
static bool compact_layout = false;
static constexpr int PANEL_GAP = 1;

static bool is_system_event(const std::string& entry);
static std::string format_system_event_entry(const std::string& entry);

static int ceil_div(int numerator, int denominator) {
    return (numerator + denominator - 1) / denominator;
}

static std::array<int, 3> compute_square_row_heights(
    int top_row_max_box_width,
    int mail_row_max_box_width,
    int bottom_row_max_box_width,
    int available_rows) {

    std::array<int, 3> targets = {
        std::max(6, ceil_div(top_row_max_box_width, 2)),
        std::max(6, ceil_div(mail_row_max_box_width, 2)),
        std::max(6, ceil_div(bottom_row_max_box_width, 2))
    };

    const int target_total = targets[0] + targets[1] + targets[2];
    if (target_total <= available_rows) {
        return targets;
    }

    std::array<int, 3> heights = {4, 4, 4};
    double scale = static_cast<double>(available_rows) / static_cast<double>(target_total);

    for (std::size_t index = 0; index < heights.size(); ++index) {
        heights[index] = std::max(4, static_cast<int>(std::floor(targets[index] * scale)));
    }

    int used_rows = heights[0] + heights[1] + heights[2];
    while (used_rows > available_rows) {
        for (int index = static_cast<int>(heights.size()) - 1;
             index >= 0 && used_rows > available_rows;
             --index) {
            if (heights[index] > 4) {
                --heights[index];
                --used_rows;
            }
        }
    }

    while (used_rows < available_rows) {
        for (std::size_t index = 0; index < heights.size() && used_rows < available_rows; ++index) {
            ++heights[index];
            ++used_rows;
        }
    }

    return heights;
}

static std::string format_time_short(std::time_t arrival_time) {
    char time_buffer[32] = {0};
    const std::tm* local_time = std::localtime(&arrival_time);
    if (local_time != nullptr) {
        std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", local_time);
    } else {
        std::strncpy(time_buffer, "N/A", sizeof(time_buffer) - 1);
    }
    return time_buffer;
}

static std::string format_message_preview(const Message& message) {
    if (message.Is_Encrypted) {
        std::string preview = message.Msg_Cipher_Text;
        if (preview.size() > 24) {
            preview = preview.substr(0, 24) + "...";
        }
        return "ENC:" + preview;
    }

    return message.Msg_Text;
}

static std::string format_demo_step_banner() {
    if (g_current_demo_step <= 0) {
        return "Demo flow ready";
    }

    std::ostringstream out;
    out << "Step " << g_current_demo_step << "/" << TOTAL_DEMO_STEPS << ": "
        << g_current_demo_label;
    return out.str();
}

static std::string format_security_posture() {
    if (g_current_demo_step <= 0) {
        return "Open IPC baseline visible now; Task 3 secure mailbox hardening is staged for later.";
    }
    if (g_current_demo_step < 13) {
        return "Plaintext IPC is visible by design so the later privacy + encryption upgrade is easy to compare.";
    }
    if (g_current_demo_step == 13) {
        return "Task 3 mailbox is now restricted and encryption-ready.";
    }
    if (g_current_demo_step == 14) {
        return "Unauthorized senders fail closed before ciphertext enters Task 3.";
    }
    if (g_current_demo_step == 15) {
        return "Task 3 now stores ciphertext-at-rest pending an authorized decrypt.";
    }
    if (g_current_demo_step == 16) {
        return "Unauthorized reads are denied; only the authorized decrypt path succeeds.";
    }
    return "Secure mailbox lifecycle is complete and the system has been cleaned up.";
}

static void mark_demo_step(int step_number, const std::string& label) {
    g_current_demo_step = step_number;
    g_current_demo_label = label;
}

struct TerminalRawMode {
    bool active = false;
#if !defined(_WIN32)
    termios original_state {};
#endif

    void enable() {
#if !defined(_WIN32)
        if (active || !isatty(STDIN_FILENO)) {
            return;
        }

        if (tcgetattr(STDIN_FILENO, &original_state) != 0) {
            return;
        }

        termios raw_state = original_state;
        raw_state.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
        raw_state.c_cc[VMIN] = 1;
        raw_state.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_state) == 0) {
            active = true;
        }
#endif
    }

    void restore() {
#if !defined(_WIN32)
        if (!active) {
            return;
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &original_state);
        active = false;
#endif
    }

    ~TerminalRawMode() {
        restore();
    }
};

static TerminalRawMode g_terminal_raw_mode;

static int get_terminal_measurement(int fallback, const char* env_name, bool columns) {
#if !defined(_WIN32)
    winsize window_size {};
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == 0) {
        const int measured = columns
            ? static_cast<int>(window_size.ws_col)
            : static_cast<int>(window_size.ws_row);
        if (measured > 0) {
            return measured;
        }
    }
#endif

    const char* env_value = std::getenv(env_name);
    if (env_value != nullptr) {
        const int parsed = std::atoi(env_value);
        if (parsed > 0) {
            return parsed;
        }
    }

    return fallback;
}

static int get_terminal_columns() {
    return get_terminal_measurement(TOTAL_WIDTH, "COLUMNS", true);
}

static bool stdout_is_tty() {
#if !defined(_WIN32)
    return isatty(STDOUT_FILENO);
#else
    return false;
#endif
}

static bool supports_ansi_color() {
    if (!stdout_is_tty()) {
        return false;
    }

    const char* term = std::getenv("TERM");
    return term == nullptr || std::string(term) != "dumb";
}

struct BorderStyle {
    std::string top_left;
    std::string top_right;
    std::string bottom_left;
    std::string bottom_right;
    std::string horizontal;
    std::string vertical;
};

static BorderStyle terminal_border_style() {
    return {"┌", "┐", "└", "┘", "─", "│"};
}

static std::string repeat_char(char value, int count) {
    return std::string(static_cast<std::size_t>(std::max(0, count)), value);
}

static std::string repeat_text(const std::string& value, int count) {
    std::string result;
    for (int index = 0; index < std::max(0, count); ++index) {
        result += value;
    }
    return result;
}

static std::size_t utf8_codepoint_length(unsigned char lead_byte) {
    if ((lead_byte & 0x80U) == 0) {
        return 1;
    }
    if ((lead_byte & 0xE0U) == 0xC0U) {
        return 2;
    }
    if ((lead_byte & 0xF0U) == 0xE0U) {
        return 3;
    }
    if ((lead_byte & 0xF8U) == 0xF0U) {
        return 4;
    }
    return 1;
}

static int terminal_cell_width(const std::string& text) {
    int width = 0;
    for (std::size_t index = 0; index < text.size();) {
        const unsigned char lead_byte = static_cast<unsigned char>(text[index]);
        index += std::min(utf8_codepoint_length(lead_byte), text.size() - index);
        ++width;
    }
    return width;
}

static std::string truncate_to_terminal_cells(const std::string& text, int max_width) {
    if (max_width <= 0) {
        return "";
    }

    std::size_t index = 0;
    std::size_t end = 0;
    int used = 0;
    while (index < text.size() && used < max_width) {
        const unsigned char lead_byte = static_cast<unsigned char>(text[index]);
        const std::size_t codepoint_length =
            std::min(utf8_codepoint_length(lead_byte), text.size() - index);
        end = index + codepoint_length;
        index = end;
        ++used;
    }

    return text.substr(0, end);
}

static std::string pad_right(const std::string& text, int width) {
    if (width <= 0) {
        return "";
    }

    const int display_width = terminal_cell_width(text);
    if (display_width >= width) {
        return truncate_to_terminal_cells(text, width);
    }

    return text + repeat_char(' ', width - display_width);
}

static std::vector<std::string> wrap_line(const std::string& text, int width) {
    std::vector<std::string> wrapped;
    if (width <= 0) {
        return wrapped;
    }

    if (text.empty()) {
        wrapped.push_back("");
        return wrapped;
    }

    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t index = start;
        std::size_t last_fit = start;
        std::size_t last_space = std::string::npos;
        int used = 0;

        while (index < text.size() && used < width) {
            const unsigned char lead_byte = static_cast<unsigned char>(text[index]);
            const std::size_t codepoint_length =
                std::min(utf8_codepoint_length(lead_byte), text.size() - index);
            if (text[index] == ' ') {
                last_space = index;
            }
            last_fit = index + codepoint_length;
            index = last_fit;
            ++used;
        }

        if (index >= text.size()) {
            wrapped.push_back(text.substr(start));
            break;
        }

        if (last_space != std::string::npos && last_space > start) {
            wrapped.push_back(text.substr(start, last_space - start));
            start = last_space + 1;
        } else {
            wrapped.push_back(text.substr(start, last_fit - start));
            start = last_fit;
        }

        while (start < text.size() && text[start] == ' ') {
            ++start;
        }
    }

    if (wrapped.empty()) {
        wrapped.push_back("");
    }

    return wrapped;
}

static std::vector<std::string> wrap_content_lines(const std::vector<std::string>& content_lines,
                                                   int inner_width) {
    std::vector<std::string> wrapped;
    for (const auto& line : content_lines) {
        const auto parts = wrap_line(line, inner_width);
        wrapped.insert(wrapped.end(), parts.begin(), parts.end());
    }
    if (wrapped.empty()) {
        wrapped.push_back("");
    }
    return wrapped;
}

static std::string colorize(const std::string& text, const char* ansi_code) {
    if (!supports_ansi_color() || ansi_code == nullptr || *ansi_code == '\0') {
        return text;
    }

    return std::string(ansi_code) + text + "\x1b[0m";
}

static std::vector<std::string> build_box_lines(const std::vector<std::string>& content_lines,
                                                int inner_width,
                                                int content_rows) {
    const BorderStyle border = terminal_border_style();
    std::vector<std::string> lines;
    if (inner_width < 1) {
        return lines;
    }

    const auto wrapped_lines = wrap_content_lines(content_lines, inner_width);
    const int padded_rows = std::max(static_cast<int>(wrapped_lines.size()), std::max(1, content_rows));

    lines.push_back(border.top_left + repeat_text(border.horizontal, inner_width) + border.top_right);
    for (int row = 0; row < padded_rows; ++row) {
        const std::string line = row < static_cast<int>(wrapped_lines.size())
            ? wrapped_lines[static_cast<std::size_t>(row)]
            : "";
        lines.push_back(border.vertical + pad_right(line, inner_width) + border.vertical);
    }
    lines.push_back(border.bottom_left + repeat_text(border.horizontal, inner_width) + border.bottom_right);

    return lines;
}

static std::vector<int> compute_box_total_widths(int total_width, int columns, int gap_count) {
    const int available_width = std::max(columns * 3, total_width - gap_count);
    const int base_width = std::max(3, available_width / columns);
    std::vector<int> widths(static_cast<std::size_t>(columns), base_width);
    const int remainder = available_width - (base_width * columns);
    if (!widths.empty()) {
        widths.back() += remainder;
    }
    return widths;
}

static void print_box_row(const std::vector<std::string>& left_box,
                          const char* left_color,
                          const std::vector<std::string>& middle_box,
                          const char* middle_color,
                          const std::vector<std::string>& right_box,
                          const char* right_color,
                          int left_margin) {
    for (std::size_t index = 0; index < left_box.size(); ++index) {
        std::cout << repeat_char(' ', left_margin)
                  << colorize(left_box[index], left_color) << ' '
                  << colorize(middle_box[index], middle_color) << ' '
                  << colorize(right_box[index], right_color) << '\n';
    }
}

static void print_box_row(const std::vector<std::string>& left_box,
                          const char* left_color,
                          const std::vector<std::string>& right_box,
                          const char* right_color,
                          int left_margin) {
    for (std::size_t index = 0; index < left_box.size(); ++index) {
        std::cout << repeat_char(' ', left_margin)
                  << colorize(left_box[index], left_color) << ' '
                  << colorize(right_box[index], right_color) << '\n';
    }
}

static std::string format_scheduler_panel() {
    if (!compact_layout) {
        return mcb.Swapper.dump_string(1);
    }

    std::ostringstream out;
    const auto tasks = mcb.Swapper.snapshot_tasks();

    out << "Tick:" << mcb.Swapper.get_scheduler_tick()
        << " Cur:T-" << mcb.Swapper.get_current_task_id()
        << " Rdy:" << mcb.Swapper.get_ready_task_count()
        << " Blk:" << mcb.Swapper.get_blocked_task_count()
        << " Dead:" << mcb.Swapper.get_dead_task_count() << "\n";

    for (const auto& task : tasks) {
        out << "T-" << task.task_id << ":" << mcb.Swapper.get_task_state_name(task.task_id);
        if (!task.last_transition.empty()) {
            out << "  (" << task.last_transition << ')';
        }
        out << "\n";
    }
    return out.str();
}

static std::string format_ipc_panel() {
    std::ostringstream out;
    out << kSecurityTheme << "\n";
    out << format_security_posture() << "\n";
    out << format_demo_step_banner() << "\n";
    out << "Total queued: " << mcb.Messenger.Message_Count() << "\n";

    auto tasks = mcb.Swapper.snapshot_tasks();
    out << "Mailbox counts:\n";
    for (const auto& task : tasks) {
        out << "T-" << task.task_id
            << ": " << mcb.Messenger.Message_Count(task.task_id) << "\n";
    }

    out << "\n" << mcb.Messenger.get_security_overview();
    out << "Mailbox semaphores protect send/receive.";
    return out.str();
}

static std::string format_mailbox_panel(int task_id) {
    if (!compact_layout) {
        return mcb.Messenger.get_mailbox_table(task_id);
    }

    std::queue<Message> mailbox_copy = mcb.Messenger.get_mailbox_copy(task_id);
    std::ostringstream out;
    out << "Task " << task_id << "  Count " << mailbox_copy.size() << "\n";
    out << mcb.Messenger.get_security_summary(task_id) << "\n";

    if (mailbox_copy.empty()) {
        out << "(empty)\n";
        return out.str();
    }

    while (!mailbox_copy.empty()) {
        const Message& message = mailbox_copy.front();
        out << message.Source_Task_Id << " -> " << message.Destination_Task_Id
            << "  " << message.Msg_Type.Message_Type_Description
            << "  " << format_time_short(message.Message_Arrival_Time) << "\n";
        out << format_message_preview(message) << "\n";
        mailbox_copy.pop();
        if (!mailbox_copy.empty()) {
            out << "---\n";
        }
    }

    return out.str();
}

static std::string format_dump_panel() {
    if (!compact_layout) {
        return mcb.Messenger.ipc_Message_Dump_String();
    }

    std::ostringstream out;
    auto tasks = mcb.Swapper.snapshot_tasks();
    for (const auto& task : tasks) {
        std::queue<Message> mailbox_copy = mcb.Messenger.get_mailbox_copy(task.task_id);
        out << "T-" << task.task_id << ": ";
        if (mailbox_copy.empty()) {
            out << "empty\n";
            continue;
        }

        const Message& message = mailbox_copy.front();
        out << mailbox_copy.size() << " msg  "
            << message.Source_Task_Id << "->" << message.Destination_Task_Id
            << " " << message.Msg_Type.Message_Type_Description
            << " " << format_time_short(message.Message_Arrival_Time) << "\n";
        out << "Lead msg: " << format_message_preview(message) << "\n";
    }

    return out.str();
}

static std::string short_message_type(const Message& message) {
    const std::string description = message.Msg_Type.Message_Type_Description;
    if (description == "Notification") {
        return "Note";
    }
    return description;
}

static std::vector<std::string> format_scheduler_terminal_lines() {
    std::vector<std::string> lines;
    std::ostringstream summary;
    const int current_task_id = mcb.Swapper.get_current_task_id();
    summary << "Tick:" << mcb.Swapper.get_scheduler_tick()
            << " Cur:" << (current_task_id >= 0 ? "T-" + std::to_string(current_task_id) : "Idle")
            << " Rdy:" << mcb.Swapper.get_ready_task_count()
            << " Blk:" << mcb.Swapper.get_blocked_task_count()
            << " Dead:" << mcb.Swapper.get_dead_task_count();
    lines.push_back(summary.str());

    for (const auto& task : mcb.Swapper.snapshot_tasks()) {
        std::ostringstream task_line;
        task_line << "T-" << task.task_id << ':'
                  << mcb.Swapper.get_task_state_name(task.task_id);
        if (!task.last_transition.empty()) {
            task_line << " (" << task.last_transition << ')';
        }
        lines.push_back(task_line.str());
    }

    return lines;
}

static std::vector<std::string> format_ipc_terminal_lines() {
    std::vector<std::string> lines;
    lines.push_back(kSecurityTheme);
    lines.push_back(format_security_posture());
    lines.push_back(format_demo_step_banner());
    lines.push_back("Total queued: " + std::to_string(mcb.Messenger.Message_Count()));
    lines.push_back("Mailbox counts:");

    for (const auto& task : mcb.Swapper.snapshot_tasks()) {
        lines.push_back(
            "T-" + std::to_string(task.task_id) + ": "
            + std::to_string(mcb.Messenger.Message_Count(task.task_id)));
    }

    return lines;
}

static std::vector<std::string> format_dump_terminal_lines() {
    std::vector<std::string> lines;
    for (const auto& task : mcb.Swapper.snapshot_tasks()) {
        std::queue<Message> mailbox_copy = mcb.Messenger.get_mailbox_copy(task.task_id);
        if (mailbox_copy.empty()) {
            lines.push_back("T-" + std::to_string(task.task_id) + ": empty");
            continue;
        }

        const Message& message = mailbox_copy.front();
        std::ostringstream line;
        line << "T-" << task.task_id
             << ": " << mailbox_copy.size() << " msg "
             << message.Source_Task_Id << "->" << message.Destination_Task_Id
             << ' ' << short_message_type(message)
             << ' ' << format_time_short(message.Message_Arrival_Time);
        lines.push_back(line.str());
        lines.push_back("Lead msg: " + format_message_preview(message));
    }

    return lines;
}

static std::vector<std::string> format_mailbox_terminal_lines(int task_id) {
    std::vector<std::string> lines;
    std::queue<Message> mailbox_copy = mcb.Messenger.get_mailbox_copy(task_id);

    lines.push_back("Task " + std::to_string(task_id) + "  Count " + std::to_string(mailbox_copy.size()));
    lines.push_back(mcb.Messenger.get_security_summary(task_id));
    if (mailbox_copy.empty()) {
        lines.push_back("(empty)");
        return lines;
    }

    const Message& message = mailbox_copy.front();
    std::ostringstream route;
    route << message.Source_Task_Id << " -> " << message.Destination_Task_Id
          << ' ' << short_message_type(message)
          << ' ' << format_time_short(message.Message_Arrival_Time);
    lines.push_back(route.str());
    lines.push_back(format_message_preview(message));

    return lines;
}

static std::vector<std::string> format_event_log_terminal_lines() {
    std::vector<std::string> lines;
    for (const auto& entry : EventLog::instance().get_all()) {
        if (!is_system_event(entry)) {
            lines.push_back(entry);
        }
    }

    if (lines.empty()) {
        lines.push_back("Waiting for message flow events...");
        return lines;
    }

    const int keep = 5;
    if (static_cast<int>(lines.size()) > keep) {
        lines.erase(lines.begin(), lines.end() - keep);
    }
    return lines;
}

static std::vector<std::string> format_system_events_terminal_lines() {
    std::vector<std::string> lines;
    for (const auto& entry : EventLog::instance().get_all()) {
        if (is_system_event(entry)) {
            lines.push_back(format_system_event_entry(entry));
        }
    }

    if (lines.empty()) {
        lines.push_back("Waiting for security, privacy, encryption, and mailbox semaphore activity");
        return lines;
    }

    const int keep = 5;
    if (static_cast<int>(lines.size()) > keep) {
        lines.erase(lines.begin(), lines.end() - keep);
    }
    return lines;
}

static bool is_system_event(const std::string& entry) {
    return entry.find("Semaphore") != std::string::npos
        || entry.find("SECURITY") != std::string::npos
        || entry.find("PRIVACY") != std::string::npos
        || entry.find("ENCRYPTION") != std::string::npos
        || entry.find("ACCESS DENIED") != std::string::npos
        || entry.find("MSG SENT SECURE") != std::string::npos
        || entry.find("MSG RECV SECURE") != std::string::npos
        || entry.find("ciphertext-at-rest") != std::string::npos
        || entry.find("decrypted secure payload") != std::string::npos
        || entry.find("garbage") != std::string::npos
        || entry.find("Garbage") != std::string::npos
        || entry.find("cleanup") != std::string::npos
        || entry.find("Cleanup") != std::string::npos;
}

static std::string format_system_event_entry(const std::string& entry) {
    if (entry.find("SECURITY STORY:") != std::string::npos) {
        return "Theme: secure baseline tracked from first frame";
    }

    if (entry.find("PRIVACY STORY:") != std::string::npos) {
        return "Theme: Task 3 privacy target established";
    }

    if (entry.find("ENCRYPTION STORY:") != std::string::npos) {
        return "Theme: ciphertext-at-rest arrives later in the demo";
    }

    if (entry.find("[Semaphore]") != std::string::npos) {
        std::string mailbox_id = "?";
        const std::size_t mailbox_pos = entry.find("mailbox ");
        if (mailbox_pos != std::string::npos) {
            std::size_t id_start = mailbox_pos + 8;
            std::size_t id_end = entry.find(' ', id_start);
            mailbox_id = entry.substr(id_start, id_end - id_start);
        }

        const bool entering = entry.find("ENTER") != std::string::npos;
        return "Mailbox " + mailbox_id + " semaphore " + (entering ? "DOWN" : "UP");
    }

    if (entry.find("System cleanup:") != std::string::npos) {
        return "Cleanup: tasks marked DEAD";
    }

    if (entry.find("Garbage collector:") != std::string::npos) {
        return "GC: removed DEAD tasks";
    }

    if (entry.find("All tasks killed and garbage collected.") != std::string::npos) {
        return "GC complete";
    }

    if (entry.find("SECURITY CONFIG:") != std::string::npos) {
        return "STEP 13: ACL + encryption enabled";
    }

    if (entry.find("SECURITY ACL:") != std::string::npos) {
        return "STEP 13: authorized sender recorded";
    }

    if (entry.find("ACCESS DENIED:") != std::string::npos
        && entry.find("send to secure mailbox") != std::string::npos) {
        return "STEP 14: unauthorized secure send denied";
    }

    if (entry.find("ACCESS DENIED:") != std::string::npos
        && entry.find("read secure mailbox") != std::string::npos) {
        return "STEP 16: unauthorized secure read denied";
    }

    if (entry.find("MSG SENT SECURE:") != std::string::npos) {
        return "STEP 15: encrypted payload queued";
    }

    if (entry.find("ciphertext-at-rest only") != std::string::npos) {
        return "STEP 15: ciphertext visible in mailbox";
    }

    if (entry.find("MSG RECV SECURE:") != std::string::npos) {
        return "STEP 16: authorized decrypt completed";
    }

    if (entry.find("decrypted secure payload") != std::string::npos) {
        return "STEP 16: plaintext delivered to T-3";
    }

    if (entry.find("STEP 17") != std::string::npos) {
        return "STEP 17: cleanup / garbage collection";
    }

    return entry;
}

static void create_layout() {
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);
    clear();
    refresh();

    compact_layout = true;

    const int total_width = std::min(TOTAL_WIDTH, term_cols);
    const int left_margin = std::max(0, (term_cols - total_width) / 2);
    const int header_h = 5;
    const int status_h = 6;
    const int available_rows = std::max(12, term_rows - header_h - status_h);

    const int total_three_width = std::max(9, total_width - (2 * PANEL_GAP));
    const int third_w1 = total_three_width / 3;
    const int third_w2 = total_three_width / 3;
    const int third_w3 = total_three_width - third_w1 - third_w2;

    const int total_two_width = std::max(6, total_width - PANEL_GAP);
    const int half_w1 = total_two_width / 2;
    const int half_w2 = total_two_width - half_w1;

    const auto row_heights = compute_square_row_heights(
        std::max({third_w1, third_w2, third_w3}),
        std::max({third_w1, third_w2, third_w3}),
        std::max(half_w1, half_w2),
        available_rows);
    const int top_h = row_heights[0];
    const int mail_h = row_heights[1];
    const int bottom_h = row_heights[2];

    header_panel.create(header_h, total_width, 0, left_margin,
                        "ULTIMA 2.0 -- Phase 2: Secure Message Passing (IPC)", 1);

    int current_y = header_h;
    scheduler_panel.create(top_h, third_w1, current_y, left_margin, "SCHEDULER / TRUSTED TASKS", 7);
    ipc_panel.create(top_h, third_w2, current_y, left_margin + third_w1 + PANEL_GAP, "SECURE IPC STATUS", 8);
    dump_panel.create(top_h, third_w3, current_y, left_margin + third_w1 + PANEL_GAP + third_w2 + PANEL_GAP, "PRIVACY / IPC DUMP", 12);
    current_y += top_h;

    mail1_panel.create(mail_h, third_w1, current_y, left_margin, "TASK 1 MAILBOX / PRIVACY", 9);
    mail2_panel.create(mail_h, third_w2, current_y, left_margin + third_w1 + PANEL_GAP, "TASK 2 MAILBOX / PRIVACY", 10);
    mail3_panel.create(mail_h, third_w3, current_y, left_margin + third_w1 + PANEL_GAP + third_w2 + PANEL_GAP, "TASK 3 SECURE MAILBOX", 11);
    current_y += mail_h;

    log_panel.create(bottom_h, half_w1, current_y, left_margin, "MESSAGE + PRIVACY LOG", 12);
    system_panel.create(bottom_h, half_w2, current_y, left_margin + half_w1 + PANEL_GAP, "SECURITY / SYSTEM EVENTS", 14);
    current_y += bottom_h;

    status_panel.create(status_h, total_width, current_y, left_margin, "", 13);
}

static void destroy_layout() {
    header_panel.destroy();
    scheduler_panel.destroy();
    ipc_panel.destroy();
    dump_panel.destroy();
    mail1_panel.destroy();
    mail2_panel.destroy();
    mail3_panel.destroy();
    log_panel.destroy();
    system_panel.destroy();
    status_panel.destroy();
}

/* =========================================================================
 *  UI Refresh Helpers  (Stewart Pawley)
 * ========================================================================= */

static void refresh_header() {
    header_panel.clear_content();
    header_panel.write_highlight(
        "Team Thunder #001  |  Stewart Pawley  |  Zander Hayes  |  Nicholas Kobs",
        1, 3);
    header_panel.write_highlight("Theme: " + kSecurityTheme, 2, 14);
    header_panel.write_text(
        "Architecture: IPC Mailboxes  |  ACL gate  |  Ciphertext-at-rest  |  " + format_demo_step_banner(),
        3, 1);
    header_panel.refresh_panel();
}

static void refresh_scheduler() {
    scheduler_panel.clear_content();
    std::string dump = format_scheduler_panel();
    scheduler_panel.write_text(dump, 1);
    scheduler_panel.refresh_panel();
}

static void refresh_ipc() {
    ipc_panel.clear_content();
    std::string dump = format_ipc_panel();
    ipc_panel.write_text(dump, 1);
    ipc_panel.refresh_panel();
}

static void refresh_dump() {
    dump_panel.clear_content();
    std::string dump = format_dump_panel();
    dump_panel.write_text(dump, 1);
    dump_panel.refresh_panel();
}

static void refresh_mailbox(Panel& panel, int task_id) {
    panel.clear_content();
    std::string tbl = format_mailbox_panel(task_id);
    panel.write_text(tbl, 1);
    panel.refresh_panel();
}

static void refresh_mailboxes(int t1, int t2, int t3) {
    refresh_mailbox(mail1_panel, t1);
    refresh_mailbox(mail2_panel, t2);
    refresh_mailbox(mail3_panel, t3);
}

static void refresh_event_panel(Panel& panel,
                                const std::function<bool(const std::string&)>& include_entry,
                                const std::string& empty_message) {
    panel.clear_content();
    auto all_entries = EventLog::instance().get_all();
    std::vector<std::string> entries;
    for (const auto& entry : all_entries) {
        if (include_entry(entry)) {
            entries.push_back(entry);
        }
    }

    if (entries.empty()) {
        panel.write_text(empty_message, 1, 13);
        panel.refresh_panel();
        return;
    }

    int row = 1;
    const int capacity = std::max(0, panel.rows - 2);
    const int start = std::max(0, static_cast<int>(entries.size()) - capacity);

    for (int i = start;
         i < static_cast<int>(entries.size()) && row < panel.rows - 1;
         ++i) {
        /* Color-code by entry type */
        int color = 0;
        if (entries[i].find("MSG SENT")    != std::string::npos) color = 4;
        else if (entries[i].find("MSG RECV") != std::string::npos) color = 5;
        else if (entries[i].find("Semaphore") != std::string::npos) color = 6;
        else if (entries[i].find("SECURITY") != std::string::npos
              || entries[i].find("ACCESS DENIED") != std::string::npos) color = 14;
        else if (entries[i].find("STEP")      != std::string::npos) color = 2;
        else if (entries[i].find("Garbage") != std::string::npos
              || entries[i].find("garbage") != std::string::npos
              || entries[i].find("cleanup") != std::string::npos
              || entries[i].find("Cleanup") != std::string::npos) color = 14;

        row = panel.write_text(entries[i], row, color);
    }
    panel.refresh_panel();
}

static void refresh_log() {
    refresh_event_panel(
        log_panel,
        [](const std::string& entry) { return !is_system_event(entry); },
        "Waiting for message flow events...");
}

static void refresh_system_events() {
    system_panel.clear_content();
    auto all_entries = EventLog::instance().get_all();
    std::vector<std::string> entries;
    for (const auto& entry : all_entries) {
        if (is_system_event(entry)) {
            entries.push_back(format_system_event_entry(entry));
        }
    }

    if (entries.empty()) {
        system_panel.write_text("Waiting for security policy,\nprivacy events, encryption, and cleanup...", 1, 13);
        system_panel.refresh_panel();
        return;
    }

    int row = 1;
    const int capacity = std::max(0, system_panel.rows - 2);
    const int start = std::max(0, static_cast<int>(entries.size()) - capacity);

    for (int i = start;
         i < static_cast<int>(entries.size()) && row < system_panel.rows - 1;
         ++i) {
        int color = (entries[i].find("Mailbox") != std::string::npos) ? 6 : 14;
        row = system_panel.write_text(entries[i], row, color);
    }
    system_panel.refresh_panel();
}

static void render_terminal_dashboard(int t1, int t2, int t3) {
    static constexpr const char* kHeaderColor = "\x1b[97m";
    static constexpr const char* kTeamColor = "\x1b[93m";
    static constexpr const char* kSchedulerColor = "\x1b[91m";
    static constexpr const char* kIpcColor = "\x1b[92m";
    static constexpr const char* kDumpColor = "\x1b[96m";
    static constexpr const char* kMailbox1Color = "\x1b[96m";
    static constexpr const char* kMailbox2Color = "\x1b[95m";
    static constexpr const char* kMailbox3Color = "\x1b[93m";
    static constexpr const char* kLogColor = "\x1b[96m";
    static constexpr const char* kSystemColor = "\x1b[96m";
    static constexpr const char* kStatusColor = "\x1b[97m";
    static constexpr const char* kPromptColor = "\x1b[95m";

    const int term_cols = std::max(60, get_terminal_columns());
    const int header_total_width = std::max(60, std::min(TOTAL_WIDTH, term_cols - 2));
    const int left_margin = std::max(0, (term_cols - header_total_width) / 2);
    const int header_inner_width = std::max(1, header_total_width - 2);

    const auto header_box = build_box_lines(
        {
            "ULTIMA 2.0 -- Phase 2: Secure Message Passing (IPC)",
            "Team Thunder #001  |  Stewart Pawley  |  Zander Hayes  |  Nicholas Kobs",
            "Security | Privacy | Encryption  |  " + format_demo_step_banner()
        },
        header_inner_width,
        3);

    const int total_width = header_total_width;
    const auto top_box_total_widths = compute_box_total_widths(total_width, 3, 2);

    auto scheduler_lines = format_scheduler_terminal_lines();
    scheduler_lines.insert(scheduler_lines.begin(), "SCHEDULER");
    auto ipc_lines = format_ipc_terminal_lines();
    ipc_lines.insert(ipc_lines.begin(), "SECURE IPC STATUS");
    auto dump_lines = format_dump_terminal_lines();
    dump_lines.insert(dump_lines.begin(), "PRIVACY / IPC DUMP");

    const int top_content_rows = std::max({
        5,
        static_cast<int>(wrap_content_lines(scheduler_lines, std::max(1, top_box_total_widths[0] - 2)).size()),
        static_cast<int>(wrap_content_lines(ipc_lines, std::max(1, top_box_total_widths[1] - 2)).size()),
        static_cast<int>(wrap_content_lines(dump_lines, std::max(1, top_box_total_widths[2] - 2)).size())
    });

    const auto scheduler_box = build_box_lines(
        scheduler_lines,
        std::max(1, top_box_total_widths[0] - 2),
        top_content_rows);

    const auto ipc_box = build_box_lines(
        ipc_lines,
        std::max(1, top_box_total_widths[1] - 2),
        top_content_rows);

    const auto dump_box = build_box_lines(
        dump_lines,
        std::max(1, top_box_total_widths[2] - 2),
        top_content_rows);

    auto mailbox_1_lines = format_mailbox_terminal_lines(t1);
    mailbox_1_lines.insert(mailbox_1_lines.begin(), "TASK 1 MAILBOX / PRIVACY");
    auto mailbox_2_lines = format_mailbox_terminal_lines(t2);
    mailbox_2_lines.insert(mailbox_2_lines.begin(), "TASK 2 MAILBOX / PRIVACY");
    auto mailbox_3_lines = format_mailbox_terminal_lines(t3);
    mailbox_3_lines.insert(mailbox_3_lines.begin(), "TASK 3 SECURE MAILBOX");

    const int mailbox_content_rows = std::max({
        4,
        static_cast<int>(wrap_content_lines(mailbox_1_lines, std::max(1, top_box_total_widths[0] - 2)).size()),
        static_cast<int>(wrap_content_lines(mailbox_2_lines, std::max(1, top_box_total_widths[1] - 2)).size()),
        static_cast<int>(wrap_content_lines(mailbox_3_lines, std::max(1, top_box_total_widths[2] - 2)).size())
    });

    const auto mailbox_1_box = build_box_lines(
        mailbox_1_lines,
        std::max(1, top_box_total_widths[0] - 2),
        mailbox_content_rows);

    const auto mailbox_2_box = build_box_lines(
        mailbox_2_lines,
        std::max(1, top_box_total_widths[1] - 2),
        mailbox_content_rows);

    const auto mailbox_3_box = build_box_lines(
        mailbox_3_lines,
        std::max(1, top_box_total_widths[2] - 2),
        mailbox_content_rows);

    const auto bottom_box_total_widths = compute_box_total_widths(total_width, 2, 1);
    auto log_lines = format_event_log_terminal_lines();
    log_lines.insert(log_lines.begin(), "MESSAGE + PRIVACY LOG");
    auto system_lines = format_system_events_terminal_lines();
    system_lines.insert(system_lines.begin(), "SECURITY / SYSTEM EVENTS");

    const int bottom_content_rows = std::max({
        5,
        static_cast<int>(wrap_content_lines(log_lines, std::max(1, bottom_box_total_widths[0] - 2)).size()),
        static_cast<int>(wrap_content_lines(system_lines, std::max(1, bottom_box_total_widths[1] - 2)).size())
    });

    const auto log_box = build_box_lines(
        log_lines,
        std::max(1, bottom_box_total_widths[0] - 2),
        bottom_content_rows);

    const auto system_box = build_box_lines(
        system_lines,
        std::max(1, bottom_box_total_widths[1] - 2),
        bottom_content_rows);

    const auto status_box = build_box_lines(
        {
            kSecurityTheme,
            format_security_posture(),
            g_status_message,
            "[ Press any key ]"
        },
        header_inner_width,
        4);

    if (stdout_is_tty()) {
        std::cout << "\x1b[2J\x1b[H";
    }
    for (std::size_t index = 0; index < header_box.size(); ++index) {
        const char* color = kHeaderColor;
        if (index == 2) {
            color = kTeamColor;
        } else if (index == 3) {
            color = kStatusColor;
        }
        std::cout << repeat_char(' ', left_margin) << colorize(header_box[index], color) << '\n';
    }
    print_box_row(scheduler_box, kSchedulerColor, ipc_box, kIpcColor, dump_box, kDumpColor, left_margin);
    print_box_row(mailbox_1_box, kMailbox1Color, mailbox_2_box, kMailbox2Color, mailbox_3_box, kMailbox3Color, left_margin);
    print_box_row(log_box, kLogColor, system_box, kSystemColor, left_margin);
    for (std::size_t index = 0; index < status_box.size(); ++index) {
        const char* color = (index == 4) ? kPromptColor : kStatusColor;
        std::cout << repeat_char(' ', left_margin) << colorize(status_box[index], color) << '\n';
    }
    std::cout.flush();
}

static void set_status(const std::string& msg) {
    g_status_message = msg;
    if (g_use_terminal_renderer) {
        return;
    }

    status_panel.clear_content();
    int row = 1;
    row = status_panel.write_text(kSecurityTheme, row, 14);
    row = status_panel.write_text(format_security_posture(), row, 8);
    row = status_panel.write_text(msg, row, 13);
    status_panel.write_highlight("[ Press any key ]", status_panel.rows - 2, 2);
    status_panel.refresh_panel();
}

static void refresh_all(int t1, int t2, int t3) {
    if (g_headless_transcript_mode) {
        return;
    }

    if (g_use_terminal_renderer) {
        render_terminal_dashboard(t1, t2, t3);
        return;
    }

    refresh_header();
    refresh_scheduler();
    refresh_ipc();
    refresh_dump();
    refresh_mailboxes(t1, t2, t3);
    refresh_log();
    refresh_system_events();
}

static void wait_key() {
    if (g_headless_transcript_mode) {
        return;
    }

    if (g_use_terminal_renderer) {
#if !defined(_WIN32)
        if (!isatty(STDIN_FILENO)) {
            return;
        }

        if (g_terminal_raw_mode.active) {
            char input = '\0';
            (void)read(STDIN_FILENO, &input, 1);
            return;
        }
#endif
        std::string discarded_line;
        std::getline(std::cin, discarded_line);
        return;
    }

    doupdate();
    flushinp();
    while (true) {
        int ch = getch();
        if (ch == KEY_RESIZE) {
            continue;
        }
        if (ch != ERR) {
            break;
        }
    }
}

static void emit_transcript(std::ostream& out) {
    out << "\n=== ULTIMA 2.0 Phase 2 -- Secure IPC Event Transcript ===\n\n";
    const auto entries = EventLog::instance().get_all();
    for (const auto& entry : entries) {
        out << "  " << entry << "\n";
    }
    out << "\n=== END TRANSCRIPT ===\n";
}

static void dispatch_to_task(int task_id) {
    for (int guard = 0;
         guard < (MAX_IPC_TASKS * 2) && mcb.Swapper.get_current_task_id() != task_id;
         ++guard) {
        mcb.Swapper.yield();
    }
}

/* =========================================================================
 *  Simulation Steps  (Nicholas Kobs — test logic / Stewart — integration)
 * ========================================================================= */

int main(int argc, char* argv[]) {
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--headless-transcript") {
            g_headless_transcript_mode = true;
            if ((index + 1) < argc) {
                g_transcript_output_path = argv[++index];
            }
        } else if (arg == "--transcript-output" && (index + 1) < argc) {
            g_transcript_output_path = argv[++index];
        } else if (arg == "--ncurses") {
            g_use_terminal_renderer = false;
        } else if (arg == "--plain-terminal") {
            g_use_terminal_renderer = true;
        }
    }

    if (!g_headless_transcript_mode && !g_use_terminal_renderer) {
        /* ---- ncurses init ---- */
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);

        if (has_colors()) {
            start_color();
            use_default_colors();
            if (can_change_color() && COLORS >= 16) {
                init_color(8,  930, 720, 930);  /* bubblegum pink */
                init_color(9,  620, 860, 1000); /* sky blue */
                init_color(10, 820, 760, 1000); /* lavender */
                init_color(11, 670, 980, 840);  /* mint */
                init_color(12, 1000, 820, 700); /* peach */
                init_color(13, 1000, 940, 700); /* butter */
                init_color(14, 1000, 640, 760); /* coral */
                init_color(15, 900, 720, 1000); /* orchid */

                init_pair(1,  15, -1);  /* header border/title   */
                init_pair(2,  13, -1);  /* prompt text           */
                init_pair(3,  11, -1);  /* author line           */
                init_pair(4,  12, -1);  /* MSG SENT              */
                init_pair(5,   9, -1);  /* MSG RECV              */
                init_pair(6,  14, -1);  /* semaphore traces      */
                init_pair(7,   9, -1);  /* scheduler panel       */
                init_pair(8,  10, -1);  /* ipc panel             */
                init_pair(9,  12, -1);  /* mailbox 1             */
                init_pair(10, 13, -1);  /* mailbox 2             */
                init_pair(11, 11, -1);  /* mailbox 3             */
                init_pair(12, 14, -1);  /* event log panel       */
                init_pair(13, 15, -1);  /* status message        */
                init_pair(14, 12, -1);  /* system events panel   */
            } else {
                init_pair(1, COLOR_MAGENTA, -1); /* header/title         */
                init_pair(2, COLOR_YELLOW,  -1); /* prompt text          */
                init_pair(3, COLOR_GREEN,   -1); /* author line          */
                init_pair(4, COLOR_RED,     -1); /* MSG SENT             */
                init_pair(5, COLOR_CYAN,    -1); /* MSG RECV             */
                init_pair(6, COLOR_MAGENTA, -1); /* semaphore traces     */
                init_pair(7, COLOR_CYAN,    -1); /* scheduler panel      */
                init_pair(8, COLOR_MAGENTA, -1); /* ipc panel            */
                init_pair(9, COLOR_RED,     -1); /* mailbox 1            */
                init_pair(10, COLOR_YELLOW, -1); /* mailbox 2            */
                init_pair(11, COLOR_GREEN,  -1); /* mailbox 3            */
                init_pair(12, COLOR_BLUE,   -1); /* event log panel      */
                init_pair(13, COLOR_WHITE,  -1); /* status message       */
                init_pair(14, COLOR_RED,    -1); /* system events panel  */
            }
        }

        create_layout();
    }

    if (!g_headless_transcript_mode && g_use_terminal_renderer) {
        g_terminal_raw_mode.enable();
    }

    /* ---- Init IPC ---- */
    mcb.Messenger.init(MAX_IPC_TASKS, &mcb.Swapper);
    EventLog::instance().add("IPC Messenger initialized — 16 mailbox slots allocated.");
    EventLog::instance().add("SECURITY STORY: baseline plaintext IPC is visible from the first frame.");
    EventLog::instance().add("PRIVACY STORY: Task 3 is the secure mailbox target for the full demo.");
    EventLog::instance().add("ENCRYPTION STORY: ciphertext-at-rest and authorized decrypt arrive later in the run.");

    /* ---- Create tasks ---- */
    int t1 = mcb.Swapper.create_task("Task_1_Window", nullptr);
    int t2 = mcb.Swapper.create_task("Task_2_Window", nullptr);
    int t3 = mcb.Swapper.create_task("Task_3_Window", nullptr);
    EventLog::instance().add("Created Task 1 (Professor window) id=" + std::to_string(t1));
    EventLog::instance().add("Created Task 2 (Team window)      id=" + std::to_string(t2));
    EventLog::instance().add("Created Task 3 (Favorite window)  id=" + std::to_string(t3));

    /* ================================================================
     *  STEP 1: Initial State
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 1: Initial State -- all tasks READY, all mailboxes empty ---");
    mark_demo_step(1, "Initial state");
    set_status("STEP 1/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Initial state -- all tasks READY, mailboxes empty");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 2: Task 1 sends Text to Task 3
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 2: Task 3 sends NOTIFICATION greeting to Task 1 ---");
    dispatch_to_task(t3);
    mcb.Messenger.Message_Send(t3, t1, kProfessorGreeting.c_str(), 2);
    mark_demo_step(2, "Task 3 sends notification to Task 1");
    set_status("STEP 2/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 3 sends \"" + kProfessorGreeting + "\" to Task 1");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 3: State after Task 1 send
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 3: Observe -- Task 1 window now shows Professor greeting ---");
    mark_demo_step(3, "Observe Task 1 greeting");
    set_status("STEP 3/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Window 1 now says \"" + kProfessorGreeting + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 4: Task 2 sends Service to Task 3
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 4: Task 1 sends TEXT greeting to Task 2 ---");
    dispatch_to_task(t1);
    mcb.Messenger.Message_Send(t1, t2, kTeamGreeting.c_str(), 0);
    mark_demo_step(4, "Task 1 sends text to Task 2");
    set_status("STEP 4/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 1 sends \"" + kTeamGreeting + "\" to Task 2");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 5: State after both sends
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 5: Observe -- Task 2 window now shows Team Thunder reply ---");
    mark_demo_step(5, "Observe Task 2 greeting");
    set_status("STEP 5/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Window 2 now says \"" + kTeamGreeting + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 6: Task 3 receives message #1 (Text)
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 6: Task 2 sends SERVICE celebration to Task 3 ---");
    dispatch_to_task(t2);
    mcb.Messenger.Message_Send(t2, t3, kTeamCelebration.c_str(), 1);
    mark_demo_step(6, "Task 2 sends service to Task 3");
    set_status("STEP 6/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 2 sends \"" + kTeamCelebration + "\" to Task 3");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 7: Task 3 receives message #2 (Service) + replies
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 7: Observe -- all three windows now display the requested greetings ---");
    mark_demo_step(7, "Observe all greetings");
    set_status(
        "STEP 7/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Window 1=\"" + kProfessorGreeting
        + "\"  Window 2=\"" + kTeamGreeting
        + "\"  Window 3=\"" + kTeamCelebration + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 8: Post-receive state
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 8: Task 1 reads its mailbox greeting from Task 3 ---");
    {
        Message incoming;
        mcb.Messenger.Message_Receive(t1, &incoming);
        std::ostringstream s;
        s << "Task 1 received: \"" << incoming.Msg_Text
          << "\" from T-" << incoming.Source_Task_Id
          << " [" << incoming.Msg_Type.Message_Type_Description << "]";
        EventLog::instance().add(s.str());
    }
    mark_demo_step(8, "Task 1 reads mailbox");
    set_status(
        "STEP 8/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 1 reads \"" + kProfessorGreeting + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 9: Task 2 reads its Notification
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 9: Task 2 reads its mailbox greeting from Task 1 ---");
    {
        Message reply;
        int rc = mcb.Messenger.Message_Receive(t2, &reply);
        if (rc == 1) {
            std::ostringstream s;
            s << "Task 2 received: \"" << reply.Msg_Text
              << "\" from T-" << reply.Source_Task_Id
              << " [" << reply.Msg_Type.Message_Type_Description << "]";
            EventLog::instance().add(s.str());
        }
    }
    mark_demo_step(9, "Task 2 reads mailbox");
    set_status(
        "STEP 9/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 2 reads \"" + kTeamGreeting + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 10: Final state — all mailboxes empty
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 10: Task 3 reads the SERVICE greeting from Task 2 ---");
    {
        Message reply;
        int rc = mcb.Messenger.Message_Receive(t3, &reply);
        if (rc == 1) {
            std::ostringstream s;
            s << "Task 3 received: \"" << reply.Msg_Text
              << "\" from T-" << reply.Source_Task_Id
              << " [" << reply.Msg_Type.Message_Type_Description << "]";
            EventLog::instance().add(s.str());
        }
    }
    mark_demo_step(10, "Task 3 reads service greeting");
    set_status("STEP 10/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 3 reads \"" + kTeamCelebration + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 11: Message_DeleteAll demonstration
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 11: Message_DeleteAll demonstration ---");
    mcb.Messenger.Message_Send(0, t1, "DeleteAll test msg 1", 0);
    mcb.Messenger.Message_Send(0, t1, "DeleteAll test msg 2", 1);
    mcb.Messenger.Message_Send(0, t1, "DeleteAll test msg 3", 2);
    EventLog::instance().add(
        "Sent 3 test messages (Text/Service/Notification) to Task 1");
    mark_demo_step(11, "DeleteAll demonstration");
    set_status("STEP 11/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Sent 3 messages to Task 1 for DeleteAll demo");
    refresh_all(t1, t2, t3);
    wait_key();

    /* Now delete them all */
    int deleted = mcb.Messenger.Message_DeleteAll(t1);
    EventLog::instance().add(
        "Message_DeleteAll(Task 1) removed "
        + std::to_string(deleted) + " message(s)");
    mark_demo_step(11, "DeleteAll completed");
    set_status(
        "STEP 11b :  Message_DeleteAll called -- Task 1 mailbox now empty ("
        + std::to_string(deleted) + " removed)");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 12: Message_Count demonstration
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 12: Message_Count demonstration ---");
    mcb.Messenger.Message_Send(0, t2, "Count demo A", 0);
    mcb.Messenger.Message_Send(0, t3, "Count demo B", 1);
    mcb.Messenger.Message_Send(0, t3, "Count demo C", 2);
    {
        int c2 = mcb.Messenger.Message_Count(t2);
        int c3 = mcb.Messenger.Message_Count(t3);
        int ct = mcb.Messenger.Message_Count();
        std::ostringstream s;
        s << "Message_Count: Task2=" << c2 << "  Task3=" << c3
          << "  Total=" << ct;
        EventLog::instance().add(s.str());
    }
    mark_demo_step(12, "Message_Count demonstration");
    set_status("STEP 12/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Message_Count -- Task2=1, Task3=2, Total=3");
    refresh_all(t1, t2, t3);
    wait_key();

    /* Clean up those messages */
    mcb.Messenger.Message_DeleteAll(t2);
    mcb.Messenger.Message_DeleteAll(t3);

    /* ================================================================
     *  STEP 13: Configure secure mailbox policy
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 13: Configure Task 3 secure mailbox (restricted access + encryption) ---");
    mcb.Messenger.Configure_Mailbox_Security(t3, kSecureSecret.c_str());
    mcb.Messenger.Allow_Mailbox_Sender(t3, t2);
    mark_demo_step(13, "Enable secure mailbox policy");
    set_status("STEP 13/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 3 mailbox now requires ACL + encryption");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 14: Unauthorized secure send blocked
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 14: Task 1 attempts unauthorized secure send to Task 3 ---");
    if (mcb.Messenger.Secure_Message_Send(t1, t3, "Unauthorized secure attempt", 1) != 1) {
        EventLog::instance().add("Task 1 secure send denied as expected.");
    }
    mark_demo_step(14, "Unauthorized secure send denied");
    set_status("STEP 14/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Unauthorized secure send to Task 3 is denied");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 15: Authorized secure send stores ciphertext at rest
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 15: Task 2 sends encrypted payload to Task 3 ---");
    dispatch_to_task(t2);
    if (mcb.Messenger.Secure_Message_Send(t2, t3, kSecurePayload.c_str(), 1) == 1) {
        EventLog::instance().add("Task 3 secure mailbox now holds ciphertext-at-rest only.");
    }
    mark_demo_step(15, "Encrypted payload stored at rest");
    set_status("STEP 15/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 2 sends encrypted payload to Task 3");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 16: Unauthorized read blocked, owner decrypts payload
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 16: Task 1 read denied; Task 3 decrypts secure payload ---");
    {
        Message blocked_probe {};
        if (mcb.Messenger.Secure_Message_Receive(t1, t3, &blocked_probe) != 1) {
            EventLog::instance().add("Task 1 secure read denied as expected.");
        }

        Message secure_reply {};
        int secure_rc = mcb.Messenger.Secure_Message_Receive(t3, t3, &secure_reply);
        if (secure_rc == 1) {
            std::ostringstream s;
            s << "Task 3 decrypted secure payload: \"" << secure_reply.Msg_Text
              << "\" from T-" << secure_reply.Source_Task_Id
              << " [" << secure_reply.Msg_Type.Message_Type_Description << "]";
            EventLog::instance().add(s.str());
        }
    }
    mark_demo_step(16, "Authorized decrypt after denied read");
    set_status("STEP 16/" + std::to_string(TOTAL_DEMO_STEPS) + " :  Task 3 decrypts authorized payload");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 17: Cleanup & exit
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 17: Task cleanup -- kill + garbage collect ---");
    mcb.Swapper.kill_task(t1);
    mcb.Swapper.kill_task(t2);
    mcb.Swapper.kill_task(t3);
    EventLog::instance().add("System cleanup: tasks marked DEAD, invoking garbage collector.");
    mcb.Swapper.garbage_collect();
    EventLog::instance().add("Garbage collector: " + mcb.Swapper.get_last_scheduler_event());
    EventLog::instance().add("All tasks killed and garbage collected.");
    EventLog::instance().add("====== PHASE 2 DEMONSTRATION COMPLETE ======");
    mark_demo_step(17, "Cleanup and garbage collection");
    set_status(
        "STEP 17/" + std::to_string(TOTAL_DEMO_STEPS) + " :  All tasks cleaned up -- DEMO COMPLETE  "
        "[ Press any key to exit ]");
    refresh_all(t1, t2, t3);
    wait_key();

    g_terminal_raw_mode.restore();

    if (!g_headless_transcript_mode && !g_use_terminal_renderer) {
        /* ---- Cleanup ncurses ---- */
        destroy_layout();
        endwin();
    }

    /* Print transcript to stdout for the submission document */
    emit_transcript(std::cout);

    if (!g_transcript_output_path.empty()) {
        std::ofstream transcript_file(g_transcript_output_path);
        if (transcript_file.is_open()) {
            emit_transcript(transcript_file);
        }
    }

    return 0;
}
