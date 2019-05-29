cmake_minimum_required(VERSION 3.13)
project(modusTcpTest C)

set(CMAKE_C_STANDARD 99)

set(INC_DIR /opt/libmodbus/include)
set(LINK_DIR /opt/libmodbus/lib)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})
link_libraries(modbus)

add_executable(modusTcpTest main.c modbusTcp.c modbusTcp.h)