cmake_minimum_required(VERSION 3.11)
project(ito_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

set(CMAKE_BUILD_TYPE "Debug")


if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
elseif(CMAKE_CXX_COMPILER MATCHES clang)
    message("using clang compiler")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I$ENV{HOME}/.local/lib/clang/17/include")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-limit-debug-info -gdwarf-4")
endif()

include_directories(${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(third_party/myrttr)


