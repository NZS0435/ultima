# Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg
# Phase Label: Phase 1 - Scheduler and Semaphore

SHELL := /bin/sh

CXX ?= c++
CXXFLAGS ?= -Wall -Wextra -g2 -std=c++17
CPPFLAGS ?= -DULTIMA_ENABLE_CURSES=1 -DULTIMA_SEPARATE_COMPILATION=1
LDFLAGS ?=

TARGET ?= ultima_os
OUTPUT_FILE ?= $(CURDIR)/phase1output.txt

SOURCES := Ultima.cpp Sched.cpp Sema.cpp U2_UI.cpp U2_Window.cpp
OBJS := $(SOURCES:.cpp=.o)

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
CURSES_LIBS ?= -lpdcurses
else ifeq ($(UNAME_S),Darwin)
CURSES_LIBS ?= -lcurses
else
CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || pkg-config --libs ncurses 2>/dev/null || echo -lncurses)
endif

THREAD_LIBS ?= -pthread
LDLIBS += $(THREAD_LIBS) $(CURSES_LIBS)

.PHONY: all build run test transcript clean

all build: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

transcript: $(TARGET)
	./$(TARGET) --transcript-only --output-file "$(OUTPUT_FILE)"

test: transcript

clean:
	rm -f $(OBJS) $(TARGET)
