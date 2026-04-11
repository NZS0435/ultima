# =========================================================================
# Makefile — ULTIMA 2.0 Phase 2 (Message Passing / IPC)
# Team Thunder #001
# Phase Label   : Phase 2 - Message Passing (IPC)
# Primary Author: Stewart Pawley
# Co-Authors    : Zander Hayes
#                 Nicholas Kobs
# =========================================================================
#
# USAGE:
#   This repo keeps the Phase 1 semaphore implementation in the sibling
#   "Phase 1 (Scheduler)" directory and bridges it through `Sema_sync.cpp`.
#   Build from this directory with:
#     make            — build the phase2_test executable
#     make run        — build and run
#     make clean      — remove object files and executable
#
# =========================================================================

CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -g2 -std=c++17
LDFLAGS  ?=
TARGET    = phase2_test

SOURCES  = Phase2_main.cpp ipc.cpp Sched.cpp Sema_sync.cpp
OBJS     = $(SOURCES:.cpp=.o)

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Darwin)
  CURSES_LIBS ?= -lcurses
else
  CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || \
                          pkg-config --libs ncurses  2>/dev/null || \
                          echo -lncurses)
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(CURSES_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
