# Try to find sqlite3

set(SQLITE3_ROOT "${SQLITE3_ROOT}" CACHE STRING "Where to find sqlite3")

unset(SQLITE3_INCLUDE_DIR CACHE)
unset(SQLITE3_LIBRARY CACHE)

# find sqlite3
find_path(SQLITE3_INCLUDE_DIR "sqlite3.h"
    HINTS ${SQLITE3_ROOT}
    PATH_SUFFIXES "include"
    )
mark_as_advanced(SQLITE3_INCLUDE_DIR)

find_library(SQLITE3_LIBRARY sqlite3 HINTS ${SQLITE3_ROOT}
    PATH_SUFFIXES "lib")
mark_as_advanced(SQLITE3_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sqlite3 DEFAULT_MSG
    SQLITE3_INCLUDE_DIR SQLITE3_LIBRARY)

# create the target
if(NOT TARGET Sqlite3::Sqlite3)
	get_filename_component(SQLITE3_LIBRARY_EXT ${SQLITE3_LIBRARY} EXT)

	if (${SQLITE3_LIBRARY_EXT} MATCHES ".so")
		add_library(Sqlite3::Sqlite3 SHARED IMPORTED)
	elseif (${SQLITE3_LIBRARY_EXT} MATCHES ".a")
		add_library(Sqlite3::Sqlite3 STATIC IMPORTED)
	else()
		message(FATAL_ERROR "Can't determine the type of the library")
	endif()

	set_target_properties(Sqlite3::Sqlite3 PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}"
		IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
	)
endif()
