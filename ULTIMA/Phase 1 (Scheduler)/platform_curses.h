/* ![Team Thunder Banner](</Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg>) */

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

#ifndef ULTIMA_PLATFORM_CURSES_H
#define ULTIMA_PLATFORM_CURSES_H

#include <pthread.h>
#include <ncurses.h>

#if defined(__has_include)
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#else
#error "No curses header found. Install ncurses (macOS/Linux) or PDCurses (Windows)."
#endif
#else
#include <curses.h>
#endif

#endif // ULTIMA_PLATFORM_CURSES_H
