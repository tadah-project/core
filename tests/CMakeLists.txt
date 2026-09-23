# Catch2 and its test discovery come from cmake-tools (tadah_setup_tests()).
tadah_setup_tests()

file(GLOB tests_src CONFIGURE_DEPENDS "*.cpp")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests_data DESTINATION .)

add_executable(test_core ${tests_src})
target_link_libraries(test_core PUBLIC Catch2 tadah.core)
target_include_directories(test_core PUBLIC ${Tadah.CORE_SOURCE_DIR}/external/toml11)

include(Catch)
catch_discover_tests(test_core)
