cmake_minimum_required(VERSION 3.18)

project(5511emu)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(5511emu jmem.cpp jtype.h 8088.cpp jevent.cpp jio1ff.cpp jkey.cpp jmem.h jvideo.cpp
	           sdlsound.cpp stdfdc.cpp 8088.h jevent.h jio1ff.h jkey.h jvideo.h sdlsound.h
	           stdfdc.h jrtc.cpp 8259a.cpp jbus.cpp jfdc.cpp jjoy.cpp jmain.cpp jrtc.h
	           sdlvideo.cpp 8259a.h jbus.h jfdc.h jjoy.h sdlvideo.h)

target_link_libraries(5511emu ${SDL2_LIBRARIES})
target_include_directories(5511emu PUBLIC ${SDL2_INCLUDE_DIRS})
