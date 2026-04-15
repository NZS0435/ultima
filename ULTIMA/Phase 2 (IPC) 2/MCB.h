/* =========================================================================
 * MCB.h — Phase 2 master control block
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

#ifndef MCB_H

#define MCB_H

#include "Sched.h"
#include "ipc.h"
#include "../Phase 1 (Scheduler)/Sema.h"

/**
 * Master Control Block (MCB)
 * Purpose: Centralized struct acting as the primary registry for the OS
 * simulation. It provides the Scheduler, IPC Messenger, and standard
 * semaphores in one accessible footprint.
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
