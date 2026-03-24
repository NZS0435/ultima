/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#ifndef SEMA_H
#define SEMA_H

#include <iostream>
#include <queue>
#include <string>

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
    int initial_sema_value;
    int sema_value;
    int owner_task_id;
    int down_operations;
    int up_operations;
    int contention_events;
    std::string last_transition;
    std::queue<int>* sema_queue;

public:
    Semaphore(const char* name, int initial_value = 1);
    ~Semaphore();

    void reset();
    void down();
    void up();
    void dump(int level = 0) const;
    bool has_waiters() const;
    int waiting_task_count() const;
    int get_owner_task_id() const;
    int get_sema_value() const;
    int get_down_operations() const;
    int get_up_operations() const;
    int get_contention_events() const;
    std::string get_last_transition() const;
    std::string describe_wait_queue() const;
    std::string get_resource_name() const;
};

#endif // SEMA_H
