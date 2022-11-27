# DO NAME THE SYMBOLIC VARIABLE `SOURCES`

include_directories(include)
set(SOURCES 
src/main.cpp 
src/drivers.cpp
src/info_display.cpp
src/logger.cpp
src/parser.cpp 
src/perfect_link.cpp
src/udp_client.cpp
src/udp_server.cpp
src/broadcast.cpp
src/best_effort_broadcast.cpp
src/fifo_broadcast.cpp
src/uniform_reliable_broadcast.cpp
src/lattice_agreement.cpp
)

# DO NOT EDIT THE FOLLOWING LINE
find_package(Threads)
add_executable(da_proc ${SOURCES})
target_link_libraries(da_proc ${CMAKE_THREAD_LIBS_INIT})
