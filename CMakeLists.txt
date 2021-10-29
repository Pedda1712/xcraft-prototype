cmake_minimum_required(VERSION 3.16)
project(XCRAFT)

include_directories(${CMAKE_SOURCE_DIR}/include  ${CMAKE_SOURCE_DIR}/src)
link_directories(${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR})

set (CLIENT_SOURCES
	${CMAKE_SOURCE_DIR}/src/main.c
	${CMAKE_SOURCE_DIR}/src/pnoise.c
	${CMAKE_SOURCE_DIR}/src/chunkbuilder.c
	${CMAKE_SOURCE_DIR}/src/dynamicarray.c
	${CMAKE_SOURCE_DIR}/src/bmp24.c
	${CMAKE_SOURCE_DIR}/src/chunklist.c
	${CMAKE_SOURCE_DIR}/src/generator.c
	${CMAKE_SOURCE_DIR}/src/globallists.c
	${CMAKE_SOURCE_DIR}/src/lightcalc.c
	${CMAKE_SOURCE_DIR}/src/blocktexturedef.c
	${CMAKE_SOURCE_DIR}/src/bmpfont.c
	${CMAKE_SOURCE_DIR}/src/game.c
	${CMAKE_SOURCE_DIR}/src/player.c
	${CMAKE_SOURCE_DIR}/src/physics.c
	${CMAKE_SOURCE_DIR}/src/ui.c
	${CMAKE_SOURCE_DIR}/src/genericlist.c
	${CMAKE_SOURCE_DIR}/src/worldsave.c
	${CMAKE_SOURCE_DIR}/src/shader.c
)

set (STRUCED_SOURCES
	${CMAKE_SOURCE_DIR}/src/struced/main.c
	${CMAKE_SOURCE_DIR}/src/struced/appstate.c
	
	${CMAKE_SOURCE_DIR}/src/bmp24.c
	${CMAKE_SOURCE_DIR}/src/bmpfont.c
	${CMAKE_SOURCE_DIR}/src/genericlist.c
	${CMAKE_SOURCE_DIR}/src/ui.c
	${CMAKE_SOURCE_DIR}/src/blocktexturedef.c
)

add_executable(xcraft ${CLIENT_SOURCES})
target_compile_options(xcraft PUBLIC -O2 -Wall -Wextra)
target_link_libraries(xcraft -lxcraftwm -lX11 -lGL -lGLU -lGLEW -lm -lpthread -no-pie)

add_executable(xcraft-struced ${STRUCED_SOURCES})
target_compile_options(xcraft-struced PUBLIC -O2 -Wall -Wextra)
target_link_libraries(xcraft-struced -lxcraftwm -lX11 -lGL -lGLU -lGLEW -lm -lpthread -no-pie)
