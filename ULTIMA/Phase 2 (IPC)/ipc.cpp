/* =========================================================================
 * ipc.cpp — Phase 2 IPC implementation
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label    : Phase 2 - Message Passing (IPC)
 * Primary Author : Zander Hayes
 * Co-Authors     : Stewart Pawley (semaphore integration and UI strings)
 *                  Nicholas Kobs  (utility workflow alignment)
 * =========================================================================
 */

#include "ipc.h"

#include "../Phase 1 (Scheduler)/Sema.h"

#include <cstring>

namespace {
std::string format_arrival_time(std::time_t arrival_time) {
    char time_buffer[32] = {0};
    const std::tm* local_time = std::localtime(&arrival_time);
    if (local_time != nullptr) {
        std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", local_time);
    } else {
        std::strncpy(time_buffer, "N/A", sizeof(time_buffer) - 1);
    }

    return time_buffer;
}
} // namespace

const char* ipc::type_id_to_string(int type_id) {
    switch (type_id) {
        case 0:
            return "Text";
        case 1:
            return "Service";
        case 2:
            return "Notification";
        default:
            return "Unknown";
    }
}

bool ipc::valid_task(int task_id) const {
    if (scheduler_ref == nullptr || task_id < 0) {
        return false;
    }

    return scheduler_ref->get_tcb(task_id) != nullptr;
}

ipc::ipc() :
    scheduler_ref(nullptr),
    max_active_tasks(0) {
}

ipc::ipc(int max_tasks, Scheduler* sched) :
    ipc() {
    if (sched != nullptr && max_tasks > 0) {
        init(max_tasks, sched);
    }
}

int ipc::init(int max_tasks, Scheduler* sched) {
    if (max_tasks <= 0 || sched == nullptr) {
        return -1;
    }

    max_active_tasks = max_tasks;
    scheduler_ref = sched;
    return 1;
}

int ipc::Message_Send(Message* msg) {
    if (msg == nullptr || !valid_task(msg->Destination_Task_Id)) {
        return -1;
    }

    TCB* destination_task = scheduler_ref->get_tcb(msg->Destination_Task_Id);
    if (destination_task == nullptr) {
        return -1;
    }

    msg->Message_Arrival_Time = std::time(nullptr);
    msg->Msg_Text[sizeof(msg->Msg_Text) - 1] = '\0';
    msg->Msg_Type.Message_Type_Description[sizeof(msg->Msg_Type.Message_Type_Description) - 1] = '\0';
    msg->Msg_Size = static_cast<int>(std::strlen(msg->Msg_Text));

    mailbox_down(msg->Destination_Task_Id);
    destination_task->mailbox.push(*msg);
    mailbox_up(msg->Destination_Task_Id);

    std::ostringstream event_stream;
    event_stream << "MSG SENT: T-" << msg->Source_Task_Id
                 << " -> T-" << msg->Destination_Task_Id
                 << " [" << msg->Msg_Type.Message_Type_Description << "] "
                 << '"' << msg->Msg_Text << '"';
    EventLog::instance().add(event_stream.str());

    return 1;
}

int ipc::Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type) {
    if (Mess == nullptr || !valid_task(D_Id)) {
        return -1;
    }

    Message new_message {};
    new_message.Source_Task_Id = S_Id;
    new_message.Destination_Task_Id = D_Id;

    std::strncpy(new_message.Msg_Text, Mess, sizeof(new_message.Msg_Text) - 1);
    new_message.Msg_Text[sizeof(new_message.Msg_Text) - 1] = '\0';
    new_message.Msg_Size = static_cast<int>(std::strlen(new_message.Msg_Text));

    new_message.Msg_Type.Message_Type_Id = Mess_Type;
    std::strncpy(new_message.Msg_Type.Message_Type_Description,
                 type_id_to_string(Mess_Type),
                 sizeof(new_message.Msg_Type.Message_Type_Description) - 1);
    new_message.Msg_Type.Message_Type_Description[sizeof(new_message.Msg_Type.Message_Type_Description) - 1] = '\0';

    return Message_Send(&new_message);
}

int ipc::Message_Send(int S_Id, int D_Id, Message* msg) {
    if (msg == nullptr) {
        return -1;
    }

    msg->Source_Task_Id = S_Id;
    msg->Destination_Task_Id = D_Id;
    return Message_Send(msg);
}

int ipc::Message_Receive(int Task_Id, Message* msg) {
    if (msg == nullptr || !valid_task(Task_Id)) {
        return -1;
    }

    TCB* task = scheduler_ref->get_tcb(Task_Id);
    if (task == nullptr) {
        return -1;
    }

    if (task->mailbox.empty()) {
        return 0;
    }

    mailbox_down(Task_Id);

    if (task->mailbox.empty()) {
        mailbox_up(Task_Id);
        return 0;
    }

    *msg = task->mailbox.front();
    task->mailbox.pop();

    mailbox_up(Task_Id);

    std::ostringstream event_stream;
    event_stream << "MSG RECV: T-" << Task_Id
                 << " got msg from T-" << msg->Source_Task_Id
                 << " [" << msg->Msg_Type.Message_Type_Description << "] "
                 << '"' << msg->Msg_Text << '"';
    EventLog::instance().add(event_stream.str());

    return 1;
}

