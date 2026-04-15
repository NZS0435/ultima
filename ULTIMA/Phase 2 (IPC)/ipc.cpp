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

#include <algorithm>
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

std::uint64_t fnv1a_mix(std::uint64_t seed, const std::string& value) {
    for (unsigned char ch : value) {
        seed ^= static_cast<std::uint64_t>(ch);
        seed *= 1099511628211ULL;
    }
    return seed;
}

char hex_nibble(unsigned int value) {
    return static_cast<char>((value < 10U) ? ('0' + value) : ('A' + (value - 10U)));
}

unsigned int parse_hex_nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<unsigned int>(value - '0');
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<unsigned int>(value - 'A' + 10);
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<unsigned int>(value - 'a' + 10);
    }
    return 0U;
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

std::string ipc::make_payload_preview(const Message& message) {
    if (message.Is_Encrypted) {
        std::string preview(message.Msg_Cipher_Text);
        if (preview.size() > 28) {
            preview = preview.substr(0, 28) + "...";
        }
        return "ENC:" + preview;
    }

    return std::string(message.Msg_Text);
}

std::uint64_t ipc::compute_stream_seed(const MailboxSecurityProfile& profile,
                                       const Message& message) {
    std::uint64_t seed = 1469598103934665603ULL;
    seed = fnv1a_mix(seed, profile.shared_secret);
    seed = fnv1a_mix(seed, std::to_string(message.Source_Task_Id));
    seed = fnv1a_mix(seed, std::to_string(message.Destination_Task_Id));
    seed = fnv1a_mix(seed, std::to_string(message.Msg_Type.Message_Type_Id));
    seed = fnv1a_mix(seed, std::to_string(message.Msg_Size));
    return seed;
}

std::uint64_t ipc::next_stream_value(std::uint64_t& state) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    state *= 2685821657736338717ULL;
    return state;
}

std::string ipc::encrypt_to_hex(const MailboxSecurityProfile& profile, const Message& message) {
    std::string cipher_hex;
    cipher_hex.reserve(static_cast<std::size_t>(message.Msg_Size) * 2);

    std::uint64_t stream_state = compute_stream_seed(profile, message);
    std::uint64_t stream_value = 0;

    for (int index = 0; index < message.Msg_Size; ++index) {
        if (index % 8 == 0) {
            stream_value = next_stream_value(stream_state);
        }

        const unsigned char plain = static_cast<unsigned char>(message.Msg_Text[index]);
        const unsigned char key_byte =
            static_cast<unsigned char>((stream_value >> ((index % 8) * 8)) & 0xFFU);
        const unsigned char cipher = static_cast<unsigned char>(plain ^ key_byte);

        cipher_hex.push_back(hex_nibble((cipher >> 4) & 0x0FU));
        cipher_hex.push_back(hex_nibble(cipher & 0x0FU));
    }

    return cipher_hex;
}

bool ipc::decrypt_from_hex(const MailboxSecurityProfile& profile, Message* message) {
    if (message == nullptr || !message->Is_Encrypted) {
        return true;
    }

    const std::size_t cipher_length = std::strlen(message->Msg_Cipher_Text);
    if ((cipher_length % 2U) != 0U) {
        return false;
    }

    const std::size_t expected_cipher_length = static_cast<std::size_t>(message->Msg_Size) * 2U;
    if (cipher_length != expected_cipher_length || static_cast<std::size_t>(message->Msg_Size) >= ULTIMA_MESSAGE_TEXT_CAPACITY) {
        return false;
    }

    const int expected_tag = compute_security_tag(profile, *message);
    if (expected_tag != message->Security_Tag) {
        return false;
    }

    std::uint64_t stream_state = compute_stream_seed(profile, *message);
    std::uint64_t stream_value = 0;

    for (int index = 0; index < message->Msg_Size; ++index) {
        if (index % 8 == 0) {
            stream_value = next_stream_value(stream_state);
        }

        const unsigned int high =
            parse_hex_nibble(message->Msg_Cipher_Text[static_cast<std::size_t>(index) * 2U]);
        const unsigned int low =
            parse_hex_nibble(message->Msg_Cipher_Text[static_cast<std::size_t>(index) * 2U + 1U]);
        const unsigned char cipher = static_cast<unsigned char>((high << 4U) | low);
        const unsigned char key_byte =
            static_cast<unsigned char>((stream_value >> ((index % 8) * 8)) & 0xFFU);

        message->Msg_Text[index] = static_cast<char>(cipher ^ key_byte);
    }

    message->Msg_Text[message->Msg_Size] = '\0';
    return true;
}

