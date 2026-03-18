#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <ncurses.h>
#include "Sched.h"

/**
 * ULTIMA 2.0 - Phase 1 Main Driver
 * Lab 7 style ncurses layout with three pthread workers, one shared log,
 * one console window, mouse logging, and mutex-protected screen updates.
 */

Scheduler sys_scheduler;

namespace {

constexpr int kMinimumTerminalRows = 24;
constexpr int kMinimumTerminalCols = 80;
constexpr int kHorizontalMargin = 1;
constexpr int kHorizontalGap = 1;
constexpr int kVerticalGap = 1;

enum class ThreadState {
    started = 0,
    ready = 1,
    running = 2,
    blocked = 3,
    terminated = 4
};

struct ManagedWindow {
    WINDOW* frame = nullptr;
    WINDOW* body = nullptr;
    int height = 0;
    int width = 0;
    int start_y = 0;
    int start_x = 0;
};

struct Layout {
    int header_height = 0;
    int header_width = 0;
    int header_y = 0;
    int header_x = 0;

    int worker_height = 0;
    int worker_y = 0;
    int worker_width_1 = 0;
    int worker_width_2 = 0;
    int worker_width_3 = 0;
    int worker_x_1 = 0;
    int worker_x_2 = 0;
    int worker_x_3 = 0;

    int log_height = 0;
    int log_width = 0;
    int log_y = 0;
    int log_x = 0;

