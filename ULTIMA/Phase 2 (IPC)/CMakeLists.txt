cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0_Phase_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Stewart's Phase 2 scope is limited to the IPC integration scaffold.
# Build it as a library until the shared driver and remaining teammate logic land.
find_package(Threads REQUIRED)

set(PHASE1_SCHEDULER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Phase 1 (Scheduler)")

if(CYGWIN)
    set(ULTIMA_PHASE2_OUTPUT_DIR "/cygdrive/c/Users/$ENV{USERNAME}/.ultima2/phase2/cygwin-debug")
elseif(WIN32)
    set(ULTIMA_PHASE2_OUTPUT_DIR "$ENV{USERPROFILE}/.ultima2/phase2/windows-debug")
elseif(APPLE)
    set(ULTIMA_PHASE2_OUTPUT_DIR "$ENV{HOME}/.ultima2/phase2/macos-debug")
else()
    set(ULTIMA_PHASE2_OUTPUT_DIR "$ENV{HOME}/.ultima2/phase2/linux-debug")
endif()

if(NOT CYGWIN)
    file(TO_CMAKE_PATH "${ULTIMA_PHASE2_OUTPUT_DIR}" ULTIMA_PHASE2_OUTPUT_DIR)
endif()

add_library(ultima_os_phase2 STATIC
    "MCB.h"
    "Message.h"
    "Sched.h"
    "Sched.cpp"
    "U2_Scheduler.h"
    "ipc.h"
    "ipc.cpp"
    "Sema_sync.cpp"
        ../Introduction/Makefile
        Phase2_main.cpp
)

add_executable(ultima_os_phase2_smoke
    "phase2_smoke_test.cpp"
        ../Introduction/Makefile
        Phase2_main.cpp
)

target_include_directories(ultima_os_phase2 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PHASE1_SCHEDULER_DIR}"
)

target_include_directories(ultima_os_phase2_smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PHASE1_SCHEDULER_DIR}"
)

set_target_properties(ultima_os_phase2 PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
)

set_target_properties(ultima_os_phase2_smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
)

target_compile_definitions(ultima_os_phase2 PRIVATE
    ULTIMA_SEPARATE_COMPILATION=1
)

target_link_libraries(ultima_os_phase2 PRIVATE Threads::Threads)
target_link_libraries(ultima_os_phase2_smoke PRIVATE ultima_os_phase2 Threads::Threads)
