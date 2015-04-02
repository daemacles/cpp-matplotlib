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

set (LIB_ERROR "FALSE")
macro(require_library _NAME _name)
  find_library (${_NAME} ${_name})
  if (${_NAME} STREQUAL "${_NAME}-NOTFOUND")
    set (LIB_ERROR 1)
    message(SEND_ERROR "Please install dev files for lib ${_name}")
  else (${_NAME} STREQUAL "${_NAME}-NOTFOUND")
    set (LIBRARIES ${LIBRARIES} ${_name})
  endif (${_NAME} STREQUAL "${_NAME}-NOTFOUND")
endmacro(require_library)

require_library (ZMQ zmq)
require_library (JSONCPP jsoncpp)
require_library (UUID uuid)
require_library (SSL ssl)
require_library (CRYPTO crypto)

if (${LIB_ERROR})
  message(FATAL_ERROR "Please install prerequisite libraries.  See README.md for details")
endif (${LIB_ERROR})

add_library (cpp_mpl SHARED
  src/cpp_mpl.cc 
  src/RequestSink.cc
  src/ipython_protocol.cc)
target_link_libraries (cpp_mpl ${LIBRARIES})

set (EXTRA_LIBS ${EXTRA_LIBS} cpp_mpl)

## Libraries for the example to link against
target_link_libraries (${EXAMPLE_BIN} 
  ${EXTRA_LIBS})

## Installation
install (TARGETS cpp_mpl LIBRARY DESTINATION lib)
install (FILES src/cpp_mpl.hpp DESTINATION include)

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
