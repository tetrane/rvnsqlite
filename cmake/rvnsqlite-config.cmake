get_filename_component(RVNSQLITE_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(sqlite3 REQUIRED)

if(NOT TARGET rvnsqlite)
  include("${RVNSQLITE_CMAKE_DIR}/rvnsqlite-targets.cmake")
endif()
