cmake_minimum_required(VERSION 3.13)
project(casm)

IF(WIN32)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj -static-libgcc -static-libstdc++")
ELSE()
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-static-libgcc -static-libstdc++")
ENDIF()

set(CMAKE_CXX_STANDARD 17)

add_executable(casm src/main.cpp src/assemble.cpp src/insns.h src/asm.h src/emulate.cpp src/ucode.cpp)