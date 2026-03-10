#ifndef U2_SCHEDULER_H
#define U2_SCHEDULER_H

#include <string>
#include <list>

// State enumeration based on the 4 required states
enum ProcessState {
    RUNNING,
    READY,
    BLOCKED,
    DEAD
};

// Task Control Block (TCB)
// Maintains the STATE of each task as requested by the assignment.
struct TCB {
    int task_id;
    std::string task_name;
    ProcessState state;

    // Placeholders for future phases (Phase 2+ Memory/File descriptors)
    void* mem_ptr;
    int priority;

    // Constructor for easy initialization
    TCB(int id, const std::string& name) :
        task_id(id), task_name(name), state(READY), mem_ptr(nullptr), priority(0) {}
};

class Scheduler {
private:
    // Dynamic thread table implemented as a linked list (scheduling ring)
    std::list<TCB*> process_table;

    // Iterator to keep track of the currently executing task for round-robin
    std::list<TCB*>::iterator current_task_it;

    // Internal counter to assign unique Task IDs
    int next_task_id;

public:
    Scheduler();
    ~Scheduler();

    // Core requirements from the prompt
    int create_task(const char* task_name);
    void kill_task(int t_id);
    void yield();
    void garbage_collect();
    void dump(int level);

    // Hooks required for Nicholas to build the Semaphore logic without busy waiting
    int get_current_task_id();
    void block_current_task();
    void unblock_task(int t_id);
};

#endif // U2_SCHEDULER_H
