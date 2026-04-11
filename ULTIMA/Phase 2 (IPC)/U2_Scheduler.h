/* =========================================================================
 * U2_Scheduler.h — Phase 2 scheduler and TCB declarations
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Primary Author : Stewart Pawley
 * Co-Authors     : Zander Hayes   (IPC mailbox shape)
 *                  Nicholas Kobs  (demo/report alignment)
 * =========================================================================
 */

#ifndef U2_SCHEDULER_H
#define U2_SCHEDULER_H

#include <cstddef>
#include <queue>
#include <string>
#include <vector>

#include "Message.h"

// Forward declaration required for Phase 2 mailbox_semaphore
class Semaphore;

enum State {
    RUNNING,
    READY,
    BLOCKED,
    DEAD
};

using TaskEntryPoint = void (*)();

/**
 * Task Control Block
 * Updated by Stewart Pawley for Phase 2 IPC Integration.
 */
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

    // --- PHASE 2 IPC ADDITIONS ---
    // The mailbox is deeply linked to the TCB as recommended in the PDF.
    std::queue<Message> mailbox;
    Semaphore* mailbox_semaphore;

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
        last_transition("Entered READY state."),
        mailbox(),
        mailbox_semaphore(nullptr) {}
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
    std::string dump_string(int level = 0) const;

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

    // --- PHASE 2 EXPOSURE ---
    // Required to allow IPC object to route messages directly to TCBs.
    TCB* get_tcb(int task_id) const;
};

#endif // U2_SCHEDULER_H
