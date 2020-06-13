# SPDX-License-Identifier: Apache-2.0

if(CONFIG_THERMOCOUPLE_DRIVER)

  # Add thermocouple_driver.h to the set of global include paths.
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    thermocouple_driver.c
    )
endif()