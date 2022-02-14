add_library(sol INTERFACE)
target_include_directories(sol INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(sol INTERFACE lualib)
target_compile_features (sol
  INTERFACE
    cxx_std_17
)

option(SOL_CHECK_ARGUMENTS "Check function arguments for lua functions, see http://sol2.readthedocs.io/en/latest/safety.html" ON)
if (SOL_CHECK_ARGUMENTS)
  target_compile_definitions(sol INTERFACE SOL_CHECK_ARGUMENTS)
endif()
