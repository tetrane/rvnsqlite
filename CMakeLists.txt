cmake_minimum_required(VERSION 3.7)
project(rvnsqlite)

include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

option(BUILD_SHARED_LIBS "Set to ON to build shared libraries; OFF for static libraries." OFF)

option(WARNING_AS_ERROR "Set to ON to build with -Werror" ON)

option(BUILD_TEST_COVERAGE "Set to ON to build while generating coverage information. Will put source on the build directory." OFF)

find_package(sqlite3 PATHS ${CMAKE_SOURCE_DIR}/cmake REQUIRED)

add_library(rvnsqlite
  src/sqlite.cpp
  src/resource_database.cpp
)

target_compile_options(rvnsqlite PRIVATE -W -Wall -Wextra -Wmissing-include-dirs -Wunknown-pragmas -Wpointer-arith
-Wmissing-field-initializers -Wno-multichar -Wreturn-type)


if(WARNING_AS_ERROR)
  target_compile_options(rvnsqlite PRIVATE -Werror)
endif()

if(BUILD_TEST_COVERAGE)
  target_compile_options(rvnsqlite PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
  target_link_libraries(rvnsqlite PRIVATE gcov)
endif()

target_include_directories(rvnsqlite
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(rvnsqlite
  PUBLIC
    Sqlite3::Sqlite3
)

set(PUBLIC_HEADERS
  include/sqlite.h
  include/query.h
  include/resource_database.h
)

set_target_properties(rvnsqlite PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
  POSITION_INDEPENDENT_CODE ON
)

include(GNUInstallDirs)
install(TARGETS rvnsqlite
  EXPORT rvnsqlite-export
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(EXPORT rvnsqlite-export
  FILE
    rvnsqlite-targets.cmake
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/cmake/rvnsqlite
)

install(FILES cmake/rvnsqlite-config.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/rvnsqlite
)

install(FILES cmake/sqlite3-config.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/sqlite3
)

enable_testing()
add_subdirectory(test)
