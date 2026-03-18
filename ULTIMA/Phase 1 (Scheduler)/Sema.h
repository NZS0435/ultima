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

#ifndef SEMA_H
#define SEMA_H

#include <queue>
#include <iostream>
#include <pthread.h>
#include <ncurses.h>

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: NICHOLAS KOBS
 */


/**
 * Class semaphore
 * Designed by: Nicholas Kobs
 * Manages access to shared resources without busy-waiting.
 */
class Semaphore {
private:
    char resource_name[64];
    int sema_value;
    std::queue<int>* sema_queue;

public:
    Semaphore(const char* name, int initial_value = 1);
    ~Semaphore();

    void down();
    void up();
    void dump(int level = 0) const;
    bool has_waiters() const;
    int waiting_task_count() const;
};

#endif // SEMA_H
