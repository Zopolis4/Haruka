cmake_minimum_required(VERSION 3.18)

project(5511emu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(5511emu ${SOURCES})

add_subdirectory(Source)

target_link_libraries(5511emu ${SDL2_LIBRARIES})
target_include_directories(5511emu PUBLIC ${SDL2_INCLUDE_DIRS})
