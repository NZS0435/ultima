#include "U2_Window.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

std::vector<std::string> split_preserving_blank_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current_line;

    for (char ch : text) {
        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            lines.push_back(current_line);
            current_line.clear();
            continue;
        }

        current_line.push_back(ch);
    }

    if (!current_line.empty() || (!text.empty() && text.back() == '\n')) {
        lines.push_back(current_line);
    }

    if (lines.empty()) {
        lines.push_back("");
    }

    return lines;
}

std::vector<std::string> wrap_line_to_width(const std::string& line, int width) {
    if (width <= 0) {
        return {""};
    }

    if (line.empty()) {
        return {""};
    }

    std::vector<std::string> wrapped_lines;
    std::size_t cursor = 0;

    while (cursor < line.size()) {
        while (cursor < line.size() && line[cursor] == ' ') {
            ++cursor;
        }

        if (cursor >= line.size()) {
            break;
        }

        const std::size_t remaining = line.size() - cursor;
        if (remaining <= static_cast<std::size_t>(width)) {
            wrapped_lines.push_back(line.substr(cursor));
            break;
        }

        const std::size_t search_limit = cursor + static_cast<std::size_t>(width);
        std::size_t break_at = line.rfind(' ', search_limit);
        if (break_at == std::string::npos || break_at < cursor) {
            break_at = search_limit;
        }

        std::string segment = line.substr(cursor, break_at - cursor);
        while (!segment.empty() && segment.back() == ' ') {
            segment.pop_back();
        }

        wrapped_lines.push_back(segment);
        cursor = break_at;
    }

    if (wrapped_lines.empty()) {
        wrapped_lines.push_back("");
    }

    return wrapped_lines;
}

std::vector<std::string> wrap_block_to_width(const std::string& text, int width) {
    std::vector<std::string> wrapped_lines;

    for (const std::string& line : split_preserving_blank_lines(text)) {
        const std::vector<std::string> line_parts = wrap_line_to_width(line, width);
        wrapped_lines.insert(wrapped_lines.end(), line_parts.begin(), line_parts.end());
    }

    if (wrapped_lines.empty()) {
        wrapped_lines.push_back("");
    }

    return wrapped_lines;
}

} // namespace

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
        idlok(win, TRUE);
        wsetscrreg(win, 1, std::max(1, h - 2));
        wmove(win, 1, 1);
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

    const int max_cols = inner_width();
    int cursor_y = getcury(win);
    if (cursor_y < 1 || cursor_y > h - 2) {
        cursor_y = 1;
    }

    const std::vector<std::string> wrapped_lines = wrap_block_to_width(text != nullptr ? text : "", max_cols);
    for (const std::string& line : wrapped_lines) {
        if (cursor_y > h - 2) {
            wscrl(win, 1);
            cursor_y = h - 2;
        }

        mvwhline(win, cursor_y, 1, ' ', max_cols);
        mvwaddnstr(win, cursor_y, 1, line.c_str(), max_cols);
        ++cursor_y;
    }

    box(win, 0, 0); // Restore borders in case text overwrote them
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wmove(win, std::min(cursor_y, std::max(1, h - 2)), 1);
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

void U2_window::draw_lines(const std::vector<std::string>& lines) {
    pthread_mutex_lock(&screen_mutex);

    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());

    const int max_rows = inner_height();
    const int max_cols = inner_width();
    std::vector<std::string> wrapped_lines;

    for (const std::string& line : lines) {
        const std::vector<std::string> line_parts = wrap_line_to_width(line, max_cols);
        wrapped_lines.insert(wrapped_lines.end(), line_parts.begin(), line_parts.end());
    }

    for (int row = 0; row < max_rows && row < static_cast<int>(wrapped_lines.size()); ++row) {
        mvwhline(win, row + 1, 1, ' ', max_cols);
        mvwaddnstr(win, row + 1, 1, wrapped_lines[static_cast<std::size_t>(row)].c_str(), max_cols);
    }

    wrefresh(win);
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
    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " %s ", window_title.c_str());
    wrefresh(win);
    pthread_mutex_unlock(&screen_mutex);
}

int U2_window::inner_height() const {
    return (h > 2) ? (h - 2) : 0;
}

int U2_window::inner_width() const {
    return (w > 2) ? (w - 2) : 0;
}
