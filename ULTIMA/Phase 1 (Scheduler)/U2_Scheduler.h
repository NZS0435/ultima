#ifndef U2_SCHEDULER_H
#define U2_SCHEDULER_H

#include <cstddef>
#include <string>
#include <vector>
#include <pthread.h>
#include <ncurses.h>

/**
 * ULTIMA 2.0 - Phase 1
 * Designed by: STEWART PAWLEY
 */


enum State {
    RUNNING,
    READY,
    BLOCKED,
    DEAD
};

using TaskEntryPoint = void (*)();

struct TCB {
    int task_id;
    std::string task_name;
    State task_state;
    TaskEntryPoint task_entry;
    void* context_pointer;

    TCB(int id, const std::string& name, TaskEntryPoint entry_point) :
        task_id(id),
        task_name(name),
        task_state(READY),
        task_entry(entry_point),
        context_pointer(nullptr) {}
};

class Scheduler {
private:
    std::vector<TCB*> process_table;
    int current_running_task;
    int next_task_id;
    bool dispatch_in_progress;

    int find_task_index(int task_id) const;
    int find_next_ready_index() const;
    const char* state_to_string(State state) const;

public:
    Scheduler();
    ~Scheduler();

    int create_task(const char* task_name, TaskEntryPoint func_ptr = nullptr);
    void kill_task(int task_id);
    void yield();
    void garbage_collect();
    void block_task(int task_id);
    void unblock_task(int task_id);
    void dump(int level = 0) const;

    int get_current_task_id() const;
    int get_active_task_count() const;
    bool has_active_tasks() const;
    bool is_task_blocked(int task_id) const;
    void block_current_task();
};

#endif // U2_SCHEDULER_H
