cmake_minimum_required(VERSION 3.10)
project(littlefs LANGUAGES C)
set(THIS_MODULE_NAME ${PROJECT_NAME})

add_library(${THIS_MODULE_NAME} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lfs_util.c
)

# Specify include directories for the library
# This makes the header files available to targets linking against this library.
target_include_directories(${THIS_MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
