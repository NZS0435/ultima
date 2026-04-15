# =========================================================================
# CMakeLists.txt - ULTIMA 2.0 Phase 3 CMake build
# =========================================================================
# Team Thunder #001
#
# Team Members: Stewart Pawley, Zander Hayes, Nicholas Kobs
#
# Workflow Ownership:
#   Stewart Pawley - Phase 3 build integration and final merge
#   Zander Hayes   - Linux allocator-build validation expectations
#   Nicholas Kobs  - Windows/CLion workflow alignment expectations
# =========================================================================

cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0_Phase_3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

if(CYGWIN)
    set(ULTIMA_PHASE3_OUTPUT_DIR "/cygdrive/c/Users/$ENV{USERNAME}/.ultima2/phase3/cygwin-debug")
elseif(WIN32)
    set(ULTIMA_PHASE3_OUTPUT_DIR "$ENV{USERPROFILE}/.ultima2/phase3/windows-debug")
elseif(APPLE)
    set(ULTIMA_PHASE3_OUTPUT_DIR "$ENV{HOME}/.ultima2/phase3/macos-debug")
else()
    set(ULTIMA_PHASE3_OUTPUT_DIR "$ENV{HOME}/.ultima2/phase3/linux-debug")
endif()

if(NOT CYGWIN)
    file(TO_CMAKE_PATH "${ULTIMA_PHASE3_OUTPUT_DIR}" ULTIMA_PHASE3_OUTPUT_DIR)
endif()

set(ULTIMA_PHASE3_CORE_SOURCES
    "mmu.cpp"
    "../Phase 2 (IPC)/ipc.cpp"
    "../Phase 2 (IPC)/Sched.cpp"
    "../Phase 2 (IPC)/Sema_sync.cpp"
)

add_library(ultima_os_phase3_core STATIC ${ULTIMA_PHASE3_CORE_SOURCES})

target_include_directories(ultima_os_phase3_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(ultima_os_phase3_core
    PRIVATE
        Threads::Threads
)

add_executable(ultima_os_phase3 "Phase3_main.cpp")
add_executable(ultima_os_phase3_smoke "phase3_smoke_test.cpp")

target_link_libraries(ultima_os_phase3
    PRIVATE
        ultima_os_phase3_core
        Threads::Threads
        ${CURSES_LIBRARIES}
)

target_include_directories(ultima_os_phase3
    PRIVATE
        ${CURSES_INCLUDE_DIR}
)

target_link_libraries(ultima_os_phase3_smoke
    PRIVATE
        ultima_os_phase3_core
        Threads::Threads
)

set_target_properties(ultima_os_phase3_core PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${ULTIMA_PHASE3_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ULTIMA_PHASE3_OUTPUT_DIR}"
)

set_target_properties(ultima_os_phase3 PROPERTIES
    OUTPUT_NAME "phase3_test"
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_PHASE3_OUTPUT_DIR}"
)

set_target_properties(ultima_os_phase3_smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_PHASE3_OUTPUT_DIR}"
)
