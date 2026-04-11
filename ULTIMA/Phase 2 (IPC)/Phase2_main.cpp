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
 *
 * The UI shows windowed panels for the Scheduler, IPC Mailboxes, and an
 * Event Log.  Execution advances one step per keypress.
 * =========================================================================
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <ctime>
#include <functional>
#include <sstream>
#include <vector>
#include <algorithm>
#include <queue>

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
static const std::string kProfessorGreeting =
    "Hello Professor Hakimzadeh!!!";
static const std::string kTeamGreeting =
    "Hello, Team Thunder! It's nice to hear from my favorite group";
static const std::string kTeamCelebration =
    "Team Thunder GETS AN A+";

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
            std::size_t cursor = 0;
            if (line.empty()) {
                mvwprintw(win, r, 1, "%-*.*s", usable_w, usable_w, "");
                ++r;
                continue;
            }

            while (cursor < line.size() && r < rows - 1) {
                std::size_t remaining = line.size() - cursor;
                std::size_t segment_len = std::min<std::size_t>(usable_w, remaining);

                if (segment_len < remaining) {
                    std::size_t split = line.rfind(' ', cursor + segment_len);
                    if (split != std::string::npos && split >= cursor) {
                        segment_len = split - cursor;
                        if (segment_len == 0) {
                            segment_len = std::min<std::size_t>(usable_w, remaining);
                        }
                    }
                }

                std::string segment = line.substr(cursor, segment_len);
                if (color_pair > 0) wattron(win, COLOR_PAIR(color_pair));
                mvwprintw(win, r, 1, "%-*.*s", usable_w, usable_w, segment.c_str());
                if (color_pair > 0) wattroff(win, COLOR_PAIR(color_pair));
                ++r;

                cursor += segment_len;
                while (cursor < line.size() && line[cursor] == ' ') {
                    ++cursor;
                }
            }
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

    bool first = true;
    for (const auto& task : tasks) {
        if (!first) {
            out << " | ";
        }
        out << "T-" << task.task_id << ":" << mcb.Swapper.get_task_state_name(task.task_id);
        first = false;
    }
    return out.str();
}

static std::string format_ipc_panel() {
    std::ostringstream out;
    out << "Total queued: " << mcb.Messenger.Message_Count() << "\n";

    auto tasks = mcb.Swapper.snapshot_tasks();
    out << "Mailbox counts:";
    for (const auto& task : tasks) {
        out << "  T-" << task.task_id
            << '=' << mcb.Messenger.Message_Count(task.task_id);
    }

    out << "\nMailbox semaphores protect send/receive.\n";
    return out.str();
}

static std::string format_mailbox_panel(int task_id) {
    if (!compact_layout) {
        return mcb.Messenger.get_mailbox_table(task_id);
    }

    std::queue<Message> mailbox_copy = mcb.Messenger.get_mailbox_copy(task_id);
    std::ostringstream out;
    out << "Task " << task_id << "  Count " << mailbox_copy.size() << "\n";

    if (mailbox_copy.empty()) {
        out << "(empty)\n";
        return out.str();
    }

    while (!mailbox_copy.empty()) {
        const Message& message = mailbox_copy.front();
        out << message.Source_Task_Id << " -> " << message.Destination_Task_Id
            << "  " << message.Msg_Type.Message_Type_Description
            << "  " << format_time_short(message.Message_Arrival_Time) << "\n";
        out << message.Msg_Text << "\n";
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
    }

    return out.str();
}

static bool is_system_event(const std::string& entry) {
    return entry.find("Semaphore") != std::string::npos
        || entry.find("garbage") != std::string::npos
        || entry.find("Garbage") != std::string::npos
        || entry.find("cleanup") != std::string::npos
        || entry.find("Cleanup") != std::string::npos;
}

static std::string format_system_event_entry(const std::string& entry) {
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

    if (entry.find("STEP 13") != std::string::npos) {
        return "STEP 13: cleanup / garbage collection";
    }

    return entry;
}

