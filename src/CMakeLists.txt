cmake_minimum_required(VERSION 3.15)

project(EITN30 CXX)

include(ExternalProject)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/")
find_package(pigpio REQUIRED)

message(${pigpio_LIBRARY})

ExternalProject_Add(RF24
	PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../lib/RF24
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/RF24
	BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/install
	INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/install
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/../lib/install -DCMAKE_BUILD_TYPE=Release -DRF24_DRIVER=SPIDEV
)

file(GLOB RADIO_SOURCE "*.cpp")

add_executable(${CMAKE_PROJECT_NAME} ${RADIO_SOURCE})

add_dependencies(${CMAKE_PROJECT_NAME} RF24)

set(RF24
	${CMAKE_CURRENT_SOURCE_DIR}/../lib/install/lib/librf24.so
)

target_include_directories(${CMAKE_PROJECT_NAME}
	PUBLIC
	(
		${CMAKE_CURRENT_SOURCE_DIR}/../lib/install/include/RF24
		${pigpio_INCLUDE_DIR}
	)
)

target_link_libraries(${CMAKE_PROJECT_NAME}
	PUBLIC
	${RF24}
	${pigpio_LIBRARY}
	pthread
)

option(USE_UDP "Use a UDP socket instead of the radio hardware" FALSE)
option(VERBOSE "Print output helpful for debugging when running the program" FALSE)
option(BASE "Compiles with settings for base unit" FALSE)
option(MTU "Set MTU" 1000)

if (USE_UDP)
	add_compile_definitions(USE_UDP)
endif ()

if (VERBOSE)
	add_compile_definitions(VERBOSE)
endif ()

if (BASE)
	add_compile_definitions(BASE)
endif ()

add_compile_definitions(MTU=${MTU})