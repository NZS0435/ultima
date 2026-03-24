/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#ifndef U2_UI_H
#define U2_UI_H

#include "U2_Window.h"
#include <list>

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
