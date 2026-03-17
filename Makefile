SHELL := /bin/sh

PHASE1_DIR := ULTIMA/Phase 1 (Scheduler)

.PHONY: all build run test clean phase1

all: test

build phase1:
	$(MAKE) -C "$(PHASE1_DIR)" build

run:
	$(MAKE) -C "$(PHASE1_DIR)" run

test:
	$(MAKE) -C "$(PHASE1_DIR)" test

clean:
	$(MAKE) -C "$(PHASE1_DIR)" clean
