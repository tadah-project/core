# Catch2 and its test discovery come from cmake-tools (tadah_setup_tests()).
tadah_setup_tests()

file(GLOB tests_src CONFIGURE_DEPENDS "*.cpp")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests_data DESTINATION .)

add_executable(test_core ${tests_src})
# Matrix_Base::row()/col() must return views whatever the compiler elides:
# this file is compiled without the optional copy elisions (NRVO), so a view
# returned through a named temporary would arrive as a copy there.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(test_matrix_views_no_elision.cpp PROPERTIES
    COMPILE_OPTIONS -fno-elide-constructors)
endif()
target_link_libraries(test_core PUBLIC Catch2 tadah.core)
target_include_directories(test_core PUBLIC ${Tadah.CORE_SOURCE_DIR}/external/toml11)

include(Catch)
catch_discover_tests(test_core)

# tadah.core_headers (ctest -L headers): every installed header, include/ and
# the generated blueprint header, compiles on its own and twice in a row.
# Before the fix: passes as well (a pin: no CORE header failed when the
# check was added).
tadah_add_header_check(tadah.core HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated)
