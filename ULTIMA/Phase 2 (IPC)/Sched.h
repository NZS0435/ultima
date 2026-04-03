#ifndef SCHED_H
#define SCHED_H

#include <vector>

#include "U2_Scheduler.h"

/**
 * Author: Stewart Pawley
 * Phase 1 legacy scheduler adapted for Phase 2 IPC.
 */
class Scheduler {
private:
    std::vector<TCB*> process_table;
    int current_running_task;
    int next_task_id;
    bool dispatch_in_progress;
    int scheduler_tick;
    std::string last_scheduler_event;

    int find_task_index(int task_id) const;
    int find_next_ready_index() const;
    static const char* state_to_string(State state);

public:
    Scheduler();
    ~Scheduler();

    void reset();
    int create_task(const char* task_name, TaskEntryPoint func_ptr);
    void kill_task(int task_id);
    void yield();
    void garbage_collect();

    void block_task(int task_id);
    void unblock_task(int task_id);
    void block_current_task();

    void dump(int level = 0) const;

    int get_current_task_id() const;
    int get_active_task_count() const;
    int get_ready_task_count() const;
    int get_blocked_task_count() const;
    int get_dead_task_count() const;
    bool has_active_tasks() const;
    bool is_task_blocked(int task_id) const;

    State get_task_state(int task_id) const;
    const char* get_task_state_name(int task_id) const;
    std::string get_task_name(int task_id) const;
    void set_task_note(int task_id, const std::string& note);

    std::vector<TaskSnapshot> snapshot_tasks() const;
    std::string describe_run_queue() const;
    int get_scheduler_tick() const;
    std::string get_last_scheduler_event() const;

    // --- PHASE 2 EXPOSURE ---
    // Required to allow IPC object to route messages directly to TCBs
    TCB* get_tcb(int task_id) const;
};

#endif // SCHED_H
