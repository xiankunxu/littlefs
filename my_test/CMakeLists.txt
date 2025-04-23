cmake_minimum_required(VERSION 3.10)

# NOTE: this module name must be identical with that one specified in PYBIND11_MODULE(lfs_test_lib, m) in the source file
project(lfs_test_lib C CXX)
set(THIS_MODULE_NAME ${PROJECT_NAME})

set(LITTLEFS_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../")
add_subdirectory(
  "${LITTLEFS_MODULE_PATH}"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake-modules/littlefs"
)

# Below pybind11 module is needed by pybind11_add_module
set(PYBIND11_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pybind11")
add_subdirectory(
  "${PYBIND11_MODULE_PATH}"
  "${CMAKE_CURRENT_BINARY_DIR}/pybind11"
)
# pybind11_add_module is built on top of CMake's add_library function. It creates a shared library
# with the given name from the provided source files (PYBIND11_MODULE). The shared library is then automatically
# loaded into the Python interpreter when the module is imported.
# NOTE: this module name must be identical with that one specified in PYBIND11_MODULE(lfs_test_lib, m)
pybind11_add_module(${THIS_MODULE_NAME} main.cpp)

target_link_libraries(${THIS_MODULE_NAME} PUBLIC littlefs)

message(STATUS "CMake project configured successfully.")

add_custom_command(TARGET ${THIS_MODULE_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${THIS_MODULE_NAME}> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMENT "Copying ${THIS_MODULE_NAME} to python directory"
)