int ipc::compute_security_tag(const MailboxSecurityProfile& profile, const Message& message) {
    std::uint64_t tag_seed = 1469598103934665603ULL;
    tag_seed = fnv1a_mix(tag_seed, profile.shared_secret);
    tag_seed = fnv1a_mix(tag_seed, message.Msg_Cipher_Text);
    tag_seed = fnv1a_mix(tag_seed, std::to_string(message.Source_Task_Id));
    tag_seed = fnv1a_mix(tag_seed, std::to_string(message.Destination_Task_Id));
    tag_seed = fnv1a_mix(tag_seed, std::to_string(message.Msg_Type.Message_Type_Id));
    tag_seed = fnv1a_mix(tag_seed, std::to_string(message.Msg_Size));
    return static_cast<int>(tag_seed & 0x7FFFFFFF);
}

MailboxSecurityProfile* ipc::get_security_profile_mut(int task_id) {
    auto it = mailbox_security_profiles.find(task_id);
    return (it != mailbox_security_profiles.end()) ? &it->second : nullptr;
}

const MailboxSecurityProfile* ipc::get_security_profile(int task_id) const {
    auto it = mailbox_security_profiles.find(task_id);
    return (it != mailbox_security_profiles.end()) ? &it->second : nullptr;
}

bool ipc::sender_authorized(int sender_id, int destination_task_id) const {
    const MailboxSecurityProfile* profile = get_security_profile(destination_task_id);
    if (profile == nullptr || !profile->security_enabled || !profile->restricted_access_enabled) {
        return true;
    }

    return profile->authorized_senders.find(sender_id) != profile->authorized_senders.end();
}

bool ipc::receiver_authorized(int requester_id, int mailbox_task_id) const {
    const MailboxSecurityProfile* profile = get_security_profile(mailbox_task_id);
    if (requester_id == mailbox_task_id) {
        return true;
    }
    if (profile == nullptr || !profile->security_enabled || !profile->restricted_access_enabled) {
        return requester_id == mailbox_task_id;
    }

    return profile->authorized_receivers.find(requester_id) != profile->authorized_receivers.end();
}

void ipc::apply_mailbox_security(Message* message) {
    if (message == nullptr) {
        return;
    }

    message->Is_Encrypted = false;
    message->Access_Restricted = false;
    message->Security_Tag = 0;
    message->Msg_Cipher_Text[0] = '\0';

    MailboxSecurityProfile* profile = get_security_profile_mut(message->Destination_Task_Id);
    if (profile == nullptr || !profile->security_enabled) {
        return;
    }

    message->Access_Restricted = profile->restricted_access_enabled;
    if (!profile->encryption_enabled) {
        return;
    }

    const std::string cipher_hex = encrypt_to_hex(*profile, *message);
    std::strncpy(message->Msg_Cipher_Text, cipher_hex.c_str(), sizeof(message->Msg_Cipher_Text) - 1);
    message->Msg_Cipher_Text[sizeof(message->Msg_Cipher_Text) - 1] = '\0';
    message->Security_Tag = compute_security_tag(*profile, *message);
    message->Is_Encrypted = true;
    message->Msg_Text[0] = '\0';
}

