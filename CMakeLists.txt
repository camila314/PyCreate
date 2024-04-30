cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(PyCreate VERSION 1.0.0)

add_library(PyBind SHARED lib/main.c)
target_link_libraries(PyBind "-framework CoreFoundation")

set(CMAKE_OSX_ARCHITECTURES "x86_64")

include(cmake/CPM.cmake)
if (DEFINED ENV{GEODE_BINDINGS_REPO_PATH})
    set(temp $ENV{GEODE_BINDINGS_REPO_PATH})
    # this is so stupid i hate windows paths
    string(REPLACE "\\" "/" GEODE_BINDINGS_REPO_PATH ${temp})
endif()

if (NOT GEODE_BINDINGS_REPO_PATH)
    CPMAddPackage(NAME "bindings"
        GITHUB_REPOSITORY "geode-sdk/bindings"
        GIT_TAG "main"
        GIT_SHALLOW
        DOWNLOAD_ONLY YES
        NO_CACHE YES
    )
    set(GEODE_BINDINGS_REPO_PATH ${bindings_SOURCE_DIR})
endif()

add_subdirectory(codegen)


add_custom_command(
    DEPENDS ${PyCodegen_BINARY_DIR}/PyCodegen
    COMMAND ${PyCodegen_BINARY_DIR}/PyCodegen ${GEODE_BINDINGS_REPO_PATH}/bindings/2.200/Entry.bro
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.cpp
)

add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_BINARY_DIR}/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
