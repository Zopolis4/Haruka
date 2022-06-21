find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(SOURCES
  8088.cpp
  8088.h
  8259a.cpp
  8259a.h
  jbus.cpp
  jbus.h
  jevent.cpp
  jevent.h
  jfdc.cpp
  jfdc.h
  jio1ff.cpp
  jio1ff.h
  jjoy.cpp
  jjoy.h
  jkey.cpp
  jkey.h
  jmain.cpp
  jmem.cpp
  jmem.h
  jrtc.cpp
  jrtc.h
  jtype.h
  jvideo.cpp
  jvideo.h
  sdlsound.cpp
  sdlsound.h
  sdlvideo.cpp
  sdlvideo.h
  stdfdc.cpp
  stdfdc.h
)
add_executable(5511emu ${SOURCES})

target_link_libraries(5511emu ${SDL2_LIBRARIES})
target_include_directories(5511emu PUBLIC ${SDL2_INCLUDE_DIRS})
