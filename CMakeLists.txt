cmake_minimum_required(VERSION 3.18)

project(5511emu)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(5511emu jmem.cc jtype.hh 8088.cc jevent.cc jio1ff.cc jkey.cc jmem.hh jvideo.cc
	           sdlsound.cc stdfdc.cc 8088.hh jevent.hh jio1ff.hh jkey.hh jvideo.hh sdlsound.hh
	           stdfdc.hh jrtc.cc 8259a.cc jbus.cc jfdc.cc jjoy.cc jmain.cc jrtc.hh sdlvideo.cc
	           8259a.hh jbus.hh jfdc.hh jjoy.hh sdlvideo.hh)

target_link_libraries(5511emu ${SDL2_LIBRARIES})
target_include_directories(5511emu PUBLIC ${SDL2_INCLUDE_DIRS})
