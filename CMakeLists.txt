cmake_minimum_required (VERSION 2.6)

set (PROJECT_NAME cpp-matplotlib)

SET (CMAKE_C_COMPILER             "/usr/bin/clang-3.5")
SET (CMAKE_CXX_COMPILER           "/usr/bin/clang++-3.5")

project (${PROJECT_NAME})

include_directories ("${PROJECT_SOURCE_DIR}")

add_executable (${PROJECT_NAME} main.cc)

## Support for Clang's CompilationDatabase system
set (CMAKE_EXPORT_COMPILE_COMMANDS 1)

## Set optional features.  This will show up as a preprocessor variable
## USE_MY_LIBRARY in source.
#option (USE_MY_LIBRARY
#        "Use the provided library" ON)

## Compile and create a library.  STATIC is default unless BUILD_SHARED_LIBS
## is on.
add_library (cpp_plot cpp_plot.cc)
set (EXTRA_LIBS ${EXTRA_LIBS} cpp_plot)

#if (USE_MY_LIBRARY)

## Search for include files here as well
#include_directories ("${PROJECT_SOURCE_DIR}/some_sub_path")

## Run Cmake also in this dir
#add_subdirectory (some_sub_path)

#set (EXTRA_LIBS ${EXTRA_LIBS} LibraryName)

#endif (USE_MYMATH)

## Libraries to link with
target_link_libraries (${PROJECT_NAME} zmq ${EXTRA_LIBS})


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
