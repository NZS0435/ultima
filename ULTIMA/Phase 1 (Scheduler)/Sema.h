#ifndef SEMA_H
#define SEMA_H

#include <deque>
#include <string>

#include "U2_Scheduler.h"

class Semaphore {
private:
    std::string resource_name;
    int initial_value;
    int value;
    Scheduler* scheduler;
    std::deque<int> wait_queue;
    int owner_task_id;
    int down_operations;
    int up_operations;
    int contention_events;
    std::string last_transition;

public:
    Semaphore(const std::string& resource, int init_val, Scheduler* sched = nullptr);

    void reset();
    void down();
    void up();
    void P();
    void V();
    void dump(int level = 0) const;

    int get_sema_value() const;
    int get_owner_task_id() const;
    std::string get_resource_name() const;
    int waiting_task_count() const;
    std::string describe_wait_queue() const;
    int get_down_operations() const;
    int get_up_operations() const;
    int get_contention_events() const;
    std::string get_last_transition() const;
};

#endif // SEMA_H
