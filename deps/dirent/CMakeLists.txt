# Use #include <dirent.h>
# to include dirent in your files.
# Angle brackets first search system files and then project includes.
# On unix and mac dirent is a system library, but on windows this version is used.

add_library(dirent INTERFACE)
if (WIN32)
  target_include_directories(dirent INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
