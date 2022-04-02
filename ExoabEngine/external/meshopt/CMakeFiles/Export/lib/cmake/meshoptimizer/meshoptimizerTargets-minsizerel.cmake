#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "meshoptimizer::meshoptimizer" for configuration "MinSizeRel"
set_property(TARGET meshoptimizer::meshoptimizer APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(meshoptimizer::meshoptimizer PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/meshoptimizer.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/meshoptimizer.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS meshoptimizer::meshoptimizer )
list(APPEND _IMPORT_CHECK_FILES_FOR_meshoptimizer::meshoptimizer "${_IMPORT_PREFIX}/lib/meshoptimizer.lib" "${_IMPORT_PREFIX}/bin/meshoptimizer.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
