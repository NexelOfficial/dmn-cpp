cmake_minimum_required(VERSION 3.20)
project(dmn VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /Zi")
  add_compile_options("/utf-8")
endif()

if(WIN32)
  add_compile_definitions(W32 ND64 _WIN32_WINNT=0x0A00)
elseif(UNIX)
  add_compile_definitions(UNIX LINUX LINUX64)
endif()

file(GLOB_RECURSE ALL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(dmn ${ALL_SOURCES})

add_library(dmn::dmn ALIAS dmn)

target_compile_features(dmn PUBLIC cxx_std_23)

target_link_libraries(dmn
  PRIVATE
  $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/notes.lib>
  $<$<PLATFORM_ID:Linux>:$ENV{Notes_ExecDirectory}/libnotes.so>
)

set_target_properties(dmn PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME "dmn"
)

target_include_directories(dmn
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include
)

install(
  TARGETS dmn
  EXPORT dmnTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
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
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dmn"
)