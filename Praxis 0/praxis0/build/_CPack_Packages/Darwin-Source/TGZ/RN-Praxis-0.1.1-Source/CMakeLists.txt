project(RN-Praxis)
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_STANDARD 11)

add_executable(hello_world hello_world.c)

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES ${CMAKE_BINARY_DIR} /\\..*$)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack) 