# Build examples
# TODO(abbyssoul): Disabled as not supported by clang (without gold linker that is)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fwhole-program")


# Async serial port example
set(EXAMPLE_async_serial_SOURCE_FILES async_serial.cpp)
add_executable(async_serial ${EXAMPLE_async_serial_SOURCE_FILES})
target_link_libraries(async_serial ${PROJECT_NAME})


# Async server
set(EXAMPLE_async_server_SOURCE_FILES async_server.cpp)
add_executable(async_server ${EXAMPLE_async_server_SOURCE_FILES})
target_link_libraries(async_server ${PROJECT_NAME})


# Sync serial port example
set(EXAMPLE_serial_SOURCE_FILES serial.cpp)
add_executable(serial ${EXAMPLE_serial_SOURCE_FILES})
target_link_libraries(serial ${PROJECT_NAME})
