set(ULTIMA_PHASE1_DIR "${CMAKE_CURRENT_LIST_DIR}/../ULTIMA/Phase 1 (Scheduler)")

set(ULTIMA_PHASE1_SOURCES
    "${ULTIMA_PHASE1_DIR}/Sched.cpp"
    "${ULTIMA_PHASE1_DIR}/Sema.cpp"
    "${ULTIMA_PHASE1_DIR}/U2_UI.cpp"
    "${ULTIMA_PHASE1_DIR}/U2_Window.cpp"
    "${ULTIMA_PHASE1_DIR}/Ultima.cpp"
)

function(ultima_configure_phase1_target target_name)
    find_package(Threads REQUIRED)
    find_package(Curses REQUIRED)

    target_sources(${target_name} PRIVATE ${ULTIMA_PHASE1_SOURCES})
    target_include_directories(${target_name} PRIVATE "${ULTIMA_PHASE1_DIR}")

    if(DEFINED CURSES_INCLUDE_DIRS)
        target_include_directories(${target_name} PRIVATE ${CURSES_INCLUDE_DIRS})
    elseif(DEFINED CURSES_INCLUDE_DIR)
        target_include_directories(${target_name} PRIVATE "${CURSES_INCLUDE_DIR}")
    endif()

    target_link_libraries(${target_name} PRIVATE Threads::Threads ${CURSES_LIBRARIES})
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()
