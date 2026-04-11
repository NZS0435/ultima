/* =========================================================================
 * phase2_smoke_test.cpp — Phase 2 non-interactive build validation
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label    : Phase 2 - Message Passing (IPC)
 * Primary Author : Stewart Pawley
 * Co-Authors     : Zander Hayes
 *                  Nicholas Kobs
 * =========================================================================
 */

#include "MCB.h"

#include <cassert>
#include <cstring>
#include <iostream>

MCB sys_mcb;

namespace {
void idle_task() {
}
} // namespace

int main() {
    const int sender_id = sys_mcb.Swapper.create_task("Sender_Task", idle_task);
    const int receiver_id = sys_mcb.Swapper.create_task("Receiver_Task", idle_task);

    TCB* receiver_tcb = sys_mcb.Swapper.get_tcb(receiver_id);
    assert(receiver_tcb != nullptr);
    assert(receiver_tcb->mailbox_semaphore != nullptr);

    const int send_down_before = receiver_tcb->mailbox_semaphore->get_down_operations();
    const int send_up_before = receiver_tcb->mailbox_semaphore->get_up_operations();

    const int send_result = sys_mcb.Messenger.Message_Send(sender_id, receiver_id, "phase2", 0);
    assert(send_result == 1);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 1);
    assert(sys_mcb.Messenger.Message_Count() == 1);
    assert(receiver_tcb->mailbox_semaphore->get_down_operations() == send_down_before + 1);
    assert(receiver_tcb->mailbox_semaphore->get_up_operations() == send_up_before + 1);

    Message received_message {};
    const int receive_down_before = receiver_tcb->mailbox_semaphore->get_down_operations();
    const int receive_up_before = receiver_tcb->mailbox_semaphore->get_up_operations();

    const int receive_result = sys_mcb.Messenger.Message_Receive(receiver_id, &received_message);
    assert(receive_result == 1);
    assert(std::strcmp(received_message.Msg_Text, "phase2") == 0);
    assert(received_message.Msg_Type.Message_Type_Id == 0);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 0);
    assert(sys_mcb.Messenger.Message_Count() == 0);
    assert(receiver_tcb->mailbox_semaphore->get_down_operations() == receive_down_before + 1);
    assert(receiver_tcb->mailbox_semaphore->get_up_operations() == receive_up_before + 1);

    assert(sys_mcb.Messenger.Message_Send(sender_id, 9999, "bad", 0) == -1);
    assert(sys_mcb.Messenger.Message_Receive(9999, &received_message) == -1);

    char buffer[33] = {0};
    int message_type = -1;
    assert(sys_mcb.Messenger.Message_Receive(receiver_id, buffer, &message_type) == 0);

    assert(sys_mcb.Messenger.Message_Send(sender_id, receiver_id, "delete-1", 1) == 1);
    assert(sys_mcb.Messenger.Message_Send(sender_id, receiver_id, "delete-2", 2) == 1);
    assert(sys_mcb.Messenger.Message_DeleteAll(receiver_id) == 2);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 0);

    const std::string dump = sys_mcb.Messenger.ipc_Message_Dump_String();
    assert(dump.find("All mailboxes empty.") != std::string::npos);

    std::cout << "Phase 2 smoke test passed.\n";
    return 0;
}
