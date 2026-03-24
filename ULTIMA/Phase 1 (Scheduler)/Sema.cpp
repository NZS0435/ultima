#include "Sema.h"

Semaphore::Semaphore(int init_val, Scheduler* sched) : value(init_val), scheduler(sched) {}

void Semaphore::P() {
    value--;
    if (value < 0) {
        scheduler->block_current_task();
    }
}

void Semaphore::V() {
    value++;
    if (value <= 0) {
        // In a real implementation, we would need a queue of blocked tasks for this semaphore.
        // For this phase, we might just unblock the first blocked task found by the scheduler,
        // or rely on the scheduler to manage blocked queues.
        // Since the Scheduler::unblock_task takes a task ID, the semaphore needs to know WHICH task to wake up.
        // This implies the Semaphore needs its own wait queue.

        // Placeholder: The current Scheduler interface doesn't expose a way to get a specific blocked task
        // associated with *this* semaphore without a wait queue in the Semaphore class.
        // Assuming for now we just need the structure.
    }
}