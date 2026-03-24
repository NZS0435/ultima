cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0)

set(CMAKE_CXX_STANDARD 11)

# 1. Find the pthread library
find_package(Threads REQUIRED)

# 2. Find the ncurses library
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# 3. Add your executable and source files (include all your .cpp files here)
add_executable(ultima_os
    main.cpp
    "ULTIMA/Phase 1 (Scheduler)/Sched.cpp"
    "ULTIMA/Phase 1 (Scheduler)/Sema.cpp"
    "ULTIMA/Phase 1 (Scheduler)/U2_UI.cpp"
    "ULTIMA/Phase 1 (Scheduler)/U2_Window.cpp"
)

# 4. Link both libraries to your executable
target_link_libraries(ultima_os Threads::Threads ${CURSES_LIBRARIES})