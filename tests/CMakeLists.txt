cmake_minimum_required(VERSION 3.10)

find_package(Catch2 3 REQUIRED)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

set(CATCH_CONFIG_NO_WINDOWS_SEH TRUE)

foreach(test_source IN LISTS TEST_SOURCES)
  get_filename_component(test_name "${test_source}" NAME_WE)

  add_executable("${test_name}" "${test_source}")

  target_link_libraries("${test_name}"
    PRIVATE
    dmn
    Catch2::Catch2WithMain
  )

  target_compile_features("${test_name}" PRIVATE cxx_std_23)

  add_test(
    NAME "${test_name}"
    COMMAND "${test_name}"
  )

endforeach()