/* =========================================================================
 * phase3_smoke_test.cpp — Phase 3 non-interactive cumulative validation
 * =========================================================================
 * Team Thunder #001
 *
 * Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
 *
 * Workflow Ownership:
 *   Stewart Pawley - cumulative integration validation and final smoke path
 *   Zander Hayes   - allocator assertions and free-space verification
 *   Nicholas Kobs  - I/O, protection-failure, and evidence assertions
 * =========================================================================
 */

#include "MCB.h"

#include <cassert>
#include <cstring>
#include <iostream>

MCB sys_mcb;

namespace {

struct SmokeState {
    int owner_id = -1;
    int wait_id = -1;

    int owner_phase = 0;
    int wait_phase = 0;

    int owner_handle = -1;
    int wait_handle = -1;

    bool ipc_seen = false;
    bool blocked_seen = false;
    bool wake_seen = false;
    bool protection_seen = false;
};

SmokeState smoke;

void owner_task() {
    if (smoke.owner_phase == 0) {
        smoke.owner_handle = sys_mcb.MemMgr.Mem_Alloc(35);
        assert(smoke.owner_handle != -1);
        assert(sys_mcb.MemMgr.get_handle_limit(smoke.owner_handle) == 64);

        assert(sys_mcb.MemMgr.Mem_Write(smoke.owner_handle, 'O') == 1);
        assert(sys_mcb.MemMgr.Mem_Write(smoke.owner_handle, 'S') == 1);

        assert(sys_mcb.Messenger.Message_Send(smoke.owner_id, smoke.wait_id, "owner ready", 0) == 1);
        smoke.owner_phase = 1;
        return;
    }

    if (smoke.owner_phase == 1) {
        assert(sys_mcb.MemMgr.Mem_Free(smoke.owner_handle) == 1);
        smoke.wake_seen = (sys_mcb.Swapper.get_task_state(smoke.wait_id) == READY);
        sys_mcb.Swapper.kill_task(smoke.owner_id);
        smoke.owner_phase = 2;
    }
}

void wait_task() {
    if (smoke.wait_phase == 0) {
        if (!smoke.ipc_seen) {
            Message incoming {};
            assert(sys_mcb.Messenger.Message_Receive(smoke.wait_id, &incoming) == 1);
            assert(std::strcmp(incoming.Msg_Text, "owner ready") == 0);
            smoke.ipc_seen = true;
        }

        char leaked = '\0';
        if (sys_mcb.MemMgr.Mem_Read(smoke.owner_handle, &leaked) == -1) {
            smoke.protection_seen = true;
        }

        smoke.wait_handle = sys_mcb.MemMgr.Mem_Alloc(980);
        if (smoke.wait_handle == -1) {
            smoke.blocked_seen = (sys_mcb.Swapper.get_task_state(smoke.wait_id) == BLOCKED);
            return;
        }

        assert(sys_mcb.MemMgr.get_handle_limit(smoke.wait_handle) == 1024);
        smoke.wait_phase = 1;
        return;
    }

    if (smoke.wait_phase == 1) {
        const char payload[] = "phase3";
        char readback[sizeof(payload)] = {0};

        assert(sys_mcb.MemMgr.Mem_Write(
            smoke.wait_handle,
            0,
            static_cast<int>(std::strlen(payload)),
            payload
        ) == static_cast<int>(std::strlen(payload)));

        assert(sys_mcb.MemMgr.Mem_Read(
            smoke.wait_handle,
            0,
            static_cast<int>(std::strlen(payload)),
            readback
        ) == static_cast<int>(std::strlen(payload)));

        assert(std::strcmp(readback, payload) == 0);
        assert(sys_mcb.MemMgr.Mem_Free(smoke.wait_handle) == 1);

        sys_mcb.Swapper.kill_task(smoke.wait_id);
        smoke.wait_phase = 2;
    }
}

} // namespace

int main() {
    EventLog::instance().clear();
    sys_mcb.Swapper.reset();
    sys_mcb.Messenger.init(32, &sys_mcb.Swapper);
    sys_mcb.Monitor.reset();
    sys_mcb.Printer.reset();
    sys_mcb.Core.reset();
    sys_mcb.MemMgr.init(&sys_mcb.Swapper, &sys_mcb.Core);
    sys_mcb.MemMgr.reset();

    smoke = SmokeState {};
    smoke.owner_id = sys_mcb.Swapper.create_task("Smoke_Owner", owner_task);
    smoke.wait_id = sys_mcb.Swapper.create_task("Smoke_Waiter", wait_task);

    int guard = 0;
    while (sys_mcb.Swapper.has_active_tasks() && guard < 8) {
        ++guard;
        sys_mcb.Swapper.yield();
    }

    sys_mcb.Swapper.garbage_collect();

    assert(smoke.ipc_seen);
    assert(smoke.blocked_seen);
    assert(smoke.wake_seen);
    assert(smoke.protection_seen);
    assert(sys_mcb.MemMgr.Mem_Left() == 1024);
    assert(sys_mcb.MemMgr.waiting_task_count() == 0);
    assert(sys_mcb.Messenger.Message_Count() == 0);
    assert(sys_mcb.Swapper.get_active_task_count() == 0);

    std::cout << "Phase 3 smoke test passed.\n";
    return 0;
}
