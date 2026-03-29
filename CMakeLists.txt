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
    "ULTIMA/Phase 1 (Scheduler)/Sched.cpp"
    "ULTIMA/Phase 1 (Scheduler)/Sema.cpp"
    "ULTIMA/Phase 1 (Scheduler)/U2_UI.cpp"
    "ULTIMA/Phase 1 (Scheduler)/U2_Window.cpp"
    "ULTIMA/Phase 1 (Scheduler)/Ultima.cpp"
)

add_executable(ultima_os)
ultima_configure_phase1_target(ultima_os)
