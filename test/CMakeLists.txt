cmake_minimum_required(VERSION 3.7)
project(test)

find_package(Boost 1.49 COMPONENTS
    unit_test_framework
)

if(NOT Boost_FOUND)
  message(WARNING "Boost not found, don't build tests")
  return()
endif(NOT Boost_FOUND)

# rvnsqlite

add_executable(test_rvnsqlite
  test_sqlite.cpp
)


target_include_directories(test_rvnsqlite PRIVATE "../include")
target_include_directories(test_rvnsqlite PRIVATE "../src")

target_link_libraries(test_rvnsqlite
  PUBLIC
    Boost::boost
  PRIVATE
    rvnsqlite
    Boost::unit_test_framework
)

target_compile_definitions(test_rvnsqlite PRIVATE "BOOST_TEST_DYN_LINK")

add_test(rvnsqlite::base test_rvnsqlite)

# rvnsqlite_query

add_executable(test_rvnsqlite_query
  test_query.cpp
)

target_include_directories(test_rvnsqlite_query PRIVATE "../include")
target_include_directories(test_rvnsqlite_query PRIVATE "../src")

target_link_libraries(test_rvnsqlite_query
  PUBLIC
    Boost::boost
  PRIVATE
    rvnsqlite
    Boost::unit_test_framework
)

target_compile_definitions(test_rvnsqlite_query PRIVATE "BOOST_TEST_DYN_LINK")


add_test(rvnsqlite::query test_rvnsqlite_query)

# rvnsqlite_resource

add_executable(test_rvnsqlite_resource
  test_resource.cpp
)

target_include_directories(test_rvnsqlite_resource PRIVATE "../include")
target_include_directories(test_rvnsqlite_resource PRIVATE "../src")

target_link_libraries(test_rvnsqlite_resource
  PUBLIC
    Boost::boost
  PRIVATE
    rvnsqlite
    Boost::unit_test_framework
)

target_compile_definitions(test_rvnsqlite_resource PRIVATE "BOOST_TEST_DYN_LINK")


add_test(rvnsqlite::resource test_rvnsqlite_resource)
