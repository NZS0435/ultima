# Team Thunder JPEG: /Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg
# Phase Label: Phase 1 - Scheduler and Semaphore

SHELL := /bin/sh

PHASE1_DIR := ULTIMA/Phase 1 (Scheduler)
PHASE1_OUTPUT := $(CURDIR)/phase1output.txt

.PHONY: all build run test clean phase1 transcript

all: test

build phase1:
	$(MAKE) -C "$(PHASE1_DIR)" build

run:
	$(MAKE) -C "$(PHASE1_DIR)" run

transcript:
	$(MAKE) -C "$(PHASE1_DIR)" transcript OUTPUT_FILE="$(PHASE1_OUTPUT)"

test:
	$(MAKE) -C "$(PHASE1_DIR)" test OUTPUT_FILE="$(PHASE1_OUTPUT)"

clean:
	$(MAKE) -C "$(PHASE1_DIR)" clean
	rm -f "$(PHASE1_OUTPUT)"