int ipc::Message_Receive(int Task_Id, char* Mess, int* Mess_Type) {
    if (Mess == nullptr || Mess_Type == nullptr) {
        return -1;
    }

    Message received_message {};
    const int result = Message_Receive(Task_Id, &received_message);
    if (result == 1) {
        std::strncpy(Mess, received_message.Msg_Text, sizeof(received_message.Msg_Text) - 1);
        Mess[sizeof(received_message.Msg_Text) - 1] = '\0';
        *Mess_Type = received_message.Msg_Type.Message_Type_Id;
    }

    return result;
}

int ipc::Message_Count(int Task_id) const {
    if (!valid_task(Task_id)) {
        return -1;
    }

    const TCB* task = scheduler_ref->get_tcb(Task_id);
    return static_cast<int>(task->mailbox.size());
}

int ipc::Message_Count() const {
    if (scheduler_ref == nullptr) {
        return -1;
    }

    int total = 0;
    for (int task_id = 0; task_id < max_active_tasks; ++task_id) {
        const TCB* task = scheduler_ref->get_tcb(task_id);
        if (task != nullptr) {
            total += static_cast<int>(task->mailbox.size());
        }
    }

    return total;
}

void ipc::Message_Print(int Task_id) const {
    std::cout << Message_Print_String(Task_id);
}

int ipc::Message_DeleteAll(int Task_id) {
    if (!valid_task(Task_id)) {
        return -1;
    }

    TCB* task = scheduler_ref->get_tcb(Task_id);
    if (task == nullptr) {
        return -1;
    }

    mailbox_down(Task_id);
    const int count = static_cast<int>(task->mailbox.size());
    std::queue<Message> empty_queue;
    std::swap(task->mailbox, empty_queue);
    mailbox_up(Task_id);

    return count;
}

void ipc::ipc_Message_Dump() const {
    std::cout << ipc_Message_Dump_String();
}

std::string ipc::Message_Print_String(int Task_id) const {
    return get_mailbox_table(Task_id);
}

std::string ipc::ipc_Message_Dump_String() const {
    std::ostringstream out;

    out << "=== IPC MESSAGE DUMP (All Mailboxes) ===\n";
    out << "Total messages: " << Message_Count() << "\n\n";

    bool wrote_any_mailbox = false;
    for (int task_id = 0; task_id < max_active_tasks; ++task_id) {
        const TCB* task = (scheduler_ref != nullptr) ? scheduler_ref->get_tcb(task_id) : nullptr;
        if (task == nullptr || task->mailbox.empty()) {
            continue;
        }

        out << get_mailbox_table(task_id) << "\n";
        wrote_any_mailbox = true;
    }

    if (!wrote_any_mailbox) {
        out << "  All mailboxes empty.\n";
    }

    return out.str();
}

std::string ipc::get_mailbox_table(int Task_id) const {
    if (!valid_task(Task_id)) {
        return "(invalid task)\n";
    }

    std::queue<Message> mailbox_copy = get_mailbox_copy(Task_id);

    std::ostringstream out;
    out << "Task Number: " << Task_id << "\n";
    out << "Message Count: " << mailbox_copy.size() << "\n";
    out << "Mail Box:\n";

    if (mailbox_copy.empty()) {
        out << "  (empty)\n";
        return out.str();
    }

    out << std::left
        << std::setw(10) << "Source"
        << std::setw(12) << "Destination"
        << std::setw(36) << "Message Content"
        << std::setw(8) << "Size"
        << std::setw(15) << "Type"
        << "Arrival Time\n";
    out << std::string(92, '-') << "\n";

    while (!mailbox_copy.empty()) {
        const Message& message = mailbox_copy.front();
        out << std::left
            << std::setw(10) << message.Source_Task_Id
            << std::setw(12) << message.Destination_Task_Id
            << std::setw(36) << message.Msg_Text
            << std::setw(8) << message.Msg_Size
            << std::setw(15) << message.Msg_Type.Message_Type_Description
            << format_arrival_time(message.Message_Arrival_Time)
            << '\n';
        mailbox_copy.pop();
    }

    return out.str();
}

void ipc::mailbox_down(int task_id) {
    if (!valid_task(task_id)) {
        return;
    }

    TCB* task = scheduler_ref->get_tcb(task_id);
    if (task != nullptr && task->mailbox_semaphore != nullptr) {
        task->mailbox_semaphore->down();
    }

    std::ostringstream event_stream;
    event_stream << "[Semaphore] down() on mailbox " << task_id << " - critical section ENTER";
    EventLog::instance().add(event_stream.str());
}

void ipc::mailbox_up(int task_id) {
    if (!valid_task(task_id)) {
        return;
    }

    TCB* task = scheduler_ref->get_tcb(task_id);
    if (task != nullptr && task->mailbox_semaphore != nullptr) {
        task->mailbox_semaphore->up();
    }

    std::ostringstream event_stream;
    event_stream << "[Semaphore] up()   on mailbox " << task_id << " - critical section LEAVE";
    EventLog::instance().add(event_stream.str());
}

int ipc::get_max_tasks() const {
    return max_active_tasks;
}

std::queue<Message> ipc::get_mailbox_copy(int task_id) const {
    if (!valid_task(task_id)) {
        return {};
    }

    const TCB* task = scheduler_ref->get_tcb(task_id);
    return (task != nullptr) ? task->mailbox : std::queue<Message> {};
}
