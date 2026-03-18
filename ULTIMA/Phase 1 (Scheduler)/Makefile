# ![Team Thunder Banner](</Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg>)

# PHASE 1 - ULTIMA 2.0 - TEAM THUNDER
#
#                 .-~~~~~~~~~-._       _.-~~~~~~~~~-.
#             __.'             ~.   .~             `.__
#           .'//                 \./                 \\`.
#         .'//   PHASE 1 CLOUD    |   CODE RAIN       \\`.
#       .'//______________________|_____________________\\`.
#              || 01 01 01 01 01 01 01 01 01 ||
#              || 10 10 10 10 10 10 10 10 10 ||
#              || 01 01 01 01 01 01 01 01 01 ||
#
# Phase Label: Scheduler and Semaphore

SHELL := /bin/sh

CXX ?= c++
CXXFLAGS ?= -Wall -Wextra -pedantic -g -std=c++17
TARGET := ultima_os
OBJS := Ultima.o Sched.o Sema.o
TEST_OUTPUT := phase1_test_output.txt

.PHONY: all build run test clean rebuild

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

Ultima.o: Ultima.cpp Sched.h U2_Scheduler.h Sema.h
	$(CXX) $(CXXFLAGS) -c Ultima.cpp

Sched.o: Sched.cpp U2_Scheduler.h
	$(CXX) $(CXXFLAGS) -c Sched.cpp

Sema.o: Sema.cpp Sema.h Sched.h U2_Scheduler.h
	$(CXX) $(CXXFLAGS) -c Sema.cpp

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@./$(TARGET) > $(TEST_OUTPUT)
	@grep -F -- "--- INITIAL SYSTEM STATE ---" $(TEST_OUTPUT) >/dev/null
	@grep -F -- "--- MID-EXECUTION STATE DUMP (Proving Queueing) ---" $(TEST_OUTPUT) >/dev/null
	@grep -E -- "Task_A[[:space:]]+1[[:space:]]+(Running|Ready)" $(TEST_OUTPUT) >/dev/null
	@grep -E -- "Task_B[[:space:]]+2[[:space:]]+Blocked" $(TEST_OUTPUT) >/dev/null
	@grep -E -- "Task_C[[:space:]]+3[[:space:]]+Blocked" $(TEST_OUTPUT) >/dev/null
	@grep -F -- "Sema_value:  0" $(TEST_OUTPUT) >/dev/null
	@grep -F -- "Sema_queue:  T-2 --> T-3" $(TEST_OUTPUT) >/dev/null
	@grep -F -- "[Task 1] Finished writing. Releasing Printer..." $(TEST_OUTPUT) >/dev/null
	@grep -F -- "[Task 2] Acquired Printer! Writing data..." $(TEST_OUTPUT) >/dev/null
	@grep -F -- "[Task 3] Acquired Printer! Writing data..." $(TEST_OUTPUT) >/dev/null
	@grep -F -- "System shutting down safely." $(TEST_OUTPUT) >/dev/null
	@rm -f $(TEST_OUTPUT)
	@printf '%s\n' "Phase 1 scheduler/semaphore tests passed."

rebuild: clean build

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OUTPUT)
