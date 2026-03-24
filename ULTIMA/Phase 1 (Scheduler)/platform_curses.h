/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#ifndef ULTIMA_PLATFORM_CURSES_H
#define ULTIMA_PLATFORM_CURSES_H

#if defined(__has_include)
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#else
/*
 * Fallback declarations for editor tooling environments where curses headers
 * are not discoverable. Real builds should still link against curses.
 */
typedef struct WINDOW WINDOW;
typedef struct {
    int id;
    int x;
    int y;
    int z;
    unsigned long bstate;
} MEVENT;

extern WINDOW* stdscr;
extern int LINES;
extern int COLS;

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

WINDOW* initscr(void);
int endwin(void);
int cbreak(void);
int noecho(void);
int curs_set(int visibility);
int start_color(void);
int init_pair(short pair, short f, short b);
int refresh(void);
int doupdate(void);
int wrefresh(WINDOW* win);
int wnoutrefresh(WINDOW* win);
int scrollok(WINDOW* win, int bf);
WINDOW* newwin(int nlines, int ncols, int begin_y, int begin_x);
WINDOW* derwin(WINDOW* orig, int nlines, int ncols, int begin_y, int begin_x);
int box(WINDOW* win, int verch, int horch);
int delwin(WINDOW* win);
int wprintw(WINDOW* win, const char* fmt, ...);
int mvwprintw(WINDOW* win, int y, int x, const char* fmt, ...);
int wclear(WINDOW* win);
int werase(WINDOW* win);
int wgetch(WINDOW* win);
int keypad(WINDOW* win, int bf);
int nodelay(WINDOW* win, int bf);
unsigned long mousemask(unsigned long newmask, unsigned long* oldmask);
void mouseinterval(int erval);
void napms(int ms);
int getmouse(MEVENT* event);
#endif
#else
#include <curses.h>
#endif

#endif // ULTIMA_PLATFORM_CURSES_H