    int console_height = 0;
    int console_width = 0;
    int console_y = 0;
    int console_x = 0;
};

struct ThreadData {
    int thread_no = 0;
    ThreadState thread_state = ThreadState::started;
    ManagedWindow* thread_window = nullptr;
    ManagedWindow* log_window = nullptr;
    std::atomic_bool kill_signal { false };
    int sleep_time = 1;
    int thread_results = 0;
};

pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

ManagedWindow heading_window;
ManagedWindow thread_window_1;
ManagedWindow thread_window_2;
ManagedWindow thread_window_3;
ManagedWindow log_window;
ManagedWindow console_window;
Layout layout;

bool is_window_ready(const ManagedWindow& window) {
    return window.frame != nullptr && window.body != nullptr;
}

bool configure_layout() {
    if (LINES < kMinimumTerminalRows || COLS < kMinimumTerminalCols) {
        return false;
    }

    layout.header_y = 2;
    layout.header_x = kHorizontalMargin;
    layout.header_height = 5;
    layout.header_width = COLS - (kHorizontalMargin * 2);

    layout.worker_y = layout.header_y + layout.header_height;

    const int total_worker_width = COLS - (kHorizontalMargin * 2) - (kHorizontalGap * 2);
    const int base_worker_width = total_worker_width / 3;
    const int worker_remainder = total_worker_width - (base_worker_width * 3);

    layout.worker_width_1 = base_worker_width;
    layout.worker_width_2 = base_worker_width;
    layout.worker_width_3 = base_worker_width + worker_remainder;

    layout.worker_x_1 = kHorizontalMargin;
    layout.worker_x_2 = layout.worker_x_1 + layout.worker_width_1 + kHorizontalGap;
    layout.worker_x_3 = layout.worker_x_2 + layout.worker_width_2 + kHorizontalGap;

    const int remaining_rows = LINES - layout.worker_y;
    layout.worker_height = (remaining_rows - kVerticalGap) / 2;
    layout.log_y = layout.worker_y + layout.worker_height + kVerticalGap;
    layout.log_height = LINES - layout.log_y;

    layout.console_height = layout.log_height;
    layout.console_width = std::min(20, (COLS - (kHorizontalMargin * 2) - kHorizontalGap) / 3);
    layout.log_width = COLS - (kHorizontalMargin * 2) - kHorizontalGap - layout.console_width;
    layout.log_x = kHorizontalMargin;
    layout.console_x = layout.log_x + layout.log_width + kHorizontalGap;
    layout.console_y = layout.log_y;

    return layout.header_height >= 5
        && layout.worker_height >= 8
        && layout.log_height >= 8
        && layout.console_width >= 18
        && layout.log_width >= 40;
}

void draw_relations_unlocked() {
    if (!is_window_ready(heading_window)
        || !is_window_ready(thread_window_1)
        || !is_window_ready(thread_window_2)
        || !is_window_ready(thread_window_3)
        || !is_window_ready(log_window)) {
        return;
    }

    const int worker_top_y = layout.worker_y;
    const int log_top_y = layout.log_y;
    const int worker_one_center = layout.worker_x_1 + (layout.worker_width_1 / 2);
    const int worker_two_center = layout.worker_x_2 + (layout.worker_width_2 / 2);
    const int worker_three_center = layout.worker_x_3 + (layout.worker_width_3 / 2);

    if (has_colors()) {
        attron(COLOR_PAIR(2));
    }
    mvaddch(worker_top_y, worker_one_center, ACS_TTEE);
    mvaddch(worker_top_y, worker_two_center, ACS_TTEE);
    mvaddch(worker_top_y, worker_three_center, ACS_TTEE);
    if (has_colors()) {
        attroff(COLOR_PAIR(2));
        attron(COLOR_PAIR(1));
    }
    mvaddch(log_top_y, worker_one_center, ACS_BTEE);
    mvaddch(log_top_y, worker_two_center, ACS_BTEE);
    mvaddch(log_top_y, worker_three_center, ACS_BTEE);
    if (has_colors()) {
        attroff(COLOR_PAIR(1));
    }

    wnoutrefresh(stdscr);
}

void refresh_layout_unlocked() {
    ManagedWindow* windows[] = {
        &heading_window,
        &thread_window_1,
        &thread_window_2,
        &thread_window_3,
        &log_window,
        &console_window
    };

    for (ManagedWindow* window : windows) {
        if (!is_window_ready(*window)) {
            continue;
        }

        box(window->frame, 0, 0);
        wnoutrefresh(window->frame);
        wnoutrefresh(window->body);
    }

    draw_relations_unlocked();
    doupdate();
}

bool create_window(ManagedWindow& window, int height, int width, int start_y, int start_x) {
    pthread_mutex_lock(&screen_mutex);

    window.height = height;
    window.width = width;
    window.start_y = start_y;
    window.start_x = start_x;
    window.frame = newwin(height, width, start_y, start_x);

    if (window.frame == nullptr) {
        pthread_mutex_unlock(&screen_mutex);
        return false;
    }

    window.body = derwin(window.frame, height - 2, width - 2, 1, 1);
    if (window.body == nullptr) {
        delwin(window.frame);
        window.frame = nullptr;
        pthread_mutex_unlock(&screen_mutex);
        return false;
    }

    scrollok(window.body, TRUE);
    idlok(window.body, TRUE);

    sched_yield();
    refresh_layout_unlocked();
    pthread_mutex_unlock(&screen_mutex);
    return true;
}

void destroy_window(ManagedWindow& window) {
    pthread_mutex_lock(&screen_mutex);

    if (window.body != nullptr) {
        delwin(window.body);
        window.body = nullptr;
    }

    if (window.frame != nullptr) {
        delwin(window.frame);
        window.frame = nullptr;
    }

    pthread_mutex_unlock(&screen_mutex);
}

void write_window(ManagedWindow& window, const char* text) {
    pthread_mutex_lock(&screen_mutex);

    if (window.body != nullptr) {
        wprintw(window.body, "%s", text);
        sched_yield();
        refresh_layout_unlocked();
    }

    pthread_mutex_unlock(&screen_mutex);
}

void write_window(ManagedWindow& window, int y, int x, const char* text) {
    pthread_mutex_lock(&screen_mutex);

    if (window.body != nullptr) {
        mvwprintw(window.body, y, x, "%s", text);
        sched_yield();
        refresh_layout_unlocked();
    }

    pthread_mutex_unlock(&screen_mutex);
}

void clear_window(ManagedWindow& window) {
    pthread_mutex_lock(&screen_mutex);

    if (window.body != nullptr) {
        werase(window.body);
        wmove(window.body, 0, 0);
        refresh_layout_unlocked();
    }

    pthread_mutex_unlock(&screen_mutex);
}

void display_screen_data() {
    int cursor_y = 0;
    int cursor_x = 0;
    int max_x = 0;
    int max_y = 0;

    pthread_mutex_lock(&screen_mutex);

    getmaxyx(stdscr, max_y, max_x);
    getyx(stdscr, cursor_y, cursor_x);

    if (has_colors()) {
        attron(COLOR_PAIR(1));
    }
    mvprintw(0, 0, "Initial Screen Height = %d, Initial Screen Width = %d", max_y, max_x);
    if (has_colors()) {
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));
    }
    mvprintw(1, 0, "Current Y = %d, Current X = %d", cursor_y, cursor_x);
    if (has_colors()) {
        attroff(COLOR_PAIR(2));
    }

    wnoutrefresh(stdscr);
    doupdate();
    pthread_mutex_unlock(&screen_mutex);
}

