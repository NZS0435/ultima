# =========================================================================
# Makefile — ULTIMA 2.0 Phase 3 (Memory Management)
# =========================================================================
# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - Phase 3 build/run/test wiring and final merge
#   Zander Hayes   - allocator-centric Linux validation workflow
#   Nicholas Kobs  - Windows/CLion transcript and evidence workflow
# =========================================================================

SHELL := /bin/sh

CXX ?= c++
CXXFLAGS ?= -Wall -Wextra -g2 -std=c++17
CPPFLAGS ?=
LDFLAGS ?=

TARGET := phase3_test
SMOKE_TARGET := phase3_smoke_test
OUTPUT_FILE ?= $(CURDIR)/phase3_transcript.txt

MMU_OBJ := mmu.o
PHASE3_MAIN_OBJ := Phase3_main.o
PHASE3_SMOKE_OBJ := phase3_smoke_test.o
PHASE2_IPC_OBJ := phase2_ipc.o
PHASE2_SCHED_OBJ := phase2_sched.o
PHASE2_SEMA_SYNC_OBJ := phase2_sema_sync.o

CORE_OBJS := $(MMU_OBJ) $(PHASE2_IPC_OBJ) $(PHASE2_SCHED_OBJ) $(PHASE2_SEMA_SYNC_OBJ)
MAIN_OBJS := $(PHASE3_MAIN_OBJ) $(CORE_OBJS)
SMOKE_OBJS := $(PHASE3_SMOKE_OBJ) $(CORE_OBJS)

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Darwin)
CURSES_LIBS ?= -lcurses
else
CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || pkg-config --libs ncurses 2>/dev/null || echo -lncurses)
endif

THREAD_LIBS ?= -pthread
LDLIBS += $(THREAD_LIBS)

.PHONY: all build run transcript test clean

all: build

build: $(TARGET) $(SMOKE_TARGET)

$(TARGET): $(MAIN_OBJS)
	$(CXX) $(LDFLAGS) -o "$@" $(MAIN_OBJS) $(LDLIBS) $(CURSES_LIBS)

$(SMOKE_TARGET): $(SMOKE_OBJS)
	$(CXX) $(LDFLAGS) -o "$@" $(SMOKE_OBJS) $(LDLIBS)

$(MMU_OBJ): mmu.cpp mmu.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(PHASE3_MAIN_OBJ): Phase3_main.cpp MCB.h mmu.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(PHASE3_SMOKE_OBJ): phase3_smoke_test.cpp MCB.h mmu.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

$(PHASE2_IPC_OBJ):
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "../Phase 2 (IPC)/ipc.cpp" -o "$@"

$(PHASE2_SCHED_OBJ):
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "../Phase 2 (IPC)/Sched.cpp" -o "$@"

$(PHASE2_SEMA_SYNC_OBJ):
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "../Phase 2 (IPC)/Sema_sync.cpp" -o "$@"

run: $(TARGET)
	./$(TARGET)

transcript: $(TARGET)
	./$(TARGET) --transcript-only --output-file "$(OUTPUT_FILE)"

test: $(SMOKE_TARGET) $(TARGET)
	./$(SMOKE_TARGET)
	./$(TARGET) --transcript-only --output-file "$(OUTPUT_FILE)"

clean:
	rm -f $(MMU_OBJ) $(PHASE3_MAIN_OBJ) $(PHASE3_SMOKE_OBJ) $(PHASE2_IPC_OBJ) $(PHASE2_SCHED_OBJ) $(PHASE2_SEMA_SYNC_OBJ) $(TARGET) $(SMOKE_TARGET) "$(OUTPUT_FILE)"