static void create_layout() {
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);
    clear();
    refresh();

    compact_layout = (term_rows <= 28 || term_cols <= 140);

    int hdr_h     = compact_layout ? 3 : 4;
    int status_h  = 5;

    int remaining = std::max(9, term_rows - hdr_h - status_h);

    /* Row 0: Header */
    header_panel.create(hdr_h, term_cols, 0, 0,
                        "ULTIMA 2.0 -- Phase 2: Message Passing (IPC)", 1);

    if (compact_layout) {
        int min_top_h = 5;
        int min_mail_h = 6;
        int min_log_h = 5;

        if (remaining < min_top_h + min_mail_h + min_log_h) {
            min_top_h = 4;
            min_mail_h = 4;
            min_log_h = 4;
        }

        int top_h = min_top_h;
        int mail_h = min_mail_h;
        int log_h = min_log_h;
        int extra_rows = std::max(0, remaining - (top_h + mail_h + log_h));
        log_h += extra_rows;

        int scheduler_w = std::max(24, term_cols / 5);
        int ipc_w = std::max(28, term_cols / 4);
        if (scheduler_w + ipc_w > term_cols - 28) {
            ipc_w = std::max(24, term_cols - scheduler_w - 28);
        }
        int dump_w = term_cols - scheduler_w - ipc_w;
        int third_w = term_cols / 3;
        int third_rem = term_cols - third_w * 2;
        int current_y = hdr_h;

        scheduler_panel.create(top_h, scheduler_w, current_y, 0, "SCHEDULER", 7);
        ipc_panel.create(top_h, ipc_w, current_y, scheduler_w, "IPC STATUS", 8);
        dump_panel.create(top_h, dump_w, current_y, scheduler_w + ipc_w, "IPC DUMP", 12);
        current_y += top_h;

        mail1_panel.create(mail_h, third_w, current_y, 0, "TASK 1 MAILBOX", 9);
        mail2_panel.create(mail_h, third_w, current_y, third_w, "TASK 2 MAILBOX", 10);
        mail3_panel.create(mail_h, third_rem, current_y, third_w * 2, "TASK 3 MAILBOX", 11);
        current_y += mail_h;

        int log_w = std::max(48, (term_cols * 2) / 3);
        int system_w = term_cols - log_w;
        log_panel.create(log_h, log_w, current_y, 0, "EVENT LOG", 12);
        system_panel.create(log_h, system_w, current_y, log_w, "SYSTEM EVENTS", 14);
        status_panel.create(status_h, term_cols, current_y + log_h, 0, "", 13);
    } else {
        int scheduler_h = 5;
        int ipc_h       = 4;
        int dump_h      = 6;
        int mail1_h     = 4;
        int mail2_h     = 4;
        int mail3_h     = 4;
        int log_h       = 5;

        int total = scheduler_h + ipc_h + dump_h + mail1_h + mail2_h + mail3_h + log_h;
        while (total > remaining) {
            bool reduced = false;
            int* panels[] = {&log_h, &dump_h, &ipc_h, &scheduler_h, &mail3_h, &mail2_h, &mail1_h};
            for (int* panel_h : panels) {
                if (*panel_h > 3 && total > remaining) {
                    --(*panel_h);
                    --total;
                    reduced = true;
                }
            }
            if (!reduced) {
                break;
            }
        }

        while (total < remaining) {
            ++log_h;
            ++total;
            if (total < remaining) {
                ++ipc_h;
                ++total;
            }
        }

        int current_y = hdr_h;
        scheduler_panel.create(scheduler_h, term_cols, current_y, 0, "SCHEDULER", 7);
        current_y += scheduler_h;

        ipc_panel.create(ipc_h, term_cols, current_y, 0, "IPC STATUS", 8);
        current_y += ipc_h;

        dump_panel.create(dump_h, term_cols, current_y, 0, "IPC DUMP", 12);
        current_y += dump_h;

        mail1_panel.create(mail1_h, term_cols, current_y, 0, "TASK 1 MAILBOX", 9);
        current_y += mail1_h;

        mail2_panel.create(mail2_h, term_cols, current_y, 0, "TASK 2 MAILBOX", 10);
        current_y += mail2_h;

        mail3_panel.create(mail3_h, term_cols, current_y, 0, "TASK 3 MAILBOX", 11);
        current_y += mail3_h;

        int log_w = std::max(56, (term_cols * 2) / 3);
        int system_w = term_cols - log_w;
        log_panel.create(log_h, log_w, current_y, 0, "EVENT LOG", 12);
        system_panel.create(log_h, system_w, current_y, log_w, "SYSTEM EVENTS", 14);
        status_panel.create(status_h, term_cols, current_y + log_h, 0, "", 13);
    }
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
    if (!compact_layout) {
        header_panel.write_text(
            "Architecture: IPC Mailboxes  |  Semaphore-protected critical sections", 2, 1);
    }
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
    int start = 0;

    auto estimate_wrapped_rows = [&](const std::string& entry) {
        const int usable_w = std::max(1, panel.cols - 2);
        int lines = 0;
        std::istringstream stream(entry);
        std::string line;
        while (std::getline(stream, line)) {
            const int length = std::max<int>(1, static_cast<int>(line.size()));
            lines += std::max(1, (length + usable_w - 1) / usable_w);
        }
        return std::max(1, lines);
    };

    int used_rows = 0;
    for (int i = static_cast<int>(entries.size()) - 1; i >= 0; --i) {
        const int entry_rows = estimate_wrapped_rows(entries[i]);
        if (used_rows + entry_rows > panel.rows - 2) {
            start = i + 1;
            break;
        }
        used_rows += entry_rows;
        start = i;
    }

    for (int i = start;
         i < static_cast<int>(entries.size()) && row < panel.rows - 1;
         ++i) {
        /* Color-code by entry type */
        int color = 0;
        if (entries[i].find("MSG SENT")    != std::string::npos) color = 4;
        else if (entries[i].find("MSG RECV") != std::string::npos) color = 5;
        else if (entries[i].find("Semaphore") != std::string::npos) color = 6;
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
        system_panel.write_text("Waiting for semaphore locks\nand garbage collection...", 1, 13);
        system_panel.refresh_panel();
        return;
    }

    int row = 1;
    int start = 0;

    auto estimate_wrapped_rows = [&](const std::string& entry) {
        const int usable_w = std::max(1, system_panel.cols - 2);
        const int length = std::max<int>(1, static_cast<int>(entry.size()));
        return std::max(1, (length + usable_w - 1) / usable_w);
    };

    int used_rows = 0;
    for (int i = static_cast<int>(entries.size()) - 1; i >= 0; --i) {
        const int entry_rows = estimate_wrapped_rows(entries[i]);
        if (used_rows + entry_rows > system_panel.rows - 2) {
            start = i + 1;
            break;
        }
        used_rows += entry_rows;
        start = i;
    }

    for (int i = start;
         i < static_cast<int>(entries.size()) && row < system_panel.rows - 1;
         ++i) {
        int color = (entries[i].find("Mailbox") != std::string::npos) ? 6 : 14;
        row = system_panel.write_text(entries[i], row, color);
    }
    system_panel.refresh_panel();
}

