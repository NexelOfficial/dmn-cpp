cmake_minimum_required(VERSION 3.25)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(
  dmn
  VERSION 1.0.4
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

file(
  GLOB_RECURSE DMN_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(dmn SHARED ${DMN_SOURCES})
add_library(dmn::dmn ALIAS dmn)

target_compile_features(dmn PUBLIC cxx_std_23)

target_include_directories(
  dmn PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(MSVC)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<$<CONFIG:Release>:ProgramDatabase>"
  )
  
  target_link_options(dmn PRIVATE $<$<CONFIG:Release>:/DEBUG>)
endif()

if(WIN32)
  target_compile_definitions(
    dmn PRIVATE
    W32 ND64 _WIN32_WINNT=0x0A00
  )

  add_library(notes::notes STATIC IMPORTED)

  set_target_properties(
    dmn PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
  )
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(
    dmn PRIVATE
    UNIX LINUX LINUX64
  )

  add_library(notes::notes SHARED IMPORTED)
endif()

find_library(
  NOTES_LIBRARY
  NAMES notes
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/domino/lib"
  NO_DEFAULT_PATH
  REQUIRED
)

set_target_properties(notes::notes PROPERTIES
  IMPORTED_LOCATION "${NOTES_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/domino/include"
)

target_link_libraries(dmn PRIVATE notes::notes)

set_target_properties(
  dmn PROPERTIES
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  EXPORT_NAME dmn
)

install(
  TARGETS dmn
  EXPORT dmnTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  EXPORT dmnTargets
  FILE dmnTargets.cmake
  NAMESPACE dmn::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dmn
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmnConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dmnConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dmn"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dmnConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
  "${CMAKE_CURRENT_BINARY_DIR}/dmnConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/dmnConfigVersion.cmake"
  DESTINATION
  "${CMAKE_INSTALL_LIBDIR}/cmake/dmn"
)

set(CPACK_PACKAGE_NAME "dmn")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
  "dmn-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)

if(WIN32)
  set(CPACK_GENERATOR ZIP)
else()
  set(CPACK_GENERATOR TGZ)
endif()

set(
  CPACK_PACKAGE_DIRECTORY
  "${CMAKE_BINARY_DIR}/packages"
)

include(CPack)
include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()