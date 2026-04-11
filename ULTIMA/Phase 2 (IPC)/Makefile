SHELL := /bin/sh

CXX ?= c++
AR ?= ar
ARFLAGS ?= rcs
CXXFLAGS ?= -Wall -Wextra -g2 -std=c++17
PHASE1_DIR := ../Phase 1 (Scheduler)
CPPFLAGS ?= -DULTIMA_SEPARATE_COMPILATION=1 -I. -I"$(PHASE1_DIR)"
LDFLAGS ?=
LDLIBS ?=

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
HOST_ENV := macOS
EXEEXT :=
else ifneq (,$(findstring CYGWIN,$(UNAME_S)))
HOST_ENV := Cygwin
EXEEXT := .exe
else ifeq ($(OS),Windows_NT)
HOST_ENV := Windows
EXEEXT := .exe
else
HOST_ENV := Unix
EXEEXT :=
endif

TARGET_LIB := libultima_os_phase2.a
TEST_TARGET := phase2_smoke_test$(EXEEXT)

SOURCES := Sched.cpp ipc.cpp Sema_sync.cpp
HEADERS := MCB.h Message.h Sched.h U2_Scheduler.h ipc.h
TEST_SOURCES := phase2_smoke_test.cpp

OBJS := $(SOURCES:.cpp=.o)
TEST_OBJS := $(TEST_SOURCES:.cpp=.o)

.PHONY: all build test clean

all build: $(TARGET_LIB) $(TEST_TARGET)

$(TARGET_LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(TEST_TARGET): $(TARGET_LIB) $(TEST_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(TEST_OBJS) $(TARGET_LIB) $(LDLIBS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET_LIB) $(TEST_TARGET)
