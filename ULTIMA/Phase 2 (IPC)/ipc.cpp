#include "ipc.h"
#include "Sema.h"

#include <iostream>

// Zander Hayes - Constructor & Base Allocations
ipc::ipc(int max_tasks, Scheduler* sched) :
    scheduler_ref(sched),
    max_active_tasks(max_tasks)
{
    // --- ZANDER'S PART INSERT HERE ---
    // [Zander] Constructor logic to be implemented by Zander Hayes.
}

// Zander Hayes (Logic) & Stewart Pawley (Semaphore Management)
int ipc::Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type) {
    (void)S_Id;
    (void)Mess;
    (void)Mess_Type;

    if (scheduler_ref == nullptr) {
        return -1;
    }

    TCB* dest_task = scheduler_ref->get_tcb(D_Id);
    if (dest_task == nullptr) {
        return -1;
    }

    // [Stewart] Critical Section Entry: Down() on destination mailbox semaphore
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->down();
    }

    // --- ZANDER'S PART INSERT HERE ---
    // [Zander] Node allocation, formatting, and pushing to dest_task->mailbox goes here.
    // Stewart's semaphore boundary is in place, but send semantics remain pending.

    // [Stewart] Critical Section Exit: Up() on destination mailbox semaphore
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->up();
    }

    return 0;
}

int ipc::Message_Send(int S_Id, int D_Id, Message* msg) {
    (void)S_Id;

    if (scheduler_ref == nullptr || msg == nullptr) {
        return -1;
    }

    TCB* dest_task = scheduler_ref->get_tcb(D_Id);
    if (dest_task == nullptr) {
        return -1;
    }

    // [Stewart] Critical Section Entry: Down() on destination mailbox semaphore
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->down();
    }

    // --- ZANDER'S PART INSERT HERE ---
    // [Zander] Queue push logic goes here.
    // Stewart's semaphore boundary is in place, but send semantics remain pending.

    // [Stewart] Critical Section Exit: Up() on destination mailbox semaphore
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->up();
    }

    return 0;
}

int ipc::Message_Receive(int Task_Id, Message* msg) {
    if (scheduler_ref == nullptr || msg == nullptr) {
        return -1;
    }

    TCB* src_task = scheduler_ref->get_tcb(Task_Id);
    if (src_task == nullptr) {
        return -1;
    }

    // [Stewart] Critical Section Entry: Down() on source mailbox semaphore
    if (src_task->mailbox_semaphore != nullptr) {
        src_task->mailbox_semaphore->down();
    }

    // --- ZANDER'S PART INSERT HERE ---
    // [Zander] Dequeue logic goes here.
    // IMPORTANT: If mailbox is empty, ensure you call src_task->mailbox_semaphore->up() before returning 0!

    // [Stewart] Critical Section Exit: Up() on source mailbox semaphore
    if (src_task->mailbox_semaphore != nullptr) {
        src_task->mailbox_semaphore->up();
    }

    return 0;
}

int ipc::Message_Receive(int Task_Id, char* Mess, int* Mess_Type) {
    (void)Task_Id;
    (void)Mess;
    (void)Mess_Type;

    // --- ZANDER'S PART INSERT HERE ---
    // [Zander] Wrapper logic for receiving message goes here.
    return -1;
}

// =========================================================================
// Nick Kobs - Utilities & Testing Framework
// =========================================================================

int ipc::Message_Count(int Task_id) const {
    (void)Task_id;

    // --- NICK'S PART INSERT HERE ---
    // [Nick] Implement Message_Count here.
    return -1;
}

int ipc::Message_Count() const {
    // --- NICK'S PART INSERT HERE ---
    // [Nick] Implement global Message_Count here.
    return -1;
}

void ipc::Message_Print(int Task_id) const {
    (void)Task_id;

    // --- NICK'S PART INSERT HERE ---
    // [Nick] Implement Message_Print here.
}

int ipc::Message_DeleteAll(int Task_id) {
    (void)Task_id;

    // --- NICK'S PART INSERT HERE ---
    // [Nick] Implement Message_DeleteAll here.
    return -1;
}

void ipc::ipc_Message_Dump() const {
    // --- NICK'S PART INSERT HERE ---
    // [Nick] Implement ipc_Message_Dump here.
}
