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

/* =========================================================================
 *  ncurses Panel Abstraction  (Stewart Pawley)
 * ========================================================================= */
struct Panel {
    WINDOW* win   = nullptr;
    int     rows  = 0;
    int     cols  = 0;
    int     y     = 0;
    int     x     = 0;
    std::string title;

    void create(int r, int c, int py, int px, const std::string& t) {
        rows = r; cols = c; y = py; x = px; title = t;
        win = newwin(rows, cols, y, x);
        box(win, 0, 0);
        draw_title();
        wrefresh(win);
    }

    void draw_title() {
        if (!win || title.empty()) return;
        wattron(win, A_BOLD | COLOR_PAIR(1));
        mvwprintw(win, 0, 2, " %s ", title.c_str());
        wattroff(win, A_BOLD | COLOR_PAIR(1));
    }

    void clear_content() {
        if (!win) return;
        for (int r = 1; r < rows - 1; ++r) {
            wmove(win, r, 1);
            wclrtoeol(win);
            /* re-draw right border */
            mvwaddch(win, r, cols - 1, ACS_VLINE);
        }
    }

    /* Write multi-line text starting at row offset inside the panel */
    void write_text(const std::string& text, int start_row = 1, int color_pair = 0) {
        if (!win) return;
        int r = start_row;
        int usable_w = cols - 2;
        if (usable_w < 1) return;
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line) && r < rows - 1) {
            if (color_pair > 0) wattron(win, COLOR_PAIR(color_pair));
            mvwprintw(win, r, 1, "%-*.*s", usable_w, usable_w, line.c_str());
            if (color_pair > 0) wattroff(win, COLOR_PAIR(color_pair));
            ++r;
        }
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
        box(win, 0, 0);
        draw_title();
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
static Panel mail1_panel;
static Panel mail2_panel;
static Panel mail3_panel;
static Panel log_panel;
static Panel status_panel;

static void create_layout() {
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);

    int hdr_h     = 4;
    int status_h  = 3;

    /* Divide remaining vertical space */
    int remaining = term_rows - hdr_h - status_h;
    int mid_h     = std::max(14, remaining * 35 / 100);
    int bot_h     = std::max(10, remaining * 30 / 100);
    int log_h     = remaining - mid_h - bot_h;
    if (log_h < 6) log_h = 6;

    int half_w    = term_cols / 2;
    int third_w   = term_cols / 3;
    int third_rem = term_cols - third_w * 2;

    /* Row 0: Header */
    header_panel.create(hdr_h, term_cols, 0, 0,
                        "ULTIMA 2.0 -- Phase 2: Message Passing (IPC)");

    /* Mid row: Scheduler (left) | IPC Dump (right) */
    int mid_y = hdr_h;
    scheduler_panel.create(mid_h, half_w, mid_y, 0, "SCHEDULER");
    ipc_panel.create(mid_h, term_cols - half_w, mid_y, half_w,
                     "IPC MAILBOX STATUS");

    /* Bottom row: 3 mailbox panels */
    int bot_y = mid_y + mid_h;
    mail1_panel.create(bot_h, third_w, bot_y, 0, "TASK 1 MAILBOX");
    mail2_panel.create(bot_h, third_w, bot_y, third_w, "TASK 2 MAILBOX");
    mail3_panel.create(bot_h, third_rem, bot_y, third_w * 2, "TASK 3 MAILBOX");

    /* Event log */
    int log_y = bot_y + bot_h;
    log_panel.create(log_h, term_cols, log_y, 0, "EVENT LOG");

    /* Status bar */
    status_panel.create(status_h, term_cols, log_y + log_h, 0, "");
}

static void destroy_layout() {
    header_panel.destroy();
    scheduler_panel.destroy();
    ipc_panel.destroy();
    mail1_panel.destroy();
    mail2_panel.destroy();
    mail3_panel.destroy();
    log_panel.destroy();
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
    header_panel.write_text(
        "Architecture: IPC via Message Passing (Mailboxes)  |  "
        "Semaphore-protected critical sections", 2);
    header_panel.refresh_panel();
}

static void refresh_scheduler() {
    scheduler_panel.clear_content();
    std::string dump = mcb.Swapper.dump_string(1);
    scheduler_panel.write_text(dump, 1);
    scheduler_panel.refresh_panel();
}

static void refresh_ipc() {
    ipc_panel.clear_content();
    std::string dump = mcb.Messenger.ipc_Message_Dump_String();
    ipc_panel.write_text(dump, 1);
    ipc_panel.refresh_panel();
}

static void refresh_mailbox(Panel& panel, int task_id) {
    panel.clear_content();
    std::string tbl = mcb.Messenger.get_mailbox_table(task_id);
    panel.write_text(tbl, 1);
    panel.refresh_panel();
}

