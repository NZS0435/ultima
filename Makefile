# Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg
# Phase Label: Phase 1 - Scheduler and Semaphore

SHELL := /bin/sh

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
HOST_ENV := macOS
else ifneq (,$(findstring CYGWIN,$(UNAME_S)))
HOST_ENV := Cygwin
else
HOST_ENV := Unix
endif

PHASE1_DIR := ULTIMA/Phase 1 (Scheduler)
PHASE2_DIR := ULTIMA/Phase 2 (IPC)
PHASE1_OUTPUT := $(CURDIR)/phase1output.txt

.PHONY: all build run test clean phase1 phase2 transcript phase1-test phase2-test

all: test

build: phase1 phase2

phase1:
	$(MAKE) -C "$(PHASE1_DIR)" build

phase2:
	$(MAKE) -C "$(PHASE2_DIR)" build

run:
	$(MAKE) -C "$(PHASE1_DIR)" run

transcript:
	$(MAKE) -C "$(PHASE1_DIR)" transcript OUTPUT_FILE="$(PHASE1_OUTPUT)"

phase1-test:
	$(MAKE) -C "$(PHASE1_DIR)" test OUTPUT_FILE="$(PHASE1_OUTPUT)"

phase2-test:
	$(MAKE) -C "$(PHASE2_DIR)" test

test: phase1-test phase2-test

clean:
	$(MAKE) -C "$(PHASE1_DIR)" clean
	$(MAKE) -C "$(PHASE2_DIR)" clean
	rm -f "$(PHASE1_OUTPUT)"
