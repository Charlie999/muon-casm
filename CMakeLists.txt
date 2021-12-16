cmake_minimum_required(VERSION 3.13)
project(casm)

IF(WIN32)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj -static-libgcc -static-libstdc++")
ELSE()
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-static-libgcc -static-libstdc++")
ENDIF()

set(CMAKE_CXX_STANDARD 17)

add_executable(casm main.cpp assemble.cpp insns.h asm.h emulate.cpp ucode.cpp)