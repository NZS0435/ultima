# =========================================================================
# CMakeLists.txt - ULTIMA 2.0 Phase 2 CMake build
# Team Thunder #001
#
# Primary Author : Stewart Pawley
# Co-Authors     : Zander Hayes
#                  Nicholas Kobs
# =========================================================================

cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0_Phase_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

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

set(ULTIMA_PHASE2_CORE_SOURCES
    "ipc.cpp"
    "Sched.cpp"
    "Sema_sync.cpp"
)

add_library(ultima_os_phase2_core STATIC ${ULTIMA_PHASE2_CORE_SOURCES})

add_executable(ultima_os_phase2
    "Phase2_main.cpp"
)

add_executable(ultima_os_phase2_smoke
    "phase2_smoke_test.cpp"
)

target_include_directories(ultima_os_phase2
    PRIVATE
        ${CURSES_INCLUDE_DIR}
)

target_link_libraries(ultima_os_phase2_core
    PRIVATE
        Threads::Threads
)

target_link_libraries(ultima_os_phase2
    PRIVATE
        ultima_os_phase2_core
        Threads::Threads
        ${CURSES_LIBRARIES}
)

target_link_libraries(ultima_os_phase2_smoke
    PRIVATE
        ultima_os_phase2_core
        Threads::Threads
)

target_compile_definitions(ultima_os_phase2_core PRIVATE
    ULTIMA_SEPARATE_COMPILATION=1
)

set_target_properties(ultima_os_phase2_core PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
)

set_target_properties(ultima_os_phase2 PROPERTIES
    OUTPUT_NAME "phase2_test"
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
)

set_target_properties(ultima_os_phase2_smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_PHASE2_OUTPUT_DIR}"
)
