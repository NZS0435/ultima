/* Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#ifndef U2_SCHEDULER_H
#define U2_SCHEDULER_H

#include <cstddef>
#include <string>
#include <vector>

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
    int dispatch_count;
    int yield_count;
    int block_count;
    int unblock_count;
    std::string detail_note;
    std::string last_transition;

    TCB(int id, const std::string& name, TaskEntryPoint entry_point) :
        task_id(id),
        task_name(name),
        task_state(READY),
        task_entry(entry_point),
        context_pointer(nullptr),
        dispatch_count(0),
        yield_count(0),
        block_count(0),
        unblock_count(0),
        detail_note("Created and waiting for first dispatch."),
        last_transition("Entered READY state.") {}
};

struct TaskSnapshot {
    int task_id;
    std::string task_name;
    State task_state;
    int dispatch_count;
    int yield_count;
    int block_count;
    int unblock_count;
    std::string detail_note;
    std::string last_transition;
};

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
    int create_task(const char* task_name, TaskEntryPoint func_ptr = nullptr);
    void kill_task(int task_id);
    void yield();
    void garbage_collect();
    void block_task(int task_id);
    void unblock_task(int task_id);
    void dump(int level = 0) const;

    int get_current_task_id() const;
    int get_active_task_count() const;
    int get_ready_task_count() const;
    int get_blocked_task_count() const;
    int get_dead_task_count() const;
    bool has_active_tasks() const;
    bool is_task_blocked(int task_id) const;
    void block_current_task();
    State get_task_state(int task_id) const;
    const char* get_task_state_name(int task_id) const;
    std::string get_task_name(int task_id) const;
    void set_task_note(int task_id, const std::string& note);
    std::vector<TaskSnapshot> snapshot_tasks() const;
    std::string describe_run_queue() const;
    int get_scheduler_tick() const;
    std::string get_last_scheduler_event() const;
};

#endif // U2_SCHEDULER_H
