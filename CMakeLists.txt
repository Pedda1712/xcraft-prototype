cmake_minimum_required(VERSION 3.16)
project(XCRAFT)

include_directories(${CMAKE_SOURCE_DIR}/include )
link_directories(${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR})

set (XCRAFT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.c

)

add_executable(xcraft ${XCRAFT_SOURCES})
target_compile_options(xcraft PUBLIC -O3 -Wall -Wextra)
target_link_libraries(xcraft -lxgame -lX11 -lGL -lGLU -lm)
