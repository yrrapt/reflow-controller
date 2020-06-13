/*
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Designed to interact with the MAX31855 thermocouple
 *  interface IC through SPI
 *
 */
#ifndef __THERMOCOUPLE_DRIVER_H__
#define __THERMOCOUPLE_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

#include <stdio.h>

/*
 *   Define the API to interact with the thermocouple
 */ 
typedef int (*thermocouple_api_read)(struct device *dev,
                float *thermocouple_temperature,
                float *ambient_temperature,
                int *error_scv_scg_oc);

struct thermocouple_driver_api {
    thermocouple_api_read read;
};
      
/*
 *  Read the thermcouple temperatures
 */
static inline int thermocouple_read(struct device *dev,
                                    float *thermocouple_temperature,
                                    float *ambient_temperature,
                                    int *error_scv_scg_oc) {

    const struct thermocouple_driver_api *api = dev->driver_api;

    __ASSERT(api->read, "Callback pointer should not be NULL");

    return api->read(dev, thermocouple_temperature, ambient_temperature, error_scv_scg_oc);
}

#ifdef __cplusplus
}
#endif

#endif /* __THERMOCOUPLE_DRIVER_H__ */