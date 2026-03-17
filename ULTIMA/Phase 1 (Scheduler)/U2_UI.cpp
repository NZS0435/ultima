#include "U2_UI.h"
#include "platform_curses.h"
#include <clocale>
#include <pthread.h>
#include <ncurses.h>

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: NICHOLAS KOBS
 */


U2_ui::U2_ui() {
    // Left empty, environment initialization separated to init_ncurses_env()
}

U2_ui::~U2_ui() {
    // Garbage collection for dynamically allocated windows
    for (auto* win : window_list) {
        delete win;
    }
    window_list.clear();
}

void U2_ui::init_ncurses_env() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();

    // Set up standard colors matching Lab 4 parameters
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    refresh();
}

void U2_ui::close_ncurses_env() {
    endwin();
}

void U2_ui::add_window(U2_window* new_window) {
    if (new_window != nullptr) {
        window_list.push_back(new_window);
    }
}

void U2_ui::delete_window(U2_window* target_window) {
    auto it = window_list.begin();
    while (it != window_list.end()) {
        if (*it == target_window) {
            delete *it;
            it = window_list.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

void U2_ui::refresh_all() {
    for (auto* win : window_list) {
        win->render();
    }
    // Final update to physical screen
    doupdate();
}

void U2_ui::clear_all() {
    for (auto* win : window_list) {
        win->clear_window();
    }
}
