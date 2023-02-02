cmake_minimum_required(VERSION 3.15)

include(../RF24/CMakeLists.txt)

project(EITN30 CXX)

file(GLOB RADIO_SOURCE "src/*.cpp")

add_executable(Radio ${RADIO_SOURCE})

target_link_libraries(Radio RF24)
