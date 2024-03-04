cmake_minimum_required(VERSION 3.12)
project(Tadah.CORE)

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles dir. ")
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

add_library(tadah.core STATIC $<TARGET_OBJECTS:tadah.core.obj>)

if(NOT DEFINED CONFIG_FILE_DIR)
    set(CONFIG_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
endif()
add_compile_definitions("TADAH_CONFIG_DIR=\"${CONFIG_FILE_DIR}\"")

include(CTest) 
if(TADAH_BUILD_TESTS)
    add_subdirectory(tests)
endif()

set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/catch2)