void display_help() {
    const int prompt_row = std::max(4, console_window.height - 3);

    clear_window(console_window);
    write_window(console_window, 0, 0, "...Help...");
    write_window(console_window, 1, 0, "1/2/3 = Kill");
    write_window(console_window, 2, 0, "c = Clear");
    write_window(console_window, 3, 0, "h = Help");
    write_window(console_window, 4, 0, "q = Quit");
    write_window(console_window, prompt_row, 0, "Ultima # ");
}

void initialize_heading() {
    const char* title = "ULTIMA 2.0 (Lab 7 pthread Mutex)";
    const int body_width = heading_window.width - 2;
    const int title_x = std::max(0, (body_width - static_cast<int>(std::strlen(title))) / 2);

    write_window(heading_window, 0, title_x, title);
    write_window(heading_window, 1, 1, "Starting ULTIMA 2.0...");
    write_window(heading_window, 2, 1, "Press 'q' or Ctrl-C to exit the program...");
}

void request_thread_shutdown(ThreadData& thread_data) {
    if (!thread_data.kill_signal.exchange(true)) {
        char message[128];
        std::snprintf(message, sizeof(message), "Kill signal delivered to Thread %d.\n", thread_data.thread_no);
        write_window(log_window, message);
    }
}

void request_all_shutdown(ThreadData& thread_one, ThreadData& thread_two, ThreadData& thread_three) {
    request_thread_shutdown(thread_one);
    request_thread_shutdown(thread_two);
    request_thread_shutdown(thread_three);
}

void* perform_simple_output(void* arguments) {
    ThreadData* thread_data = static_cast<ThreadData*>(arguments);
    int cpu_quantum = 0;
    char message[256];

    thread_data->thread_state = ThreadState::running;

    while (!thread_data->kill_signal.load()) {
        std::snprintf(message, sizeof(message), "Task-%d running #%d\n", thread_data->thread_no, cpu_quantum);
        write_window(*thread_data->thread_window, message);

        std::snprintf(message, sizeof(message), "Thread %d -> quantum %d\n", thread_data->thread_no, cpu_quantum);
        write_window(*thread_data->log_window, message);

        ++cpu_quantum;
        sleep(thread_data->sleep_time);
    }

    thread_data->thread_state = ThreadState::terminated;
    std::snprintf(message, sizeof(message), "Thread %d shutting down.\n", thread_data->thread_no);
    write_window(*thread_data->log_window, message);
    write_window(*thread_data->thread_window, "TERMINATED\n");

    return nullptr;
}

} // namespace

