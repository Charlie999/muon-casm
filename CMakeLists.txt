cmake_minimum_required(VERSION 3.13)
IF(UNIX)
  find_program(CCACHE_PROGRAM ccache)
  IF(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    message("use ccache: " ${MUST_USE_CCACHE})
    message("ccache at: " ${CCACHE_PROGRAM})
  ELSE()
    IF(MUST_USE_CCACHE)
      message(FATAL_ERROR "ccache not found, exiting")
    ENDIF()
  ENDIF()
ELSE()
    IF(MUST_USE_CCACHE)
      IF(CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
        message("use ccache: " ${MUST_USE_CCACHE})
        message("ccache at: " ${CCACHE_PROGRAM})
      ELSE()
        message(FATAL_ERROR "ccache not impl'd on windows, exiting")
      ENDIF()
    ENDIF()
ENDIF()

project(casm)

IF(ENABLE_OPTIMIZATION)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -O2)
ENDIF()

IF(WIN32)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wa,-mbig-obj -static-libgcc -static-libstdc++")
ENDIF()

set(CMAKE_CXX_STANDARD 17)

IF(USE_GIT_VERSION)
  execute_process(COMMAND git log --pretty=format:'%h' -n 1
          OUTPUT_VARIABLE GIT_REV
          ERROR_QUIET)

  string(STRIP "${GIT_REV}" GIT_REV)
  string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
  set(VERSION "-${GIT_REV}-git")

  IF ("NOT ${GIT_REV}" STREQUAL "")
    execute_process(
            COMMAND git describe --exact-match --tags
            OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
    set(VERSION "-${GIT_TAG}")
  ENDIF()
  message("** Set version to ${VERSION}")
ENDIF()

IF(BUILD_STATIC)
  message("** Building static binaries")
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-static")
  link_libraries("-static")
  add_executable(casm-static${VERSION} src/main.cpp src/assemble.cpp src/insns.h src/asm.h src/emulate.cpp src/ucode.cpp src/int24.h)
  IF(WIN32)
    target_link_libraries(casm-static${VERSION} wsock32 ws2_32)
  ENDIF()
ELSE()
  add_executable(casm${VERSION} src/main.cpp src/assemble.cpp src/insns.h src/asm.h src/emulate.cpp src/ucode.cpp src/int24.h)
  IF(WIN32)
    target_link_libraries(casm${VERSION} wsock32 ws2_32)
  ENDIF()
ENDIF()

