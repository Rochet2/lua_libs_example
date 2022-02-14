file(GLOB LOCAL_SOURCES *.c *.cpp *.h *.hpp)

add_executable(lua_example "${LOCAL_SOURCES}")
target_include_directories(lua_example PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lua_example dirent lualib sol)

if (MSVC)
  # For easier debug starting, set the VS debugger working directory to installation directory
  set_target_properties(lua_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")

  # install output files
  install(TARGETS lua_example DESTINATION "${CMAKE_INSTALL_PREFIX}")
  install(FILES $<TARGET_PDB_FILE:lua_example> DESTINATION "${CMAKE_INSTALL_PREFIX}" OPTIONAL)
else()
  install(TARGETS lua_example DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif ()
