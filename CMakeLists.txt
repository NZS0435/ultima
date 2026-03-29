cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0 LANGUAGES CXX)

include(cmake/UltimaPhase1Target.cmake)

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

# 4. Link both libraries to your executable
target_link_libraries(ultima_os Threads::Threads ${CURSES_LIBRARIES})
