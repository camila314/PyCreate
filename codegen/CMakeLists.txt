cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
project(PyCodegen VERSION 0.1.0 LANGUAGES CXX C)

include(./CPM.cmake)

CPMAddPackage("gh:camila314/Broma#master")
CPMAddPackage("gh:fmtlib/fmt#master")

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(PyCodegen ${SOURCES})

target_link_libraries(PyCodegen PRIVATE Broma fmt)
