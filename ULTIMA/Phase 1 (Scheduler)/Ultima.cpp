#include "Sched.h"
#include "Sema.h"
#include <iostream>

int main() {
    Scheduler scheduler;

    std::cout << "Creating tasks..." << std::endl;
    scheduler.create_task("Task 1");
    int t2 = scheduler.create_task("Task 2");
    scheduler.create_task("Task 3");

    std::cout << "Initial Process Table (All READY):" << std::endl;
    scheduler.dump(0);

    std::cout << "\nFirst Yield (Should pick Task 1 to RUN):" << std::endl;
    scheduler.yield();
    scheduler.dump(0);

    std::cout << "\nSecond Yield (Should pick Task 2 to RUN):" << std::endl;
    scheduler.yield();
    scheduler.dump(0);

    std::cout << "\nThird Yield (Should pick Task 3 to RUN):" << std::endl;
    scheduler.yield();
    scheduler.dump(0);

    std::cout << "\nKilling Task 2..." << std::endl;
    scheduler.kill_task(t2);
    scheduler.dump(0);

    std::cout << "\nGarbage Collecting..." << std::endl;
    scheduler.garbage_collect();
    scheduler.dump(0);

    std::cout << "\nSemaphore Test:" << std::endl;
    Semaphore sem(1, &scheduler);

    std::cout << "P() called (Value 1 -> 0, no block)" << std::endl;
    sem.P();
    scheduler.dump(0);

    std::cout << "P() called (Value 0 -> -1, block current task)" << std::endl;
    sem.P();
    scheduler.dump(0);

    std::cout << "V() called (Value -1 -> 0, unblock waiting task)" << std::endl;
    sem.V();
    scheduler.dump(0);

    return 0;
}
