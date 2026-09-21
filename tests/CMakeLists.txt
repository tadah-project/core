set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/catch2)
add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CATCH_INCLUDE_DIR})

file(GLOB tests_src "*.cpp")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests_data DESTINATION .)

add_executable(test_core ${tests_src})
target_link_libraries(test_core PUBLIC Catch2 tadah.core)
target_include_directories(test_core PRIVATE Catch2 tadah.core)
target_include_directories(test_core PUBLIC ${Tadah.CORE_SOURCE_DIR}/external/toml11)

include(Catch.cmake)
catch_discover_tests(test_core)

# The vendored Catch discovery script folds some existing bracket-containing
# names into a single nonmatching filter. Register this regression group
# explicitly as well, so the registered suite actually executes its cases.
add_test(NAME core_pc_dataset_indices COMMAND test_core "[pc_dataset]")
