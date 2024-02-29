cmake_minimum_required(VERSION 3.12)

project(Tadah.CORE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(tadah.core STATIC
    config/config.cpp
    lapack.cpp
    maths.cpp
    utils/utils.cpp
    )

# Create compile definition to toml config directory
install(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX}/include/tadah)
set(CONFIG_FILE_DIR "${CMAKE_INSTALL_PREFIX}/include/tadah/config/")
add_compile_definitions("TADAH_CONFIG_DIR=\"${CONFIG_FILE_DIR}\"")
