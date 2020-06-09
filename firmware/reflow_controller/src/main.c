#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <console/console.h>
#include <string.h>
#include <stdio.h>

#include <pinmux/stm32/pinmux_stm32.h>


// LED declares
#define LEDS_NUMBER                 4
#define LEDS_PORT                   DT_GPIO_LABEL(DT_NODELABEL(led_red), gpios)
#define LED_RED                     DT_GPIO_PIN(DT_NODELABEL(led_red), gpios)
#define LED_YELLOW                  DT_GPIO_PIN(DT_NODELABEL(led_yellow), gpios)
#define LED_WHITE                   DT_GPIO_PIN(DT_NODELABEL(led_white), gpios)
#define LED_BLUE                    DT_GPIO_PIN(DT_NODELABEL(led_blue), gpios)
const char LEDS[] =                 {LED_RED, LED_YELLOW, LED_WHITE, LED_BLUE};


// thermocouple declares
#define THERMOCOUPLE_SPI_DEVICE     "SPI_1"
#define THERMOCOUPLE_SPI_CS_PORT    "GPIOA"
#define THERMOCOUPLE_SPI_CS_PIN     4


// heater control declares
#define HEATER_TOP                  0
#define HEATER_BOTTOM               1
const char HEATER_PORT[][6] =       {"GPIOA", "GPIOB"};
const char HEATER_PIN[] =           {8, 9};
#define ON                          1
#define OFF                         0




// Store the device bindings
struct device *dev_leds[LEDS_NUMBER];
struct device *dev_spi;
struct device *dev_spi_cs;
struct device *dev_heaters[2];


// define the SPI configuration
struct spi_cs_control spi_cs = {
    .gpio_dev = NULL,
    .gpio_pin = 0,
    .delay = 0,
};

struct spi_config spi_cfg = {
    .frequency = 0,
    .operation = 0,
    .slave = 0,
    .cs = NULL
};


/*
 * Initialize the console
 * --------------------
 *  Prepare the console so that the program can read lines from the console
 *  provided by the user or automated interface.
 */
int init_console() {

    // setup to retrieve info from terminal
    console_getline_init();

    return 0;
}


/*
 * Initialize the LEDs
 * --------------------
 *  Prepare the LED ports so that the LEDs can be easily toggle to communicate
 *  program state
 */
int init_leds() {

    // if nothing goes wrong we'll return no error code
    int return_code = 0;

    // run through each LED enabling it
    for(int i=0; i<LEDS_NUMBER; i++) {

        // create the device binding
        dev_leds[i] = device_get_binding(LEDS_PORT);
        if (dev_leds[i] == NULL) {
            printk("FAILED: Could not setup LED number %d\n", i);
            return_code = -1;
        }

        // configure the LED pin as an output
        gpio_pin_configure(dev_leds[i], LEDS[i], GPIO_OUTPUT_ACTIVE);
        gpio_pin_set(dev_leds[i], LEDS[i], OFF);
    }

    return return_code;
}


/*
 * Initialize the SPI interface
 * --------------------
 *  Prepare the SPI interface peripheral
 */
int init_spi() {

    // get the device binding
    dev_spi = device_get_binding(THERMOCOUPLE_SPI_DEVICE);
    if (!dev_spi) {
        printk("FAILED: Could not bind to the SPI device\n");
        return -1;
    }

    // setup the chip select (CS) as a GPIO pin
    dev_spi_cs = device_get_binding(THERMOCOUPLE_SPI_CS_PORT);
    if (dev_spi_cs == NULL) {
        printk("FAILED: Could not bind to the SPI CS GPIO device\n");
        return -1;
    }
    gpio_pin_configure(dev_spi_cs, THERMOCOUPLE_SPI_CS_PIN, GPIO_OUTPUT_ACTIVE);

    // define the SPI configuration
    spi_cs.gpio_dev = dev_spi_cs;
    spi_cs.gpio_pin = 4;
    spi_cs.delay = 0;
    spi_cfg.frequency = 500000;
    spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB;
    spi_cfg.slave = 0;
    spi_cfg.cs = &spi_cs;

    return 0;
}


/*
 * Initialize the heaters
 * --------------------
 *  Prepare the heater control ports that turn on each heater element
 */
int init_heaters() {

    // get the port bindings
    dev_heaters[HEATER_TOP] = device_get_binding(HEATER_PORT + HEATER_TOP);
    dev_heaters[HEATER_BOTTOM] = device_get_binding(HEATER_PORT + HEATER_BOTTOM);
    // dev_heaters[HEATER_TOP] = device_get_binding("GPIOA");
    // dev_heaters[HEATER_BOTTOM] = device_get_binding("GPIOB");

    // configure the pins as outputs
    gpio_pin_configure(dev_heaters[HEATER_TOP], HEATER_PIN[HEATER_TOP], GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(dev_heaters[HEATER_BOTTOM], HEATER_PIN[HEATER_BOTTOM], GPIO_OUTPUT_ACTIVE);

    // ensure the heaters are turned off
    gpio_pin_set(dev_heaters[HEATER_TOP], HEATER_PIN[HEATER_TOP], OFF);
    gpio_pin_set(dev_heaters[HEATER_BOTTOM], HEATER_PIN[HEATER_BOTTOM], OFF);

    return 0;
}


/*
 * Initialize the microcontroller
 * --------------------
 *  Prepare the all the peripherals, interfaces and indicators
 */
int init() {

    // if nothing goes wrong we'll return no error code
    int return_code = 0;
    printk("Starting initialize process...\n");

    // setup the console
    return_code = return_code | init_console();
    printk("  Console initialized\n");

    // setup the LEDs
    return_code = return_code | init_leds();
    printk("  LEDs initialized\n");

    // setup the SPI interface
    return_code = return_code | init_spi();
    printk("  SPI initialized\n");

    // setup the heaters
    return_code = return_code | init_heaters();
    printk("  Heaters initialized\n");
    printk("  Initialization complete!\n");

    return return_code;  

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
int thermocouple_read(float *thermocouple_temperature, float *ambient_temperature, int *error_scv_scg_oc) {

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
    int spi_error = spi_read(dev_spi, &spi_cfg, &rx);

    if (spi_error) {
        printk("FAILED: Could not read from thermocouple SPI device\n");
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
 * Set heater on/off
 * --------------------
 *  Change the heater status to ON or OFF
 */
int heater_output(int heater, int state) {

    // set the heater output state
    gpio_pin_set(dev_heaters[heater], HEATER_PIN[heater], state);

    return 0;
}


void main(void)
{

    // initiliase
    init();

    // thermocouple values
    float thermocouple_temperature;
    float ambient_temperature;
    int thermocouple_error[3];   // short circuit to VDD / short circuit to GND / open circuit
    

    for (;;) {
        

        // ask user for input
        char *s = console_getline();
        printf("%s\n", s);

        // turn the heaters on/off
        for(int i=0; i<2; i++) {
            if (s[i] == '1') {
                heater_output(i, ON);
            } else {
                heater_output(i, OFF);
            }
        }


        // read the thermocouple values
        thermocouple_read(&thermocouple_temperature, &ambient_temperature, thermocouple_error);

        // print the thermocouple values
        printf("Thermocouple Temperature = %0.2f\n", thermocouple_temperature);
        printf("Ambient Temperature      = %0.2f\n", ambient_temperature);
        printf("Error Flags\n");
        printf("   Short to VDD          = %d\n", thermocouple_error[0]);
        printf("   Short to GND          = %d\n", thermocouple_error[1]);
        printf("   Open Circuit          = %d\n", thermocouple_error[2]);

    }
}