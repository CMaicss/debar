cmake_minimum_required(VERSION 3.10)
project(debar)

set(CMAKE_CXX_STANDARD 17)


include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
)

FetchContent_MakeAvailable(yaml-cpp)

find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)

message(${CMAKE_INSTALL_PREFIX})

add_subdirectory(src)