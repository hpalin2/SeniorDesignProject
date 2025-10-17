cmake_minimum_required(VERSION 3.18)
project(room-suction-status VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.0+5
)

FetchContent_MakeAvailable(crow)

add_executable(room-suction-status
  src/main.cpp
)

target_link_libraries(room-suction-status PRIVATE Crow::Crow)

target_compile_features(room-suction-status PRIVATE cxx_std_20)

if(MSVC)
  target_compile_options(room-suction-status PRIVATE /W4 /permissive-)
else()
  target_compile_options(room-suction-status PRIVATE -Wall -Wextra -Wpedantic)
endif()
