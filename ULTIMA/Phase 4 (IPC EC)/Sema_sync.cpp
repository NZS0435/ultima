/* =========================================================================
 * Sema_sync.cpp — Phase 1 semaphore bridge for Phase 2
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

#include "U2_Scheduler.h"

// Compile the Phase 1 semaphore implementation against the Phase 2 scheduler
// definitions so the phases stay stacked instead of diverging.
#include "../Phase 1 (Scheduler)/Sema.cpp"
