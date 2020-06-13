#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <console/console.h>
#include <string.h>
#include <stdio.h>

#include <pinmux/stm32/pinmux_stm32.h>
#include "thermocouple_driver.h"


// LED declares
#define LEDS_NUMBER                 4
#define LEDS_PORT                   DT_GPIO_LABEL(DT_NODELABEL(led_red), gpios)
#define LED_RED                     DT_GPIO_PIN(DT_NODELABEL(led_red), gpios)
#define LED_YELLOW                  DT_GPIO_PIN(DT_NODELABEL(led_yellow), gpios)
#define LED_WHITE                   DT_GPIO_PIN(DT_NODELABEL(led_white), gpios)
#define LED_BLUE                    DT_GPIO_PIN(DT_NODELABEL(led_blue), gpios)
const char LEDS[] =                 {LED_RED, LED_YELLOW, LED_WHITE, LED_BLUE};


#define THERMOCOUPLE_LABEL          "THERMOCOUPLE"


// heater control declares
#define HEATER_TOP                  0
#define HEATER_BOTTOM               1
const char HEATER_PORT[][6] =       {"GPIOA", "GPIOB"};
const char HEATER_PIN[] =           {8, 9};
#define ON                          1
#define OFF                         0


// Store the device bindings
struct device *dev_leds[LEDS_NUMBER];
struct device *dev_heaters[2];
struct device *dev_thermocouple;


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

    // setup the heaters
    return_code = return_code | init_heaters();
    printk("  Heaters initialized\n");

    // setup the thermocouple interface
    dev_thermocouple = device_get_binding(THERMOCOUPLE_LABEL);

    printk("  Initialization complete!\n");
    return return_code;  

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
        thermocouple_read(dev_thermocouple, &thermocouple_temperature, &ambient_temperature, thermocouple_error);

        // print the thermocouple values
        printf("Thermocouple Temperature = %0.2f\n", thermocouple_temperature);
        printf("Ambient Temperature      = %0.2f\n", ambient_temperature);
        printf("Error Flags\n");
        printf("   Short to VDD          = %d\n", thermocouple_error[0]);
        printf("   Short to GND          = %d\n", thermocouple_error[1]);
        printf("   Open Circuit          = %d\n", thermocouple_error[2]);

    }
}