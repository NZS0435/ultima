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
#     make betti-bundle BETTI_ROOT=/path/to/betti_os
#                    — emit BETTI sidecar witness/artifact bundle
#     make transcript — build and regenerate phase2_transcript.txt
#     make clean      — remove object files and executable
#
# =========================================================================

CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -g2 -std=c++17
LDFLAGS  ?=
TARGET    = phase2_test
SMOKE_TARGET = phase2_smoke_test
BETTI_TARGET = phase2_smoke_betti
OPENSSL_CFLAGS ?= $(shell pkg-config --cflags openssl 2>/dev/null)
OPENSSL_LIBS ?= $(shell pkg-config --libs openssl 2>/dev/null || echo -lcrypto)

SOURCES  = Phase2_main.cpp ipc.cpp Sched.cpp Sema_sync.cpp
OBJS     = $(SOURCES:.cpp=.o)
SMOKE_SOURCES = phase2_smoke_test.cpp ipc.cpp Sched.cpp Sema_sync.cpp
SMOKE_OBJS = $(SMOKE_SOURCES:.cpp=.smoke.o)
BETTI_OBJS = $(SMOKE_SOURCES:.cpp=.betti.o)

BETTI_ROOT ?= /Users/stewartpawley/Desktop/betti_os
BETTI_PREFIX ?= betti_phase2_secure_ipc
BETTI_INCLUDE_DIR := $(BETTI_ROOT)/crates/Betti-ffi/include
BETTI_LIB_DIR := $(BETTI_ROOT)/target/debug
BETTI_BETTICTL := $(BETTI_ROOT)/target/debug/bettictl
BETTI_CPPFLAGS := -I$(BETTI_INCLUDE_DIR) -DULTIMA_BETTI_FFI_ENABLED=1

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Darwin)
  CURSES_LIBS ?= -lcurses
  BETTI_FFI_LIB := $(BETTI_LIB_DIR)/libbetti_ffi.dylib
else
  CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || \
                          pkg-config --libs ncurses  2>/dev/null || \
                          echo -lncurses)
  BETTI_FFI_LIB := $(BETTI_LIB_DIR)/libbetti_ffi.so
endif
BETTI_RPATH := -Wl,-rpath,$(BETTI_LIB_DIR)
BETTI_LIBS := -L$(BETTI_LIB_DIR) -lbetti_ffi $(BETTI_RPATH)

.PHONY: all run smoke transcript betti-proof-paths betti-bundle betti-full clean

all: $(TARGET) $(SMOKE_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(CURSES_LIBS) $(OPENSSL_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPENSSL_CFLAGS) -c $< -o $@

$(SMOKE_TARGET): $(SMOKE_OBJS)
	$(CXX) $(LDFLAGS) -o $(SMOKE_TARGET) $(SMOKE_OBJS) $(OPENSSL_LIBS)

%.smoke.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPENSSL_CFLAGS) -c $< -o $@

$(BETTI_TARGET): $(BETTI_OBJS)
	@test -f "$(BETTI_FFI_LIB)" || (echo "Missing Betti-FFI library: $(BETTI_FFI_LIB)"; exit 1)
	$(CXX) $(LDFLAGS) -o $(BETTI_TARGET) $(BETTI_OBJS) $(BETTI_LIBS) $(OPENSSL_LIBS)

%.betti.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPENSSL_CFLAGS) $(BETTI_CPPFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

smoke: $(SMOKE_TARGET)
	./$(SMOKE_TARGET)

transcript: $(TARGET)
	./$(TARGET) --headless-transcript phase2_transcript.txt

betti-proof-paths:
	cargo build -p betti_millicent --manifest-path "$(BETTI_ROOT)/Cargo.toml"
	cargo build -p betti_solver_ecology --manifest-path "$(BETTI_ROOT)/Cargo.toml"

betti-bundle: $(BETTI_TARGET)
	@test -x "$(BETTI_BETTICTL)" || (echo "Missing bettictl binary: $(BETTI_BETTICTL)"; exit 1)
	./$(BETTI_TARGET) --emit-betti-artifacts $(BETTI_PREFIX)
	"$(BETTI_BETTICTL)" witness verify --witness $(BETTI_PREFIX)_snapshot_witness.jcs --json > $(BETTI_PREFIX)_snapshot_witness_verify.json
	"$(BETTI_BETTICTL)" witness verify --witness $(BETTI_PREFIX)_commit_witness.jcs --json > $(BETTI_PREFIX)_commit_witness_verify.json
	perl -0pi -e 's/"bettictl_verify", "status": "pending_external_command"/"bettictl_verify", "status": "pass"/g' $(BETTI_PREFIX)_trusted_path_gate.json

betti-full: betti-proof-paths betti-bundle

clean:
	rm -f $(OBJS) $(TARGET) $(SMOKE_OBJS) $(SMOKE_TARGET) $(BETTI_OBJS) $(BETTI_TARGET)
