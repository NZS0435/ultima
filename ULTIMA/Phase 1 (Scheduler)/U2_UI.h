/*
 * PHASE 1 - ULTIMA 2.0 - TEAM THUNDER
 *
 *                 .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *             __.'             ~.   .~             `.__
 *           .'//                 \./                 \\`.
 *         .'//   PHASE 1 CLOUD    |   CODE RAIN       \\`.
 *       .'//______________________|_____________________\\`.
 *              || 01 01 01 01 01 01 01 01 01 ||
 *              || 10 10 10 10 10 10 10 10 10 ||
 *              || 01 01 01 01 01 01 01 01 01 ||
 *
 * Phase Label: Scheduler and Semaphore
 */

#ifndef U2_UI_H
#define U2_UI_H

#include "U2_Window.h"
#include <list>
#include <pthread.h>
#include <ncurses.h>

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: NICHOLAS KOBS
 */

class U2_ui {
private:
    // Dynamic linked list to hold pointers to our UI windows
    std::list<U2_window*> window_list;

public:
    U2_ui();
    ~U2_ui();

    // Environment management
    void init_ncurses_env();
    void close_ncurses_env();

    // Window management functions
    void add_window(U2_window* new_window);
    void delete_window(U2_window* target_window);

    // Global refresh and clear
    void refresh_all();
    void clear_all();
};

#endif // U2_UI_H
