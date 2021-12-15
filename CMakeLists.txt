cmake_minimum_required(VERSION 3.16)
project(casm)

IF(WIN32)
FIND_PACKAGE( Boost 1.40 COMPONENTS REQUIRED )
ELSE()
FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )
ENDIF()

INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} )

MESSAGE("Boost include dirs: " ${Boost_INCLUDE_DIRS})
MESSAGE("Boost libraries: " ${Boost_LIBRARIES})

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj -static-libgcc -static-libstdc++")

set(CMAKE_CXX_STANDARD 14)

add_executable(casm main.cpp assemble.cpp insns.h asm.h emulate.cpp)
IF(WIN32)
  target_link_libraries(casm "C:\\SDKs\\boost\\lib\\libboost_program_options-mgw92-1_62.a")
ELSE()
  target_link_libraries(casm ${Boost_LIBRARIES} )
ENDIF()
