#ifndef keyboard_driver_h
#define keyboard_driver_h

#include <linux/ioctl.h>

//#include "keyboard-interrupt.h"

#define AUTHOR "David Nicuesa Aranda | david.nicuesa.aranda@gmail.com"
#define DEVICE_NAME "simple-keyboard"
#define LICENSE "Dual BSD/GPL"
#define DESCRIPTION "This module is intended to include a driver for the simple keyboard on the BeagleBone"
#define VERSION "1.0"

#define COUNT 1

/* Values to be used within user space */
#define NUM_CHARS_PER_KEY 1
#define UNDEFINED_KEY 0
#define RIGHT 1
#define START 2
#define UP 3
#define DOWN 4
#define ESCAPE 5
#define LEFT 6

#define KEYBOARD_RESET 0
#define KEYBOARD_CONFIG_MULTI_LINE 1
#define KEYBOARD_CONFIG_SINGLE_LINE 2

#define KEYBOARD_MAGIC (0xDA) //Magic number 0xDA is unused in this kernel currently


/* Define commnands to configure the input data mode
 *
 * KEYBOARD_CONFIG_MULTI_LINE:
 *    Uses one irq for each input line (GPIO), this is 6 irqs to handle each line separately.
 *    Since the BeagleBone splits its GPIO pins into four different banks (x from GPIOx_Y)
 *    the user must count on that each bank can handle up to two interrupts asociated
 *    with it (same GPIOx_Y or different GPIOx_Y,GPIOx_Z). Therefore within this mode
 *    only 2 interrupts last for the whole system.  USE THIS MODE WITH CAUTION

 * KEYBOARD_CONFIG_SINGLE_LINE:
 *    Uses only one GPIO pin for interrupting the system when a key is pressed.
 *    Then, within the interrupt handler, it polls the 6 GPIO pins asociated with
 *    each input data to find what pin is active and so retrieve the pressed
 *    key. This mode needs an extra pin connected to a free GPIO which will be
 *    the interrupt line for the system so 7 interrupt last for the whole BeagleBone
 *    system.
 *
 */
#define IO_KEYBOARD_RESET _IO(KEYBOARD_MAGIC, KEYBOARD_RESET)	//Reset the configuration
#define IO_KEYBOARD_CONFIG_MULTI_LINE _IO(KEYBOARD_MAGIC, KEYBOARD_CONFIG_MULTI_LINE)
#define IO_KEYBOARD_CONFIG_SINGLE_LINE _IO(KEYBOARD_MAGIC, KEYBOARD_CONFIG_SINGLE_LINE)

#endif
