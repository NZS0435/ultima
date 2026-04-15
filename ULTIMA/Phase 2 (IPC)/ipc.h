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
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct MailboxSecurityProfile {
    bool security_enabled = false;
    bool restricted_access_enabled = false;
    bool encryption_enabled = false;
    std::string shared_secret;
    std::unordered_set<int> authorized_senders;
    std::unordered_set<int> authorized_receivers;
};

class ipc {
private:
    Scheduler* scheduler_ref;
    int max_active_tasks;
    std::unordered_map<int, MailboxSecurityProfile> mailbox_security_profiles;

    bool valid_task(int task_id) const;
    static const char* type_id_to_string(int type_id);
    static std::string make_payload_preview(const Message& message);
    static std::uint64_t compute_stream_seed(const MailboxSecurityProfile& profile,
                                             const Message& message);
    static std::uint64_t next_stream_value(std::uint64_t& state);
    static std::string encrypt_to_hex(const MailboxSecurityProfile& profile, const Message& message);
    static bool decrypt_from_hex(const MailboxSecurityProfile& profile, Message* message);
    static int compute_security_tag(const MailboxSecurityProfile& profile, const Message& message);
    MailboxSecurityProfile* get_security_profile_mut(int task_id);
    const MailboxSecurityProfile* get_security_profile(int task_id) const;
    bool sender_authorized(int sender_id, int destination_task_id) const;
    bool receiver_authorized(int requester_id, int mailbox_task_id) const;
    void apply_mailbox_security(Message* message);
    bool deliver_mailbox_message(int requester_id, int mailbox_task_id, Message* message);

public:
    ipc();
    explicit ipc(int max_tasks, Scheduler* sched = nullptr);

    int init(int max_tasks, Scheduler* sched);

    int Message_Send(Message* msg);
    int Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type);
    int Message_Send(int S_Id, int D_Id, Message* msg);

    int Message_Receive(int Task_Id, Message* msg);
    int Message_Receive(int Task_Id, char* Mess, int* Mess_Type);
    int Message_Receive(int Task_Id, char* Mess, std::size_t Mess_Buffer_Size, int* Mess_Type);

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

    int Configure_Mailbox_Security(int Task_Id,
                                   const char* shared_secret,
                                   bool restricted_access = true,
                                   bool encrypt_payload = true);
    int Disable_Mailbox_Security(int Task_Id);
    int Allow_Mailbox_Sender(int Task_Id, int Sender_Id);
    int Revoke_Mailbox_Sender(int Task_Id, int Sender_Id);
    int Allow_Mailbox_Reader(int Task_Id, int Reader_Id);
    int Revoke_Mailbox_Reader(int Task_Id, int Reader_Id);
    int Secure_Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type);
    int Secure_Message_Receive(int Requester_Id, int Task_Id, Message* msg);
    int Secure_Message_Receive(int Requester_Id, int Task_Id, char* Mess, std::size_t Mess_Buffer_Size, int* Mess_Type);
    std::string get_security_summary(int Task_Id) const;
    std::string get_security_overview() const;

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