static void refresh_mailboxes(int t1, int t2, int t3) {
    refresh_mailbox(mail1_panel, t1);
    refresh_mailbox(mail2_panel, t2);
    refresh_mailbox(mail3_panel, t3);
}

static void refresh_log() {
    log_panel.clear_content();
    auto entries = EventLog::instance().get_all();
    int usable_rows = log_panel.rows - 2;
    /* Show the most recent entries that fit */
    int start = 0;
    if (static_cast<int>(entries.size()) > usable_rows)
        start = static_cast<int>(entries.size()) - usable_rows;

    int row = 1;
    for (int i = start;
         i < static_cast<int>(entries.size()) && row < log_panel.rows - 1;
         ++i, ++row) {
        /* Color-code by entry type */
        int color = 0;
        if (entries[i].find("MSG SENT")    != std::string::npos) color = 4;
        else if (entries[i].find("MSG RECV") != std::string::npos) color = 5;
        else if (entries[i].find("Semaphore") != std::string::npos) color = 6;
        else if (entries[i].find("STEP")      != std::string::npos) color = 2;

        if (color) log_panel.write_highlight(entries[i], row, color);
        else       log_panel.write_text(entries[i], row);
    }
    log_panel.refresh_panel();
}

static void set_status(const std::string& msg) {
    status_panel.clear_content();
    status_panel.write_highlight(msg + "   [ Press any key ]", 1, 2);
    status_panel.refresh_panel();
}

static void refresh_all(int t1, int t2, int t3) {
    refresh_header();
    refresh_scheduler();
    refresh_ipc();
    refresh_mailboxes(t1, t2, t3);
    refresh_log();
}

static void wait_key() {
    doupdate();
    getch();
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
        init_pair(1, COLOR_CYAN,    -1);   /* titles                */
        init_pair(2, COLOR_YELLOW,  -1);   /* status / step labels  */
        init_pair(3, COLOR_GREEN,   -1);   /* header info           */
        init_pair(4, COLOR_GREEN,   -1);   /* MSG SENT              */
        init_pair(5, COLOR_CYAN,    -1);   /* MSG RECV              */
        init_pair(6, COLOR_MAGENTA, -1);   /* Semaphore traces      */
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
        "--- STEP 2: Task 1 sends TEXT greeting to Task 2 ---");
    mcb.Swapper.yield();  /* Task 1 → RUNNING */
    mcb.Messenger.Message_Send(t1, t2, "Hello, Team Thunder!", 0);
    set_status("STEP 2/13 :  Task 1 sends \"Hello, Team Thunder!\" to Task 2");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 3: State after Task 1 send
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 3: Observe -- Task 2 window now shows Team Thunder greeting ---");
    set_status("STEP 3/13 :  Window 2 now says \"Hello, Team Thunder!\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 4: Task 2 sends Service to Task 3
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 4: Task 2 sends SERVICE greeting to Task 3 ---");
    mcb.Swapper.yield();  /* Task 2 → RUNNING */
    mcb.Messenger.Message_Send(t2, t3, "Team Thunder is my favorite Group!!!!", 1);
    set_status("STEP 4/13 :  Task 2 sends favorite-group SERVICE message to Task 3");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 5: State after both sends
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 5: Observe -- Task 3 window now shows the favorite-group message ---");
    set_status(
        "STEP 5/13 :  Window 3 now says \"Team Thunder is my favorite Group!!!!\"");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 6: Task 3 receives message #1 (Text)
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 6: Task 3 sends NOTIFICATION greeting to Task 1 ---");
    mcb.Swapper.yield();  /* Task 3 → RUNNING */
    mcb.Messenger.Message_Send(t3, t1, "Hello, Professor Hakimzadeh!", 2);
    set_status("STEP 6/13 :  Task 3 sends \"Hello, Professor Hakimzadeh!\" to Task 1");
    refresh_all(t1, t2, t3);
    wait_key();

    /* ================================================================
     *  STEP 7: Task 3 receives message #2 (Service) + replies
     * ================================================================ */
    EventLog::instance().add(
        "--- STEP 7: Observe -- all three windows now display the requested greetings ---");
    set_status(
        "STEP 7/13 :  Window 1=Professor  Window 2=Team Thunder  Window 3=Favorite Group");
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
        "STEP 8/13 :  Task 1 reads \"Hello, Professor Hakimzadeh!\"");
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
        "STEP 9/13 :  Task 2 reads \"Hello, Team Thunder!\"");
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
    set_status("STEP 10/13 :  Task 3 reads \"Team Thunder is my favorite Group!!!!\"");
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
    mcb.Swapper.garbage_collect();
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
