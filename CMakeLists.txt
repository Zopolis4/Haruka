cmake_minimum_required(VERSION 3.18)

project(Haruka LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
  add_compile_options(-fmodules-ts)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(Haruka ${SOURCES})

add_subdirectory(Source)

target_link_libraries(Haruka ${SDL2_LIBRARIES})
target_include_directories(Haruka PUBLIC ${SDL2_INCLUDE_DIRS})
