#ifndef U2_WINDOW_H
#define U2_WINDOW_H

#include "platform_curses.h"
#include <string>
#include "platform_threads.h"

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: ZANDER HAYES
 */


class U2_window {
private:
    WINDOW *win;
    std::string window_title;
    int h, w, start_y, start_x;
    bool scroll_enabled;

    // Static mutex shared across all window instances to prevent the
    // ncurses race conditions and screen corruption highlighted in Lab 7.
    static pthread_mutex_t screen_mutex;

public:
    // Constructor
    U2_window(int height, int width, int starty, int startx, const std::string& title, bool scroll);

    // Destructor
    ~U2_window();

    // Core functionality wrapping the ncurses library
    void render();
    void write_text(const char* text);
    void write_text_at(int y, int x, const char* text);
    void box_window();
    void clear_window();

    // Accessor
    WINDOW* get_win_ptr() const { return win; }
};

#endif // U2_WINDOW_H
