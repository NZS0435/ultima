/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#include "U2_Window.h"
#include <algorithm>

// Initialize the global mutual exclusion semaphore for thread-safe UI rendering
/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: ZANDER HAYES
 */
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
    box(win, 0, 0);
    // Print title nicely at the top of the box border
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());

    text_win = derwin(win, h - 2, w - 2, 1, 1);
    if (scroll_enabled) {
        scrollok(text_win, TRUE);
    }

    wrefresh(win);

    pthread_mutex_unlock(&screen_mutex);
}

U2_window::~U2_window() {
    pthread_mutex_lock(&screen_mutex);
    delwin(text_win);
    delwin(win);
    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::render() {
    pthread_mutex_lock(&screen_mutex);
    wnoutrefresh(win);
    wnoutrefresh(text_win);
    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::write_text(const char* text) {
    // Critical Section
    pthread_mutex_lock(&screen_mutex);

    wprintw(text_win, "%s", text);
    wrefresh(text_win);

    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::write_text_at(int y, int x, const char* text) {
    // Critical Section
    pthread_mutex_lock(&screen_mutex);

    mvwprintw(text_win, y, x, "%s", text);
    wrefresh(text_win);

    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::draw_lines(const std::vector<std::string>& lines) {
    pthread_mutex_lock(&screen_mutex);

    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    werase(text_win);

    const int max_lines = std::max(0, h - 2);
    const int max_width = std::max(0, w - 3);
    const int visible_lines = std::min(max_lines, static_cast<int>(lines.size()));

    for (int index = 0; index < visible_lines; ++index) {
        std::string visible_text = lines[static_cast<std::size_t>(index)];
        if (static_cast<int>(visible_text.size()) > max_width) {
            if (max_width > 3) {
                visible_text = visible_text.substr(0, static_cast<std::size_t>(max_width - 3)) + "...";
            } else {
                visible_text = visible_text.substr(0, static_cast<std::size_t>(max_width));
            }
        }

        mvwprintw(text_win, index, 0, "%s", visible_text.c_str());
    }

    wnoutrefresh(win);
    wnoutrefresh(text_win);

    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::box_window() {
    pthread_mutex_lock(&screen_mutex);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wrefresh(win);
    pthread_mutex_unlock(&screen_mutex);
}

void U2_window::clear_window() {
    pthread_mutex_lock(&screen_mutex);
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    werase(text_win);
    wnoutrefresh(win);
    wnoutrefresh(text_win);
    pthread_mutex_unlock(&screen_mutex);
}
