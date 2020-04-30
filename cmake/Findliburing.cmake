find_path(liburing_INCLUDE_DIR
NAMES liburing.h
PATHS /usr/include)

find_library(liburing_LIBRARY
NAMES liburing.a
PATHS /usr/lib
)

mark_as_advanced(liburing_FOUND liburing_INCLUDE_DIR liburing_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(liburing
  REQUIRED_VARS liburing_INCLUDE_DIR liburing_LIBRARY
)

if(liburing_FOUND AND NOT TARGET liburing::liburing)
  add_library(liburing::liburing STATIC IMPORTED)
  set_target_properties(liburing::liburing PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES "C"
  INTERFACE_INCLUDE_DIRECTORIES "${liburing_INCLUDE_DIR}"
  IMPORTED_LOCATION "${liburing_LIBRARY}"
  )
endif()

