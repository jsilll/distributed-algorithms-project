# DO NAME THE SYMBOLIC VARIABLE `SOURCES`

include_directories(include)
set(SOURCES 
src/main.cpp 
src/broadcast.cpp
src/logger.cpp
src/parser.cpp 
src/perfect_link.cpp
src/receiver.cpp
src/sender.cpp
src/threaded_queue.cpp
src/udp_client.cpp
src/udp_server.cpp
)

# DO NOT EDIT THE FOLLOWING LINE
find_package(Threads)
add_executable(da_proc ${SOURCES})
target_link_libraries(da_proc ${CMAKE_THREAD_LIBS_INIT})
