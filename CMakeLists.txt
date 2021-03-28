cmake_minimum_required(VERSION 3.16)
project(XCRAFT)

include_directories(${CMAKE_SOURCE_DIR}/include  ${CMAKE_SOURCE_DIR}/src)
link_directories(${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR})

set (SHARED_SOURCES
	${CMAKE_SOURCE_DIR}/src/clientlist.c
)

set (CLIENT_SOURCES
	${CMAKE_SOURCE_DIR}/src/Client/client.c
	${CMAKE_SOURCE_DIR}/src/Client/netthread_cli.c
	${CMAKE_SOURCE_DIR}/src/Client/worldthread_cli.c
)

set (SERVER_SOURCES
	${CMAKE_SOURCE_DIR}/src/Server/server.c
	${CMAKE_SOURCE_DIR}/src/Server/netthread_srv.c
	${CMAKE_SOURCE_DIR}/src/Server/world_srv.c
)

add_executable(xcraft_client ${CLIENT_SOURCES} ${SHARED_SOURCES})
target_compile_options(xcraft_client PUBLIC -O2 -Wall -Wextra -no-pie)
target_link_libraries(xcraft_client -lxgame -lX11 -lGL -lGLU -lm -lpthread )

add_executable(xcraft_server ${SERVER_SOURCES} ${SHARED_SOURCES})
target_compile_options(xcraft_server PUBLIC -O2 -Wall -Wextra -no-pie)
target_link_libraries(xcraft_server -lm -lpthread)
