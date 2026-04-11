/* =========================================================================
 * ipc.h — Phase 2 IPC interface
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label    : Phase 2 - Message Passing (IPC)
 * Primary Author : Zander Hayes
 * Co-Authors     : Stewart Pawley (integration design)
 *                  Nicholas Kobs  (utility/report alignment)
 * =========================================================================
 */

#ifndef IPC_H
#define IPC_H

#include "Message.h"
#include "Sched.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

class ipc {
private:
    Scheduler* scheduler_ref;
    int max_active_tasks;

    bool valid_task(int task_id) const;
    static const char* type_id_to_string(int type_id);

public:
    ipc();
    explicit ipc(int max_tasks, Scheduler* sched = nullptr);

    int init(int max_tasks, Scheduler* sched);

    int Message_Send(Message* msg);
    int Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type);
    int Message_Send(int S_Id, int D_Id, Message* msg);

    int Message_Receive(int Task_Id, Message* msg);
    int Message_Receive(int Task_Id, char* Mess, int* Mess_Type);

    int Message_Count(int Task_id) const;
    int Message_Count() const;
    void Message_Print(int Task_id) const;
    int Message_DeleteAll(int Task_id);
    void ipc_Message_Dump() const;

    std::string Message_Print_String(int Task_id) const;
    std::string ipc_Message_Dump_String() const;
    std::string get_mailbox_table(int Task_id) const;

    void mailbox_down(int task_id);
    void mailbox_up(int task_id);

    int get_max_tasks() const;
    std::queue<Message> get_mailbox_copy(int task_id) const;
};

class EventLog {
public:
    static EventLog& instance() {
        static EventLog event_log;
        return event_log;
    }

    void add(const std::string& entry) {
        std::lock_guard<std::mutex> guard(mutex_);
        entries_.push_back(entry);
    }

    std::vector<std::string> get_all() const {
        std::lock_guard<std::mutex> guard(mutex_);
        return entries_;
    }

    void clear() {
        std::lock_guard<std::mutex> guard(mutex_);
        entries_.clear();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> guard(mutex_);
        return entries_.size();
    }

private:
    EventLog() = default;

    mutable std::mutex mutex_;
    std::vector<std::string> entries_;
};

#endif // IPC_H
