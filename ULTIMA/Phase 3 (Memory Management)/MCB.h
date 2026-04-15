/* =========================================================================
 * MCB.h — Phase 3 master control block
 * =========================================================================
 * Team Thunder #001
 *
 * Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label : Phase 3 - Memory Management
 *
 * Workflow Ownership:
 *   Stewart Pawley - MCB design, scheduler/MMU integration,
 *                    core-memory semaphore ownership, final merge
 *   Zander Hayes   - allocator hookup requirements for MMU residency
 *   Nicholas Kobs  - cumulative test-output expectations for MMU + IPC
 * =========================================================================
 */

#ifndef ULTIMA_PHASE3_MCB_H
#define ULTIMA_PHASE3_MCB_H

#include "../Phase 2 (IPC)/Sched.h"
#include "../Phase 2 (IPC)/ipc.h"
#include "../Phase 1 (Scheduler)/Sema.h"
#include "mmu.h"

struct MCB {
    Scheduler Swapper;
    Semaphore Monitor;
    Semaphore Printer;
    Semaphore Core;
    ipc Messenger;
    mmu MemMgr;

    MCB() :
        Swapper(),
        Monitor("Monitor", 1, &Swapper),
        Printer("Printer", 1, &Swapper),
        Core("Core Memory", 1, &Swapper),
        Messenger(32, &Swapper),
        MemMgr(1024, '.', 64, &Swapper, &Core) {
    }
};

extern MCB sys_mcb;

#endif // ULTIMA_PHASE3_MCB_H
