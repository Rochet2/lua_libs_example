cmake_minimum_required (VERSION 2.6)
project(lua_example)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")

# disable "bin dir"
# we cannot use a single folder for output since some files have same name and they overwrite each other
# please use install target that cmake provides instead
unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)

# set default "install dir" to what default "bin dir" would be
# this will make it easier for people accustomed to bin dir on windows
if (WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" CACHE PATH "default install path" FORCE)
endif()

# set some warning levels and flags
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else ()
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-long-long -Wno-format-security")
endif ()

# set a relative hardcoded RPATH that is used to find dynamic libraries, like lua
# we dont want to accidentally use system-provided libs
if (APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
endif()

add_subdirectory(deps)
add_subdirectory(src)
