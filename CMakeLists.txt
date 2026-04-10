cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1. Find the pthread library
find_package(Threads REQUIRED)

# 2. Find the ncurses library
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# 3. Add your executable and source files (include all your .cpp files here)
add_executable(ultima_os
    "ULTIMA/Phase 1 (Scheduler)/Mem_mgr.cpp"
    "ULTIMA/Phase 1 (Scheduler)/Sched.cpp"
    "ULTIMA/Phase 1 (Scheduler)/Sema.cpp"
    "ULTIMA/Phase 1 (Scheduler)/U2_UI.cpp"
    "ULTIMA/Phase 1 (Scheduler)/U2_Window.cpp"
    "ULTIMA/Phase 1 (Scheduler)/Ultima.cpp"
)

if(CYGWIN)
    set(ULTIMA_RUNTIME_OUTPUT_DIR "/cygdrive/c/Users/$ENV{USERNAME}/.ultima2/run/cygwin-debug")
elseif(WIN32)
    set(ULTIMA_RUNTIME_OUTPUT_DIR "$ENV{USERPROFILE}/.ultima2/run/windows-debug")
elseif(APPLE)
    set(ULTIMA_RUNTIME_OUTPUT_DIR "$ENV{HOME}/.ultima2/run/macos-debug")
else()
    set(ULTIMA_RUNTIME_OUTPUT_DIR "$ENV{HOME}/.ultima2/run/linux-debug")
endif()

if(NOT CYGWIN)
    file(TO_CMAKE_PATH "${ULTIMA_RUNTIME_OUTPUT_DIR}" ULTIMA_RUNTIME_OUTPUT_DIR)
endif()
set_target_properties(ultima_os PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ULTIMA_RUNTIME_OUTPUT_DIR}"
)

target_compile_definitions(ultima_os PRIVATE
    ULTIMA_ENABLE_CURSES=1
    ULTIMA_SEPARATE_COMPILATION=1
)

# 4. Link both libraries to your executable
target_link_libraries(ultima_os Threads::Threads ${CURSES_LIBRARIES})

add_subdirectory("ULTIMA/Phase 2 (IPC)")

set(PHASE1_LEGACY_DIR "${CMAKE_SOURCE_DIR}/ULTIMA/Phase 1 (Scheduler)")
set(PHASE1_LEGACY_ULTIMA_OS "${PHASE1_LEGACY_DIR}/ultima_os")
set(PHASE1_LEGACY_ULTIMA "${PHASE1_LEGACY_DIR}/Ultima")

add_custom_command(TARGET ultima_os POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ultima_os>" "${PHASE1_LEGACY_ULTIMA_OS}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ultima_os>" "${PHASE1_LEGACY_ULTIMA}"
    COMMENT "Syncing legacy Phase 1 executables"
    VERBATIM
)

if(APPLE)
    add_custom_command(TARGET ultima_os POST_BUILD
        COMMAND /usr/bin/codesign --force --sign - "$<TARGET_FILE:ultima_os>"
        COMMAND /usr/bin/codesign --force --sign - "${PHASE1_LEGACY_ULTIMA_OS}"
        COMMAND /usr/bin/codesign --force --sign - "${PHASE1_LEGACY_ULTIMA}"
        COMMENT "Ad hoc signing Phase 1 executables for macOS taskgated"
        VERBATIM
    )
endif()
