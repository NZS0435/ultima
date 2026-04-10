/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#ifndef ULTIMA_PLATFORM_CURSES_H
#define ULTIMA_PLATFORM_CURSES_H

#if defined(__clangd__) || defined(__INTELLISENSE__) || defined(__JETBRAINS_IDE__)
#define ULTIMA_EDITOR_CURSES_STUBS 1
#endif

#if defined(ULTIMA_ENABLE_CURSES) && !defined(ULTIMA_EDITOR_CURSES_STUBS)
#if defined(__has_include)
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#else
#error "ULTIMA_ENABLE_CURSES is set, but no curses header was found."
#endif
#else
#include <curses.h>
#endif
#else
/*
 * Transcript-only and editor-analysis fallback used for single-translation-unit
 * builds that do not link against curses, or for IDE parsing environments that
 * do not have the active toolchain's curses headers on the default include path.
 */
struct WINDOW {
    int height;
    int width;
    int begin_y;
    int begin_x;
    int cursor_y;
    int cursor_x;
    int scroll_enabled;
};

struct MEVENT {
    int id;
    int x;
    int y;
    int z;
    unsigned long bstate;
};

typedef unsigned long chtype;

#ifndef ERR
#define ERR (-1)
#endif
#ifndef OK
#define OK (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef KEY_MOUSE
#define KEY_MOUSE (0x1b5)
#endif
#ifndef ALL_MOUSE_EVENTS
#define ALL_MOUSE_EVENTS (0UL)
#endif
#ifndef REPORT_MOUSE_POSITION
#define REPORT_MOUSE_POSITION (0UL)
#endif
#ifndef COLOR_BLACK
#define COLOR_BLACK (0)
#endif
#ifndef COLOR_RED
#define COLOR_RED (1)
#endif
#ifndef COLOR_GREEN
#define COLOR_GREEN (2)
#endif

static WINDOW ultima_stub_stdscr_storage = {24, 80, 0, 0, 0, 0, FALSE};
static WINDOW* stdscr = &ultima_stub_stdscr_storage;
static int LINES = 24;
static int COLS = 80;

inline WINDOW* initscr(void) {
    return stdscr;
}

inline int endwin(void) {
    return OK;
}

inline int cbreak(void) {
    return OK;
}

inline int noecho(void) {
    return OK;
}

inline int curs_set(int) {
    return OK;
}

inline int start_color(void) {
    return OK;
}

inline int init_pair(short, short, short) {
    return OK;
}

inline int refresh(void) {
    return OK;
}

inline int doupdate(void) {
    return OK;
}

inline int wrefresh(WINDOW*) {
    return OK;
}

inline int wnoutrefresh(WINDOW*) {
    return OK;
}

inline int scrollok(WINDOW* win, int bf) {
    if (win != nullptr) {
        win->scroll_enabled = bf;
    }
    return OK;
}

inline int idlok(WINDOW*, int) {
    return OK;
}

inline WINDOW* newwin(int nlines, int ncols, int begin_y, int begin_x) {
    WINDOW* win = new WINDOW {nlines, ncols, begin_y, begin_x, 0, 0, FALSE};
    return win;
}

inline WINDOW* derwin(WINDOW*, int nlines, int ncols, int begin_y, int begin_x) {
    return newwin(nlines, ncols, begin_y, begin_x);
}

inline int box(WINDOW*, int, int) {
    return OK;
}

inline int delwin(WINDOW* win) {
    if (win != nullptr && win != stdscr) {
        delete win;
    }
    return OK;
}

inline int wprintw(WINDOW*, const char*, ...) {
    return OK;
}

inline int mvwprintw(WINDOW* win, int y, int x, const char*, ...) {
    if (win != nullptr) {
        win->cursor_y = y;
        win->cursor_x = x;
    }
    return OK;
}

inline int mvwhline(WINDOW* win, int y, int x, chtype, int) {
    if (win != nullptr) {
        win->cursor_y = y;
        win->cursor_x = x;
    }
    return OK;
}

inline int mvwaddnstr(WINDOW* win, int y, int x, const char*, int n) {
    if (win != nullptr) {
        win->cursor_y = y;
        win->cursor_x = x + ((n > 0) ? n : 0);
    }
    return OK;
}

inline int wclear(WINDOW*) {
    return OK;
}

inline int werase(WINDOW*) {
    return OK;
}

inline int wgetch(WINDOW*) {
    return ERR;
}

inline int keypad(WINDOW*, int) {
    return OK;
}

inline int nodelay(WINDOW*, int) {
    return OK;
}

inline int getcury(WINDOW* win) {
    return (win != nullptr) ? win->cursor_y : 0;
}

inline int wmove(WINDOW* win, int y, int x) {
    if (win != nullptr) {
        win->cursor_y = y;
        win->cursor_x = x;
    }
    return OK;
}

inline int wscrl(WINDOW* win, int n) {
    if (win != nullptr) {
        win->cursor_y -= n;
        if (win->cursor_y < 0) {
            win->cursor_y = 0;
        }
    }
    return OK;
}

inline int wsetscrreg(WINDOW*, int, int) {
    return OK;
}

inline unsigned long mousemask(unsigned long newmask, unsigned long* oldmask) {
    if (oldmask != nullptr) {
        *oldmask = newmask;
    }
    return newmask;
}

inline void mouseinterval(int) {}

inline void napms(int) {}

inline int getmouse(MEVENT*) {
    return ERR;
}
#endif

#endif // ULTIMA_PLATFORM_CURSES_H
