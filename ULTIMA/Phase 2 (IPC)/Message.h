#ifndef MESSAGE_H
#define MESSAGE_H

#include <ctime>

/**
 * Author: Zander Hayes
 * Co-Author: Stewart Pawley (Integration)
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
    char Msg_Text[32]; // fixed buffer (avoids dynamic alloc complexity)
};

#endif // MESSAGE_H
