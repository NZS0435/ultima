#ifndef SEMA_H
#define SEMA_H

#include "Sched.h"

class Semaphore {
private:
    int value;
    Scheduler* scheduler;

public:
    Semaphore(int init_val, Scheduler* sched);
    void P(); // Down / Wait
    void V(); // Up / Signal
};

#endif // SEMA_H