static void set_status(const std::string& msg) {
    status_panel.clear_content();
    status_panel.write_text(msg, 1, 13);
    status_panel.write_highlight("[ Press any key ]", status_panel.rows - 2, 2);
    status_panel.refresh_panel();
}

static void refresh_all(int t1, int t2, int t3) {
    refresh_header();
    refresh_scheduler();
    refresh_ipc();
    refresh_dump();
    refresh_mailboxes(t1, t2, t3);
    refresh_log();
    refresh_system_events();
}

static void wait_key() {
    doupdate();
    getch();
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

int main() {
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

    /* ---- Init IPC ---- */
    mcb.Messenger.init(MAX_IPC_TASKS, &mcb.Swapper);
    EventLog::instance().add("IPC Messenger initialized — 16 mailbox slots allocated.");

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
    set_status("STEP 1/13 :  Initial state -- all tasks READY, mailboxes empty");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 2: Task 1 sends Text to Task 3
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 2: Task 3 sends NOTIFICATION greeting to Task 1 ---");
    dispatch_to_task(t3);
    mcb.Messenger.Message_Send(t3, t1, kProfessorGreeting.c_str(), 2);
    set_status("STEP 2/13 :  Task 3 sends \"" + kProfessorGreeting + "\" to Task 1");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 3: State after Task 1 send
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 3: Observe -- Task 1 window now shows Professor greeting ---");
    set_status("STEP 3/13 :  Window 1 now says \"" + kProfessorGreeting + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 4: Task 2 sends Service to Task 3
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 4: Task 1 sends TEXT greeting to Task 2 ---");
    dispatch_to_task(t1);
    mcb.Messenger.Message_Send(t1, t2, kTeamGreeting.c_str(), 0);
    set_status("STEP 4/13 :  Task 1 sends \"" + kTeamGreeting + "\" to Task 2");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 5: State after both sends
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 5: Observe -- Task 2 window now shows Team Thunder reply ---");
    set_status("STEP 5/13 :  Window 2 now says \"" + kTeamGreeting + "\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 6: Task 3 receives message #1 (Text)
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 6: Task 2 sends SERVICE celebration to Task 3 ---");
    dispatch_to_task(t2);
    mcb.Messenger.Message_Send(t2, t3, kTeamCelebration.c_str(), 1);
    set_status("STEP 6/13 :  Task 2 sends \"" + kTeamCelebration + "\" to Task 3");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 7: Task 3 receives message #2 (Service) + replies
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 7: Observe -- all three windows now display the requested greetings ---");
    set_status(
        "STEP 7/13 :  Window 1=\"" + kProfessorGreeting
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
    set_status(
        "STEP 8/13 :  Task 1 reads \"" + kProfessorGreeting + "\"");
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
    set_status(
        "STEP 9/13 :  Task 2 reads \"" + kTeamGreeting + "\"");
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
    set_status("STEP 10/13 :  Task 3 reads \"" + kTeamCelebration + "\"");
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
    set_status("STEP 11/13 :  Sent 3 messages to Task 1 for DeleteAll demo");
    refresh_all(t1, t2, t3);
    wait_key();

    /* Now delete them all */
    int deleted = mcb.Messenger.Message_DeleteAll(t1);
    EventLog::instance().add(
        "Message_DeleteAll(Task 1) removed "
        + std::to_string(deleted) + " message(s)");
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
    set_status("STEP 12/13 :  Message_Count -- Task2=1, Task3=2, Total=3");
    refresh_all(t1, t2, t3);
    wait_key();

    /* Clean up those messages */
    mcb.Messenger.Message_DeleteAll(t2);
    mcb.Messenger.Message_DeleteAll(t3);

    /* ================================================================
     *  STEP 13: Cleanup & exit
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 13: Task cleanup -- kill + garbage collect ---");
    mcb.Swapper.kill_task(t1);
    mcb.Swapper.kill_task(t2);
    mcb.Swapper.kill_task(t3);
    EventLog::instance().add("System cleanup: tasks marked DEAD, invoking garbage collector.");
    mcb.Swapper.garbage_collect();
    EventLog::instance().add("Garbage collector: " + mcb.Swapper.get_last_scheduler_event());
    EventLog::instance().add("All tasks killed and garbage collected.");
    EventLog::instance().add("====== PHASE 2 DEMONSTRATION COMPLETE ======");
    set_status(
        "STEP 13/13 :  All tasks cleaned up -- DEMO COMPLETE  "
        "[ Press any key to exit ]");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ---- Cleanup ncurses ---- */
    destroy_layout();
    endwin();

    /* Print transcript to stdout for the submission document */
    std::cout << "\n=== ULTIMA 2.0 Phase 2 -- Event Transcript ===\n\n";
    auto entries = EventLog::instance().get_all();
    for (const auto& e : entries)
        std::cout << "  " << e << "\n";
    std::cout << "\n=== END TRANSCRIPT ===\n";

    return 0;
}
