/* =========================================================================
 * Message.h — Phase 2 IPC message structures
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label    : Phase 2 - Message Passing (IPC)
 * Primary Author : Zander Hayes
 * Co-Authors     : Stewart Pawley (integration)
 *                  Nicholas Kobs  (utilities alignment)
 * =========================================================================
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <ctime>

/**
 * Definitions matching the Phase II rubric requirements.
 */
struct Message_Type {
    int Message_Type_Id;
    // 0 = Text (No action required)
    // 1 = Service (Request for a service, send notification/result back)
    // 2 = Notification (Indicates service completed, no action required)
    char Message_Type_Description[64];
};

struct Message {
    int Source_Task_Id;
    int Destination_Task_Id;
    std::time_t Message_Arrival_Time;
    Message_Type Msg_Type;
    int Msg_Size;
    char Msg_Text[65]; // 64 usable chars + null terminator
};

#endif // MESSAGE_H
