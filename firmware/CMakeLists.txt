# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Re-direct the directory where the 'boards' directory is found from
# $ZEPHYR_BASE to this directory.
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

# This project is only compatible with the "reflow_controller" because it has
# re-directed the 'boards' directory to a directory that only contains
# this board.
set(BOARD reflow_controller)

# Add the driver directories
list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/thermocouple_module
)

# Add an absolute directory path to the CMake variable
# SYSCALL_INCLUDE_DIRS. This ensures that the syscall machinery will
# be able to find the module's syscalls.
list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/drivers/thermocouple_module/zephyr)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
