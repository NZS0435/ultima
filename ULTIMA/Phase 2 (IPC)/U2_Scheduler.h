#ifndef U2_SCHEDULER_H
#define U2_SCHEDULER_H

#include <queue>
#include <string>

#include "Message.h"

// Forward declaration required for Phase 2 mailbox_semaphore
class Semaphore;

enum State {
    RUNNING,
    READY,
    BLOCKED,
    DEAD
};

typedef void (*TaskEntryPoint)();

/**
 * Task Control Block
 * Updated by Stewart Pawley for Phase 2 IPC Integration.
 */
struct TCB {
    int task_id;
    std::string task_name;
    TaskEntryPoint task_entry;
    State task_state;

    int dispatch_count = 0;
    int yield_count = 0;
    int block_count = 0;
    int unblock_count = 0;

    std::string detail_note;
    std::string last_transition;

    // --- PHASE 2 IPC ADDITIONS ---
    // The mailbox is deeply linked to the TCB as recommended in the PDF.
    std::queue<Message> mailbox;
    Semaphore* mailbox_semaphore = nullptr;

    TCB(int id, const std::string& name, TaskEntryPoint entry) :
        task_id(id),
        task_name(name),
        task_entry(entry),
        task_state(READY)
    {
    }
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

#endif // U2_SCHEDULER_H
