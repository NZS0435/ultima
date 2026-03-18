# Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg
# Phase Label: Phase 1 - Scheduler and Semaphore

SHELL := /bin/sh

CXX ?= c++
CXXFLAGS ?= -Wall -Wextra -pedantic -g -std=c++17
LDLIBS ?= -lncurses -pthread
TARGET := ultima_os
OBJS := Ultima.o Sched.o Sema.o U2_UI.o U2_Window.o
TEST_OUTPUT := phase1_test_output.txt
OUTPUT_FILE ?= ../../phase1output.txt

.PHONY: all build run transcript test clean rebuild

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

Ultima.o: Ultima.cpp Sched.h U2_Scheduler.h Sema.h U2_UI.h U2_Window.h
	$(CXX) $(CXXFLAGS) -c Ultima.cpp

Sched.o: Sched.cpp U2_Scheduler.h
	$(CXX) $(CXXFLAGS) -c Sched.cpp

Sema.o: Sema.cpp Sema.h Sched.h U2_Scheduler.h
	$(CXX) $(CXXFLAGS) -c Sema.cpp

U2_UI.o: U2_UI.cpp U2_UI.h U2_Window.h platform_curses.h
	$(CXX) $(CXXFLAGS) -c U2_UI.cpp

U2_Window.o: U2_Window.cpp U2_Window.h platform_curses.h
	$(CXX) $(CXXFLAGS) -c U2_Window.cpp

run: $(TARGET)
	./$(TARGET)

transcript: $(TARGET)
	@./$(TARGET) --transcript-only --output-file "$(OUTPUT_FILE)" >/dev/null
	@printf '%s\n' "Phase 1 transcript written to $(OUTPUT_FILE)"

test: $(TARGET)
	@./$(TARGET) --transcript-only --output-file "$(OUTPUT_FILE)" > $(TEST_OUTPUT)
	@grep -F -- "--- INITIAL SYSTEM STATE ---" "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "--- MID-EXECUTION STATE DUMP (Proving Queueing) ---" "$(OUTPUT_FILE)" >/dev/null
	@grep -E -- "Task_A[[:space:]]+1[[:space:]]+(Running|Ready)" "$(OUTPUT_FILE)" >/dev/null
	@grep -E -- "Task_B[[:space:]]+2[[:space:]]+Blocked" "$(OUTPUT_FILE)" >/dev/null
	@grep -E -- "Task_C[[:space:]]+3[[:space:]]+Blocked" "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "Sema_value:  0" "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "Sema_queue:  T-2 --> T-3" "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "[Task_B] acquired Printer_Output." "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "[Task_C] acquired Printer_Output." "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "--- POST-GARBAGE-COLLECTION DUMP ---" "$(OUTPUT_FILE)" >/dev/null
	@grep -F -- "System shutting down safely." "$(OUTPUT_FILE)" >/dev/null
	@rm -f $(TEST_OUTPUT)
	@printf '%s\n' "Phase 1 scheduler/semaphore tests passed. Output saved to $(OUTPUT_FILE)"

rebuild: clean build

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OUTPUT) "$(OUTPUT_FILE)"