int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_RED, -1);
        init_pair(2, COLOR_GREEN, -1);
    }

    if (!configure_layout()) {
        endwin();
        std::cerr << "Resize the terminal to at least "
                  << kMinimumTerminalCols
                  << " columns by "
                  << kMinimumTerminalRows
                  << " rows and rerun ultima_os."
                  << std::endl;
        return 1;
    }

    display_screen_data();

    if (!create_window(heading_window, layout.header_height, layout.header_width, layout.header_y, layout.header_x)
        || !create_window(thread_window_1, layout.worker_height, layout.worker_width_1, layout.worker_y, layout.worker_x_1)
        || !create_window(thread_window_2, layout.worker_height, layout.worker_width_2, layout.worker_y, layout.worker_x_2)
        || !create_window(thread_window_3, layout.worker_height, layout.worker_width_3, layout.worker_y, layout.worker_x_3)
        || !create_window(log_window, layout.log_height, layout.log_width, layout.log_y, layout.log_x)
        || !create_window(console_window, layout.console_height, layout.console_width, layout.console_y, layout.console_x)) {
        endwin();
        std::cerr << "Failed to initialize the ncurses window layout." << std::endl;
        return 1;
    }

    initialize_heading();
    write_window(log_window, "Log Window online.\n");
    display_help();

    ThreadData thread_data_1;
    thread_data_1.thread_no = 1;
    thread_data_1.thread_state = ThreadState::ready;
    thread_data_1.thread_window = &thread_window_1;
    thread_data_1.log_window = &log_window;
    thread_data_1.sleep_time = 1;

    ThreadData thread_data_2;
    thread_data_2.thread_no = 2;
    thread_data_2.thread_state = ThreadState::ready;
    thread_data_2.thread_window = &thread_window_2;
    thread_data_2.log_window = &log_window;
    thread_data_2.sleep_time = 1;

    ThreadData thread_data_3;
    thread_data_3.thread_no = 3;
    thread_data_3.thread_state = ThreadState::ready;
    thread_data_3.thread_window = &thread_window_3;
    thread_data_3.log_window = &log_window;
    thread_data_3.sleep_time = 1;

    write_window(thread_window_1, "Starting Thread 1...\n");
    write_window(thread_window_2, "Starting Thread 2...\n");
    write_window(thread_window_3, "Starting Thread 3...\n");

    pthread_t thread_1;
    pthread_t thread_2;
    pthread_t thread_3;

    int result_code = pthread_create(&thread_1, nullptr, perform_simple_output, &thread_data_1);
    assert(result_code == 0);
    result_code = pthread_create(&thread_2, nullptr, perform_simple_output, &thread_data_2);
    assert(result_code == 0);
    result_code = pthread_create(&thread_3, nullptr, perform_simple_output, &thread_data_3);
    assert(result_code == 0);

    keypad(console_window.frame, TRUE);
    nodelay(console_window.frame, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);

    MEVENT event;
    int input = ERR;
    char message[256];

    while (input != 'q') {
        input = wgetch(console_window.frame);

        switch (input) {
            case '1':
                request_thread_shutdown(thread_data_1);
                break;
            case '2':
                request_thread_shutdown(thread_data_2);
                break;
            case '3':
                request_thread_shutdown(thread_data_3);
                break;
            case 'c':
                clear_window(console_window);
                write_window(console_window, std::max(4, console_window.height - 3), 0, "Ultima # ");
                break;
            case 'h':
                display_help();
                break;
            case 'q':
                request_all_shutdown(thread_data_1, thread_data_2, thread_data_3);
                break;
            case KEY_MOUSE:
                if (getmouse(&event) == OK) {
                    std::snprintf(
                        message,
                        sizeof(message),
                        "Mouse click at row=%d, col=%d\n",
                        event.y,
                        event.x
                    );
                    write_window(log_window, message);
                }
                break;
            case ERR:
                break;
            default:
                break;
        }

        usleep(100000);
    }

    pthread_join(thread_1, nullptr);
    pthread_join(thread_2, nullptr);
    pthread_join(thread_3, nullptr);

    write_window(log_window, "All worker threads joined.\n");

    destroy_window(console_window);
    destroy_window(log_window);
    destroy_window(thread_window_3);
    destroy_window(thread_window_2);
    destroy_window(thread_window_1);
    destroy_window(heading_window);

    endwin();
    return 0;
}
