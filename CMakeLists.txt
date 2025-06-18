cmake_minimum_required(VERSION 3.12)
project(Tadah.CORE)

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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(BUILD_SHARED_LIBS)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (BUILD_SHARED_LIBS)
  # Always full RPATH
  # use, i.e. don't skip the full RPATH for the build tree
  set(CMAKE_SKIP_BUILD_RPATH FALSE)

  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  # the RPATH to be used when installing, but only if it's not a system directory
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif("${isSystemDir}" STREQUAL "-1")
endif()

# Check is build supported
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
if (CMAKE_BUILD_TYPE STREQUAL "DEBUG" 
    OR
    CMAKE_BUILD_TYPE STREQUAL "RELEASE"
    OR
    CMAKE_BUILD_TYPE STREQUAL "PROFILE"
    OR
    CMAKE_BUILD_TYPE STREQUAL "DEVELOP"
)
  message(STATUS "${PROJECT_NAME} Build type:  ${CMAKE_BUILD_TYPE}.")
else()
  message(FATAL_ERROR "
  Selected build type is not supported: ${CMAKE_BUILD_TYPE}.
  Supported build types: Release, Debug, Profile Develop"
  )
endif()

# Update the documentation string of CMAKE_BUILD_TYPE for GUIs
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
  "Choose the type of build, options are: None Debug Release Profile."
  FORCE)

if(NOT DEFINED GIT_BRANCH AND DEFINED ENV{CI_COMMIT_REF_NAME})
  set(GIT_BRANCH "$ENV{CI_COMMIT_REF_NAME}")
else()
  execute_process(
    COMMAND bash -c "git status | grep origin |sed -e 's/.*origin\\///' -e 's/[^a-zA-Z\/_-].*//'"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
message(STATUS "${PROJECT_NAME} module is using GIT_BRANCH:  ${GIT_BRANCH}")

add_subdirectory(external/toml11)
# embedd config.toml into tadah.core library
include(${PROJECT_SOURCE_DIR}/external/cmrc-master/CMakeRC.cmake)
cmrc_add_resource_library(
    config-resources
    ALIAS config::rc
    NAMESPACE CORE
    ${PROJECT_SOURCE_DIR}/blueprint/blueprint_tadah.toml
)

file(GLOB CORE_SRC CONFIGURE_DEPENDS "src/*.cpp")
add_library(tadah.core 
    ${CORE_SRC}
    $<TARGET_OBJECTS:config-resources>
    )

target_include_directories(tadah.core PUBLIC include)
target_include_directories(tadah.core PUBLIC ${CMAKE_BINARY_DIR}/_cmrc/include)
target_include_directories(tadah.core PUBLIC external/boost/include)

if(TADAH_ENABLE_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(tadah.core PRIVATE OpenMP::OpenMP_CXX)
endif()

target_link_libraries(tadah.core PUBLIC config::rc)
target_link_libraries(tadah.core PRIVATE toml11)

find_package(LAPACK)
target_link_libraries (tadah.core PUBLIC ${LAPACK_LIBRARIES})

if(TADAH_BUILD_TESTS)
  include(CTest) 
  add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS tadah.core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY include/
${CMAKE_BINARY_DIR}/_cmrc/include/
    DESTINATION include
)
