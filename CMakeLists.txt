cmake_minimum_required(VERSION 3.25.1)
project(npchat)

add_subdirectory(external/npsystem/3rd/spdlog)

set(BOOST_LIB_PREFIX "/usr/lib" CACHE STRING "Location of the boost libs")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
else()
  set(CMAKE_BUILD_TYPE "Release")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
  message(WARNING "CMAKE_BUILD_TYPE is not set to Debug or Release, defaulting to Release")
endif()

message("Building: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wno-deprecated-declarations)

include_directories(
  "${CMAKE_SOURCE_DIR}/external/npsystem/nprpc/include"
  "${CMAKE_SOURCE_DIR}/external/npsystem/nplib/include"
)

add_subdirectory(external/npsystem/nplib)
add_subdirectory(external/npsystem/nprpc)

add_dependencies(nprpc nplib)

include(external/npsystem/nprpc/cmake/npidl.cmake)

npidl_generate_idl_files("${CMAKE_SOURCE_DIR}/idl/npchat.npidl" npchat_stub)

add_subdirectory(server)
add_subdirectory(client)
