cmake_minimum_required(VERSION 3.10)
project(ULTIMA_2_0 LANGUAGES CXX)

include(cmake/UltimaPhase1Target.cmake)

add_executable(ultima_os)
ultima_configure_phase1_target(ultima_os)
