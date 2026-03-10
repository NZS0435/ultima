cmake_minimum_required(VERSION 3.16)

project(ULTIMA_2_0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIncludeFileCXX)

set(SCHEDULER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ULTIMA/Phase 1 (Scheduler)")
set(ULTIMA_SOURCES
    main.cpp
    "${SCHEDULER_DIR}/Sched.cpp"
    "${SCHEDULER_DIR}/Sema.cpp"
    "${SCHEDULER_DIR}/U2_UI.cpp"
    "${SCHEDULER_DIR}/U2_Window.cpp"
)

add_executable(ultima_os ${ULTIMA_SOURCES})
target_include_directories(ultima_os PRIVATE "${SCHEDULER_DIR}")

find_package(Threads REQUIRED)
target_link_libraries(ultima_os PRIVATE Threads::Threads)

if(WIN32)
    set(PDCURSES_ROOT "C:/msys64/mingw64" CACHE PATH "Root path of a PDCurses install" FORCE)

    find_path(ULTIMA_CURSES_INCLUDE_DIR
        NAMES curses.h ncurses.h
        HINTS
            "${PDCURSES_ROOT}/include"
            "$ENV{PDCURSES_ROOT}/include"
            "C:/msys64/mingw64/include"
            "C:/msys64/ucrt64/include"
            "C:/msys64/clang64/include"
            "C:/vcpkg/installed/x64-windows/include"
    )

    find_library(ULTIMA_CURSES_LIBRARY
        NAMES pdcurses pdcursesw curses ncursesw ncurses
        HINTS
            "${PDCURSES_ROOT}/lib"
            "$ENV{PDCURSES_ROOT}/lib"
            "C:/msys64/mingw64/lib"
            "C:/msys64/ucrt64/lib"
            "C:/msys64/clang64/lib"
            "C:/vcpkg/installed/x64-windows/lib"
    )

    if(NOT ULTIMA_CURSES_INCLUDE_DIR OR NOT ULTIMA_CURSES_LIBRARY)
        message(FATAL_ERROR
            "Curses library not found on Windows.\n"
            "Install PDCurses (MSYS2: pacman -S mingw-w64-x86_64-pdcurses) "
            "and configure with -DPDCURSES_ROOT=<path> if needed."
        )
    endif()

    target_include_directories(ultima_os PRIVATE "${ULTIMA_CURSES_INCLUDE_DIR}")
    target_link_libraries(ultima_os PRIVATE "${ULTIMA_CURSES_LIBRARY}")
else()
    find_package(Curses REQUIRED)
    target_include_directories(ultima_os PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(ultima_os PRIVATE ${CURSES_LIBRARIES})
endif()

check_include_file_cxx("pthread.h" ULTIMA_HAS_PTHREAD_HEADER)
if(ULTIMA_HAS_PTHREAD_HEADER)
    target_compile_definitions(ultima_os PRIVATE ULTIMA_USE_NATIVE_PTHREADS=1)
endif()

if(MSVC)
    target_compile_definitions(ultima_os PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()