bool ipc::deliver_mailbox_message(int requester_id, int mailbox_task_id, Message* message) {
    if (message == nullptr || !valid_task(mailbox_task_id)) {
        return false;
    }

    const MailboxSecurityProfile* profile = get_security_profile(mailbox_task_id);
    if (message->Is_Encrypted) {
        if (profile == nullptr || !profile->security_enabled || !profile->encryption_enabled) {
            return false;
        }
        if (!receiver_authorized(requester_id, mailbox_task_id)) {
            return false;
        }
        return decrypt_from_hex(*profile, message);
    }

    return receiver_authorized(requester_id, mailbox_task_id);
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
    mailbox_security_profiles.clear();
    return 1;
}

int ipc::Message_Send(Message* msg) {
    if (msg == nullptr || !valid_task(msg->Destination_Task_Id) || msg->Source_Task_Id < 0) {
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

    if (msg->Msg_Size >= static_cast<int>(ULTIMA_MESSAGE_TEXT_CAPACITY)) {
        return -1;
    }

    if (!sender_authorized(msg->Source_Task_Id, msg->Destination_Task_Id)) {
        std::ostringstream deny_stream;
        deny_stream << "ACCESS DENIED: T-" << msg->Source_Task_Id
                    << " cannot send to secure mailbox T-" << msg->Destination_Task_Id;
        EventLog::instance().add(deny_stream.str());
        return -1;
    }

    apply_mailbox_security(msg);

    mailbox_down(msg->Destination_Task_Id);
    destination_task->mailbox.push(*msg);
    mailbox_up(msg->Destination_Task_Id);

    std::ostringstream event_stream;
    if (msg->Is_Encrypted) {
        event_stream << "MSG SENT SECURE: T-" << msg->Source_Task_Id
                     << " -> T-" << msg->Destination_Task_Id
                     << " [" << msg->Msg_Type.Message_Type_Description << "] "
                     << make_payload_preview(*msg);
    } else {
        event_stream << "MSG SENT: T-" << msg->Source_Task_Id
                     << " -> T-" << msg->Destination_Task_Id
                     << " [" << msg->Msg_Type.Message_Type_Description << "] "
                     << '"' << msg->Msg_Text << '"';
    }
    EventLog::instance().add(event_stream.str());

    return 1;
}

int ipc::Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type) {
    if (Mess == nullptr || !valid_task(D_Id) || S_Id < 0) {
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
    return Secure_Message_Receive(Task_Id, Task_Id, msg);
}

int ipc::Message_Receive(int Task_Id, char* Mess, int* Mess_Type) {
    return Message_Receive(Task_Id, Mess, ULTIMA_MESSAGE_TEXT_CAPACITY, Mess_Type);
}

int ipc::Message_Receive(int Task_Id, char* Mess, std::size_t Mess_Buffer_Size, int* Mess_Type) {
    return Secure_Message_Receive(Task_Id, Task_Id, Mess, Mess_Buffer_Size, Mess_Type);
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
    for (int task_id = 0; task_id <= max_active_tasks; ++task_id) {
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
    for (int task_id = 0; task_id <= max_active_tasks; ++task_id) {
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
    out << "Security: " << get_security_summary(Task_id) << "\n";
    out << "Mail Box:\n";

    if (mailbox_copy.empty()) {
        out << "  (empty)\n";
        return out.str();
    }

    out << std::left
        << std::setw(8) << "Source"
        << std::setw(12) << "Dest"
        << std::setw(34) << "Payload"
        << std::setw(8) << "Size"
        << std::setw(14) << "Type"
        << std::setw(12) << "Security"
        << "Arrival Time\n";
    out << std::string(102, '-') << "\n";

    while (!mailbox_copy.empty()) {
        const Message& message = mailbox_copy.front();
        std::string security_label = message.Is_Encrypted ? "ENC+ACL" :
            (message.Access_Restricted ? "ACL" : "Plain");
        out << std::left
            << std::setw(8) << message.Source_Task_Id
            << std::setw(12) << message.Destination_Task_Id
            << std::setw(34) << make_payload_preview(message)
            << std::setw(8) << message.Msg_Size
            << std::setw(14) << message.Msg_Type.Message_Type_Description
            << std::setw(12) << security_label
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

int ipc::Configure_Mailbox_Security(int Task_Id,
                                    const char* shared_secret,
                                    bool restricted_access,
                                    bool encrypt_payload) {
    if (!valid_task(Task_Id) || shared_secret == nullptr || shared_secret[0] == '\0') {
        return -1;
    }

    MailboxSecurityProfile& profile = mailbox_security_profiles[Task_Id];
    profile.security_enabled = true;
    profile.restricted_access_enabled = restricted_access;
    profile.encryption_enabled = encrypt_payload;
    profile.shared_secret = shared_secret;
    profile.authorized_senders.clear();
    profile.authorized_receivers.clear();
    profile.authorized_receivers.insert(Task_Id);

    std::ostringstream event_stream;
    event_stream << "SECURITY CONFIG: mailbox T-" << Task_Id
                 << " restricted=" << (restricted_access ? "yes" : "no")
                 << " encrypted=" << (encrypt_payload ? "yes" : "no");
    EventLog::instance().add(event_stream.str());

    return 1;
}

int ipc::Disable_Mailbox_Security(int Task_Id) {
    if (!valid_task(Task_Id)) {
        return -1;
    }

    mailbox_security_profiles.erase(Task_Id);
    EventLog::instance().add("SECURITY CONFIG: mailbox T-" + std::to_string(Task_Id) + " disabled");
    return 1;
}

int ipc::Allow_Mailbox_Sender(int Task_Id, int Sender_Id) {
    if (!valid_task(Task_Id) || Sender_Id < 0 || (Sender_Id != 0 && !valid_task(Sender_Id))) {
        return -1;
    }

    MailboxSecurityProfile* profile = get_security_profile_mut(Task_Id);
    if (profile == nullptr || !profile->security_enabled) {
        return -1;
    }

    profile->authorized_senders.insert(Sender_Id);
    EventLog::instance().add("SECURITY ACL: mailbox T-" + std::to_string(Task_Id)
                             + " allows sender T-" + std::to_string(Sender_Id));
    return 1;
}

int ipc::Revoke_Mailbox_Sender(int Task_Id, int Sender_Id) {
    if (!valid_task(Task_Id) || Sender_Id < 0 || (Sender_Id != 0 && !valid_task(Sender_Id))) {
        return -1;
    }

    MailboxSecurityProfile* profile = get_security_profile_mut(Task_Id);
    if (profile == nullptr || !profile->security_enabled) {
        return -1;
    }

    profile->authorized_senders.erase(Sender_Id);
    EventLog::instance().add("SECURITY ACL: mailbox T-" + std::to_string(Task_Id)
                             + " revoked sender T-" + std::to_string(Sender_Id));
    return 1;
}

int ipc::Allow_Mailbox_Reader(int Task_Id, int Reader_Id) {
    if (!valid_task(Task_Id) || !valid_task(Reader_Id)) {
        return -1;
    }

    MailboxSecurityProfile* profile = get_security_profile_mut(Task_Id);
    if (profile == nullptr || !profile->security_enabled) {
        return -1;
    }

    profile->authorized_receivers.insert(Reader_Id);
    EventLog::instance().add("SECURITY ACL: mailbox T-" + std::to_string(Task_Id)
                             + " allows reader T-" + std::to_string(Reader_Id));
    return 1;
}

int ipc::Revoke_Mailbox_Reader(int Task_Id, int Reader_Id) {
    if (!valid_task(Task_Id) || !valid_task(Reader_Id)) {
        return -1;
    }

    MailboxSecurityProfile* profile = get_security_profile_mut(Task_Id);
    if (profile == nullptr || !profile->security_enabled || Reader_Id == Task_Id) {
        return -1;
    }

    profile->authorized_receivers.erase(Reader_Id);
    EventLog::instance().add("SECURITY ACL: mailbox T-" + std::to_string(Task_Id)
                             + " revoked reader T-" + std::to_string(Reader_Id));
    return 1;
}

int ipc::Secure_Message_Send(int S_Id, int D_Id, const char* Mess, int Mess_Type) {
    const MailboxSecurityProfile* profile = get_security_profile(D_Id);
    if (profile == nullptr || !profile->security_enabled || !profile->encryption_enabled) {
        return -1;
    }

    return Message_Send(S_Id, D_Id, Mess, Mess_Type);
}

int ipc::Secure_Message_Receive(int Requester_Id, int Task_Id, Message* msg) {
    if (msg == nullptr || !valid_task(Task_Id) || !valid_task(Requester_Id)) {
        return -1;
    }

    if (!receiver_authorized(Requester_Id, Task_Id)) {
        std::ostringstream deny_stream;
        deny_stream << "ACCESS DENIED: T-" << Requester_Id
                    << " cannot read secure mailbox T-" << Task_Id;
        EventLog::instance().add(deny_stream.str());
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

    if (!deliver_mailbox_message(Requester_Id, Task_Id, msg)) {
        std::ostringstream deny_stream;
        deny_stream << "SECURITY ERROR: mailbox T-" << Task_Id
                    << " rejected delivery to T-" << Requester_Id;
        EventLog::instance().add(deny_stream.str());
        return -1;
    }

    std::ostringstream event_stream;
    if (msg->Is_Encrypted) {
        event_stream << "MSG RECV SECURE: T-" << Requester_Id
                     << " got decrypted msg from T-" << msg->Source_Task_Id
                     << " [" << msg->Msg_Type.Message_Type_Description << "] "
                     << '"' << msg->Msg_Text << '"';
    } else {
        event_stream << "MSG RECV: T-" << Requester_Id
                     << " got msg from T-" << msg->Source_Task_Id
                     << " [" << msg->Msg_Type.Message_Type_Description << "] "
                     << '"' << msg->Msg_Text << '"';
    }
    EventLog::instance().add(event_stream.str());

    return 1;
}

int ipc::Secure_Message_Receive(int Requester_Id,
                                int Task_Id,
                                char* Mess,
                                std::size_t Mess_Buffer_Size,
                                int* Mess_Type) {
    if (Mess == nullptr || Mess_Type == nullptr || Mess_Buffer_Size == 0U) {
        return -1;
    }

    Message received_message {};
    const int result = Secure_Message_Receive(Requester_Id, Task_Id, &received_message);
    if (result == 1) {
        const std::size_t copy_length = std::min(
            Mess_Buffer_Size - 1U,
            std::strlen(received_message.Msg_Text));
        std::memcpy(Mess, received_message.Msg_Text, copy_length);
        Mess[copy_length] = '\0';
        *Mess_Type = received_message.Msg_Type.Message_Type_Id;
    }

    return result;
}

std::string ipc::get_security_summary(int Task_Id) const {
    if (!valid_task(Task_Id)) {
        return "invalid";
    }

    const MailboxSecurityProfile* profile = get_security_profile(Task_Id);
    if (profile == nullptr || !profile->security_enabled) {
        return "Open mailbox";
    }

    std::ostringstream out;
    out << (profile->encryption_enabled ? "Encrypted" : "Plain")
        << " / "
        << (profile->restricted_access_enabled ? "Restricted" : "Open ACL")
        << " / senders=" << profile->authorized_senders.size()
        << " / readers=" << profile->authorized_receivers.size();
    return out.str();
}

std::string ipc::get_security_overview() const {
    if (scheduler_ref == nullptr) {
        return "Security profiles unavailable.";
    }

    std::ostringstream out;
    out << "Security profiles:\n";
    bool wrote_profile = false;

    for (int task_id = 0; task_id <= max_active_tasks; ++task_id) {
        const TCB* task = scheduler_ref->get_tcb(task_id);
        if (task == nullptr) {
            continue;
        }

        out << "T-" << task_id << ": " << get_security_summary(task_id) << "\n";
        wrote_profile = true;
    }

    if (!wrote_profile) {
        out << "(no active tasks)\n";
    }

    return out.str();
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
