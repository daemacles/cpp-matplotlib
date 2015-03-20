cmake_minimum_required (VERSION 2.6)

set (PROJECT_NAME cpp-matplotlib)

# If you want to use clang, uncomment these.
#SET (CMAKE_C_COMPILER             "/usr/bin/clang-3.5")
#SET (CMAKE_CXX_COMPILER           "/usr/bin/clang++-3.5")

project (${PROJECT_NAME})
set (EXAMPLE_BIN ${PROJECT_NAME}-example)

include_directories ("${PROJECT_SOURCE_DIR}/src")

add_executable (${EXAMPLE_BIN} src/main.cc)

## Support for Clang's CompilationDatabase system
set (CMAKE_EXPORT_COMPILE_COMMANDS 1)

## Compile and create a library.  STATIC is default unless BUILD_SHARED_LIBS
## is on.
add_library (cpp_mpl 
  src/cpp_mpl.cc 
  src/RequestSink.cc
  src/ipython_protocol.cc)

set (EXTRA_LIBS ${EXTRA_LIBS} cpp_mpl)

## Libraries to link with
target_link_libraries (${EXAMPLE_BIN} 
  ${EXTRA_LIBS}
  zmq jsoncpp uuid ssl crypto)

SET (CMAKE_C_FLAGS                "-Wall -std=c11 -Wextra -Werror")
SET (CMAKE_C_FLAGS_DEBUG          "${CMAKE_CFLAGS} -g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_CFLAGS} -Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "${CMAKE_CFLAGS} -O3 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CFLAGS} -O2 -g")

SET (CMAKE_CXX_FLAGS                "-Wall -std=c++11 -Wextra -Werror")
SET (CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXXFLAGS} -g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXXFLAGS} -Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXXFLAGS} -O3 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXXFLAGS} -O2 -g")
