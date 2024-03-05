cmake_minimum_required(VERSION 3.12)
project(Tadah.CORE)

get_directory_property(hasParent PARENT_DIRECTORY)

# Guard against in-source builds
# Also, make sure user does not accidently invoke cmake .. from the source dir
# of the module in case module is added as a submodule.
get_filename_component(MODULE_TEST ${CMAKE_BINARY_DIR} NAME)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR}
        OR ${MODULE_TEST} STREQUAL "CORE"
        )
    message(FATAL_ERROR "
    In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt and CMakeFiles dir. ")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB CORE_SRC CONFIGURE_DEPENDS "*.cpp" "*.h" "*.hpp")
file(GLOB_RECURSE CONFIG_SRC CONFIGURE_DEPENDS "config/*.cpp" "config/*.h" "config/*.hpp")
file(GLOB_RECURSE LIBS_SRC CONFIGURE_DEPENDS "lib/*.cpp" "lib/*.h" "lib/*.hpp")
file(GLOB_RECURSE UTILS_SRC CONFIGURE_DEPENDS "utils/*.cpp" "utils/*.h" "utils/*.hpp")

add_library(tadah.core.obj OBJECT
    ${CORE_SRC}
    ${CONFIG_SRC}
    ${LIBS_SRC}
    ${UTILS_SRC}
    )

if(TADAH_ENABLE_OPENMP)
    find_package(OpenMP REQUIRED)
    target_link_libraries(tadah.core.obj PUBLIC OpenMP::OpenMP_CXX)
endif()

add_library(tadah.core $<TARGET_OBJECTS:tadah.core.obj>)
find_package(LAPACK)
target_link_libraries (tadah.core PRIVATE ${LAPACK_LIBRARIES})

if(NOT DEFINED CONFIG_FILE_DIR)
    set(CONFIG_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
    add_compile_definitions("TADAH_CONFIG_DIR=\"${CONFIG_FILE_DIR}\"")
endif()

include(CTest) 
if(TADAH_BUILD_TESTS)
    add_subdirectory(tests)
endif()

set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/catch2)
