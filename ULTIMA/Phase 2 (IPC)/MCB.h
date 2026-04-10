#ifndef MCB_H
#define MCB_H

#include "Sched.h"
#include "ipc.h"
#include "Sema.h"

/**
 * Author: Stewart Pawley
 * Master Control Block (MCB)
 * Purpose: Centralized struct acting as the primary registry for the OS simulation.
 * It provides the Scheduler, IPC Messenger, and standard Semaphores in one accessible footprint.
 */
struct MCB {
    Scheduler Swapper;
    ipc Messenger;
    Semaphore Monitor;
    Semaphore Printer;

    // Constructor ties the scheduler to the respective IPC and Semaphores
    MCB() :
        Swapper(),
        Messenger(32, &Swapper),
        Monitor("Monitor", 1, &Swapper),
        Printer("Printer", 1, &Swapper)
    {
    }
};

// Global instance defined in your main driver when the shared Phase 2 driver is added.
extern MCB sys_mcb;

#endif // MCB_H
