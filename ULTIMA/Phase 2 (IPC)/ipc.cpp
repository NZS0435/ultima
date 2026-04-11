#include "ipc.h"
#include "Sema.h"
#include <cstring>
#include <ctime>
#include <iostream>

ipc::ipc(int max_tasks, Scheduler* sched)
    : scheduler_ref(sched),
      max_active_tasks(max_tasks)
{
    // Nothing to allocate here: each TCB owns its own
    // std::queue<Message> mailbox and Semaphore* mailbox_semaphore.
    // Both are set up (and torn down) by Scheduler::create_task()
    // and Scheduler::reset() / garbage_collect() respectively.
    // The ipc object only holds a reference to the scheduler so
    // it can reach those TCBs at send/receive time.
}

// ---------------------------------------------------------------
// Message_Send  (char* overload — primary, string-based entry)
// Returns:  1 on success,  -1 on error
// ---------------------------------------------------------------
int ipc::Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type)
{
    if (scheduler_ref == nullptr || Mess == nullptr) {
        return -1;
    }

    TCB* dest_task = scheduler_ref->get_tcb(D_Id);
    if (dest_task == nullptr) {
        return -1;
    }

    // Build the Message value on the stack before entering the
    // critical section — keeps the lock window as small as possible.
    Message msg;
    msg.Source_Task_Id      = S_Id;
    msg.Destination_Task_Id = D_Id;
    msg.Message_Arrival_Time = std::time(nullptr);
    msg.Msg_Type.Message_Type_Id = Mess_Type;

    // Populate the human-readable type description.
    switch (Mess_Type) {
        case 0:
            std::strncpy(msg.Msg_Type.Message_Type_Description,
                         "Text", 63);
            break;
        case 1:
            std::strncpy(msg.Msg_Type.Message_Type_Description,
                         "Service", 63);
            break;
        case 2:
            std::strncpy(msg.Msg_Type.Message_Type_Description,
                         "Notification", 63);
            break;
        default:
            std::strncpy(msg.Msg_Type.Message_Type_Description,
                         "Unknown", 63);
            break;
    }
    msg.Msg_Type.Message_Type_Description[63] = '\0';

    // Copy text — truncate cleanly to the 32-byte buffer.
    std::strncpy(msg.Msg_Text, Mess, 31);
    msg.Msg_Text[31] = '\0';
    msg.Msg_Size = static_cast<int>(std::strlen(msg.Msg_Text));

    // [Stewart] Critical Section Entry: Down() on destination mailbox
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->down();
    }

    dest_task->mailbox.push(msg); 

    // [Stewart] Critical Section Exit: Up() on destination mailbox
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->up();
    }

    return 1;
}

// ---------------------------------------------------------------
// Message_Send  (Message* overload — caller pre-fills the struct)
// Returns:  1 on success,  -1 on error
// ---------------------------------------------------------------
int ipc::Message_Send(int S_Id, int D_Id, Message* msg)
{
    if (scheduler_ref == nullptr || msg == nullptr) {
        return -1;
    }

    TCB* dest_task = scheduler_ref->get_tcb(D_Id);
    if (dest_task == nullptr) {
        return -1;
    }

    msg->Source_Task_Id      = S_Id;
    msg->Destination_Task_Id = D_Id;
    msg->Message_Arrival_Time = std::time(nullptr);

    msg->Msg_Text[31] = '\0';
    msg->Msg_Type.Message_Type_Description[63] = '\0';
    msg->Msg_Size = static_cast<int>(std::strlen(msg->Msg_Text));

    // [Stewart] Critical Section Entry
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->down();
    }

    dest_task->mailbox.push(*msg);

    // [Stewart] Critical Section Exit
    if (dest_task->mailbox_semaphore != nullptr) {
        dest_task->mailbox_semaphore->up();
    }

    return 1;
}

// ---------------------------------------------------------------
// Message_Receive  (Message* overload — primary)
// Returns:  1 = message loaded,  0 = mailbox empty,  -1 = error
// ---------------------------------------------------------------
int ipc::Message_Receive(int Task_Id, Message* msg)
{
    if (scheduler_ref == nullptr || msg == nullptr) {
        return -1;
    }

    TCB* src_task = scheduler_ref->get_tcb(Task_Id);
    if (src_task == nullptr) {
        return -1;
    }

    // Check count BEFORE taking the lock — early-out avoids
    // unnecessary semaphore traffic on an empty mailbox.
    if (src_task->mailbox.empty()) {
        return 0;
    }

    // [Stewart] Critical Section Entry
    if (src_task->mailbox_semaphore != nullptr) {
        src_task->mailbox_semaphore->down();
    }

    // Re-check inside the lock: another task could have drained
    // the queue between our check above and acquiring the semaphore.
    if (src_task->mailbox.empty()) {
        if (src_task->mailbox_semaphore != nullptr) {
            src_task->mailbox_semaphore->up();
        }
        return 0;
    }

    *msg = src_task->mailbox.front();
    src_task->mailbox.pop();

    // [Stewart] Critical Section Exit
    if (src_task->mailbox_semaphore != nullptr) {
        src_task->mailbox_semaphore->up();
    }

    return 1;
}

// ---------------------------------------------------------------
// Message_Receive  (char* / int* overload — convenience wrapper)
// Returns:  1 = message loaded,  0 = empty,  -1 = error
// ---------------------------------------------------------------
int ipc::Message_Receive(int Task_Id, char* Mess, int* Mess_Type)
{
    if (Mess == nullptr || Mess_Type == nullptr) {
        return -1;
    }

    Message tmp;
    int result = Message_Receive(Task_Id, &tmp);

    if (result == 1) {
        // Caller receives a copy of the text and the type id.
        // We assume Mess points to a buffer of at least 32 bytes
        // (same cap as Msg_Text).
        std::strncpy(Mess, tmp.Msg_Text, 31);
        Mess[31]   = '\0';
        *Mess_Type = tmp.Msg_Type.Message_Type_Id;
    }

    return result;
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
