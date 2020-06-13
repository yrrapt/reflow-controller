/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT thermocouple

#include "thermocouple_driver.h"

#include <zephyr/types.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/spi.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(thermocouple);


/*
 * SPI master configuration:
 *
 * - mode 0 (the default), 8 bit, MSB first (arbitrary), one-line SPI
 * - no shenanigans (don't hold CS, don't hold the device lock)
 */
#define SPI_OPER (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
          SPI_WORD_SET(8) | SPI_LINES_SINGLE)


/*
 *   Store the internal state within these structures
 */
struct thermocouple_data {
    struct device *spi;
};

struct thermocouple_cfg {
    struct spi_config       spi_cfg;
};

/*
 *   Use these functions to access the data within the structures
 */
static struct thermocouple_data *dev_data(struct device *dev) {
    return dev->driver_data;
}

static const struct thermocouple_cfg *dev_cfg(struct device *dev) {
    return dev->config_info;
}

/*
 * Read the thermocouple temperature
 * --------------------
 *  Read from the SPI connectred thermocouple interface IC to find the following data:
 *
 *      - Thermocouple temperature
 *      - Ambient temperature of the IC
 *      - Any error codes returned from the interface
 */
static int read_impl(   struct device *dev,
                        float *thermocouple_temperature,
                        float *ambient_temperature,
                        int *error_scv_scg_oc)
{
    // create buffer structures to store the SPI data in
    static u8_t rx_buffer[4];

    struct spi_buf rx_buf = {
        .buf = rx_buffer,
        .len = sizeof(rx_buffer),
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1
    };

    // read from the SPI device
    const struct thermocouple_cfg *cfg = dev_cfg(dev);
    int spi_error = spi_read(dev_data(dev)->spi, &cfg->spi_cfg, &rx);

    if (spi_error) {
        LOG_ERR("FAILED: Could not read from thermocouple SPI device.");
        return spi_error;
    }

    // grab the bits
    int thermocouple_temperature_fixed_point = ((rx_buffer[0] * 0x100) + (rx_buffer[1] & 0xFC)) >> 2;
    int ambient_temperature_fixed_point = ((rx_buffer[2] * 0x100) + (rx_buffer[3] & 0xF0)) >> 4;

    // convert to signed
    if (thermocouple_temperature_fixed_point & 0xC000) {
        thermocouple_temperature_fixed_point |= ~0xFFFF;
    }
    if (ambient_temperature_fixed_point & 0xF000) {
        ambient_temperature_fixed_point |= ~0xFFFF;
    }

    // convert to float
    *thermocouple_temperature = thermocouple_temperature_fixed_point;
    *thermocouple_temperature = *thermocouple_temperature / 4.0;
    *ambient_temperature = ambient_temperature_fixed_point;
    *ambient_temperature = *ambient_temperature / 16.0;
    
    // collect the errors
    error_scv_scg_oc[0] = (rx_buffer[3] & 0x04) >> 2;
    error_scv_scg_oc[1] = (rx_buffer[3] & 0x02) >> 1;
    error_scv_scg_oc[2] = rx_buffer[3] & 0x01;

    return rx_buffer[1] & 0x01;
}


/*
 *   API structure to link external calls to internal functions
 */
static const struct thermocouple_driver_api thermocouple_api = {
    .read = read_impl
};



/*
 *   Initialize the device.
 *      Performed inside define statements so that multiple instances can be created
 *      with different identification numbers (idx)
 */
#define THERMOCOUPLE_LABEL(idx)                                                             \
    (DT_INST_LABEL(idx))
#define THERMOCOUPLE_BUS(idx)                                                               \
    (DT_INST_BUS_LABEL(idx))
#define THERMOCOUPLE_SLAVE(idx)                                                             \
    (DT_INST_REG_ADDR(idx))
#define THERMOCOUPLE_FREQ(idx)                                                              \
    500000
    // (DT_INST_PROP(idx, spi_max_frequency))

#define THERMOCOUPLE_DEVICE(idx)                                                            \
                                                                                            \
    static struct thermocouple_data thermocouple_##idx##_data;                              \
                                                                                            \
    static const struct thermocouple_cfg thermocouple_##idx##_cfg = {                       \
        .spi_cfg = {                                                                        \
            .frequency = THERMOCOUPLE_FREQ(idx),                                            \
            .operation = SPI_OPER,                                                          \
            .slave = THERMOCOUPLE_SLAVE(idx),                                               \
            .cs = NULL                                                                      \
        }                                                                                   \
    };                                                                                      \
                                                                                            \
                                                                                            \
    static int thermocouple_##idx##_init(struct device *dev) {                              \
        printf("init'ed\n");                                                                \
                                                                                            \
        struct thermocouple_data *data = dev_data(dev);                                     \
                                                                                            \
        data->spi = device_get_binding(THERMOCOUPLE_BUS(idx));                              \
        if (!data->spi) {                                                                   \
            LOG_ERR("SPI device %s not found", THERMOCOUPLE_BUS(idx));                      \
            return -ENODEV;                                                                 \
        }                                                                                   \
        return 0;                                                                           \
    }                                                                                       \
                                                                                            \
                                                                                            \
    DEVICE_AND_API_INIT(thermocouple_##idx,                                                 \
                        THERMOCOUPLE_LABEL(idx),                                            \
                        thermocouple_##idx##_init,                                          \
                        &thermocouple_##idx##_data,                                         \
                        &thermocouple_##idx##_cfg,                                          \
                        POST_KERNEL,                                                        \
                        CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                 \
                        &thermocouple_api);

DT_INST_FOREACH_STATUS_OKAY(THERMOCOUPLE_DEVICE)