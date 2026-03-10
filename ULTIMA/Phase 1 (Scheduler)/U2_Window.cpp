#include "U2_Window.h"

// Initialize the global mutual exclusion semaphore for thread-safe UI rendering
pthread_mutex_t U2_window::screen_mutex = PTHREAD_MUTEX_INITIALIZER;

U2_window::U2_window(int height, int width, int starty, int startx, const std::string& title, bool scroll) {
    h = height;
    w = width;
    start_y = starty;
    start_x = startx;
    window_title = title;
    scroll_enabled = scroll;

    // Protect initialization with mutex
    pthread_mutex_lock(&screen_mutex);

    win = newwin(h, w, start_y, start_x);
    if (scroll_enabled) {
        scrollok(win, TRUE);
    }

    box(win, 0, 0);
    // Print title nicely at the top of the box border
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wrefresh(win);

    pthread_mutex_unlock(&screen_mutex);
}

U2_window::~U2_window() {
    pthread_mutex_lock(&screen_mutex);
    delwin(win);
    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::render() {
    pthread_mutex_lock(&screen_mutex);
    wrefresh(win);
    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::write_text(const char* text) {
    // Critical Section
    pthread_mutex_lock(&screen_mutex);

    wprintw(win, "%s", text);
    box(win, 0, 0); // Restore borders in case text overwrote them
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wrefresh(win);

    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::write_text_at(int y, int x, const char* text) {
    // Critical Section
    pthread_mutex_lock(&screen_mutex);

    mvwprintw(win, y, x, "%s", text);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wrefresh(win);

    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::box_window() {
    pthread_mutex_lock(&screen_mutex);
    box(win, 0, 0);
    wrefresh(win);
    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::clear_window() {
    pthread_mutex_lock(&screen_mutex);
    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wrefresh(win);
    pthread_mutex_unlock(&screen_mutex);
}
