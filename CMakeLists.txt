cmake_minimum_required(VERSION 3.16)

project(ULTIMA_2_0 LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
            "In-source builds are not supported.\n"
            "Configure with: cmake -S . -B <local-build-dir>"
    )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIncludeFileCXX)

set(SCHEDULER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ULTIMA/Phase 1 (Scheduler)")
set(ULTIMA_SOURCES
        "${SCHEDULER_DIR}/Ultima.cpp"
        "${SCHEDULER_DIR}/Sched.cpp"
        "${SCHEDULER_DIR}/Sema.cpp"
        "${SCHEDULER_DIR}/U2_UI.cpp"
        "${SCHEDULER_DIR}/U2_Window.cpp"
)

add_executable(ultima_os ${ULTIMA_SOURCES})

find_package(Threads REQUIRED)
target_link_libraries(ultima_os PRIVATE Threads::Threads)

if(WIN32 AND NOT CYGWIN)
    set(PDCURSES_ROOT "" CACHE PATH "Root path of a PDCurses install (optional)")

    set(ULTIMA_CURSES_INCLUDE_HINTS
            "$ENV{PDCURSES_ROOT}/include"
            "C:/msys64/mingw64/include"
            "C:/msys64/ucrt64/include"
            "C:/msys64/clang64/include"
            "C:/vcpkg/installed/x64-windows/include"
    )
    set(ULTIMA_CURSES_LIBRARY_HINTS
            "$ENV{PDCURSES_ROOT}/lib"
            "C:/msys64/mingw64/lib"
            "C:/msys64/ucrt64/lib"
            "C:/msys64/clang64/lib"
            "C:/vcpkg/installed/x64-windows/lib"
    )

    if(PDCURSES_ROOT)
        list(PREPEND ULTIMA_CURSES_INCLUDE_HINTS "${PDCURSES_ROOT}/include")
        list(PREPEND ULTIMA_CURSES_LIBRARY_HINTS "${PDCURSES_ROOT}/lib")
    endif()

    find_path(ULTIMA_CURSES_INCLUDE_DIR
            NAMES curses.h ncurses.h
            HINTS ${ULTIMA_CURSES_INCLUDE_HINTS}
    )

    find_library(ULTIMA_CURSES_LIBRARY
            NAMES pdcurses pdcursesw curses ncursesw ncurses
            HINTS ${ULTIMA_CURSES_LIBRARY_HINTS}
    )

    if(NOT ULTIMA_CURSES_INCLUDE_DIR OR NOT ULTIMA_CURSES_LIBRARY)
        message(FATAL_ERROR
                "Curses library not found on Windows.\n"
                "Install PDCurses (MSYS2: pacman -S mingw-w64-x86_64-pdcurses)\n"
                "or install through vcpkg, then pass -DPDCURSES_ROOT=<path> if needed."
        )
    endif()

    target_include_directories(ultima_os PRIVATE "${ULTIMA_CURSES_INCLUDE_DIR}")
    target_link_libraries(ultima_os PRIVATE "${ULTIMA_CURSES_LIBRARY}")
else()
    find_package(Curses REQUIRED)

    if(CURSES_INCLUDE_DIRS)
        target_include_directories(ultima_os PRIVATE ${CURSES_INCLUDE_DIRS})
    elseif(CURSES_INCLUDE_DIR)
        target_include_directories(ultima_os PRIVATE "${CURSES_INCLUDE_DIR}")
    endif()

    if(CURSES_LIBRARIES)
        target_link_libraries(ultima_os PRIVATE ${CURSES_LIBRARIES})
    elseif(CURSES_LIBRARY)
        target_link_libraries(ultima_os PRIVATE "${CURSES_LIBRARY}")
    else()
        message(FATAL_ERROR "Curses package found, but no link libraries were provided by CMake.")
    endif()
endif()

check_include_file_cxx("pthread.h" ULTIMA_HAS_PTHREAD_HEADER)
if(ULTIMA_HAS_PTHREAD_HEADER)
    target_compile_definitions(ultima_os PRIVATE ULTIMA_USE_NATIVE_PTHREADS=1)
endif()

if(MSVC)
    target_compile_definitions(ultima_os PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()