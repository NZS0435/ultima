#ifndef IPC_H
#define IPC_H

#include "Message.h"
#include "Sched.h"

/**
 * Author: Zander Hayes (Core Data Structures & Base Logic)
 * Co-Authors: Stewart Pawley (Semaphore & MCB Integration), Nicholas Kobs (Utilities)
 * Phase 2 IPC Messaging System
 */
class ipc {
private:
    Scheduler* scheduler_ref;
    int max_active_tasks;

public:
    // Zander: Constructor & Setup
    explicit ipc(int max_tasks, Scheduler* sched = nullptr);

    // Zander & Stewart: Sending Logic & Semaphore Down/Up Injections
    int Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type);
    int Message_Send(int S_Id, int D_Id, Message* msg);

    // Zander & Stewart: Receiving Logic & Semaphore Down/Up Injections
    int Message_Receive(int Task_Id, Message* msg);
    int Message_Receive(int Task_Id, char* Mess, int* Mess_Type);

    // Nick: IPC Utilities
    int Message_Count(int Task_id) const;
    int Message_Count() const;
    void Message_Print(int Task_id) const;
    int Message_DeleteAll(int Task_id);
    void ipc_Message_Dump() const;
};

#endif // IPC_H
