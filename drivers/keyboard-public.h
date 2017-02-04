#ifndef keyboard_public_h
#define keyboard_public_h

#include <linux/ioctl.h>

/* Config structure, it is shared since user could config the pins used for this
 * device and it is done by passing this structure to ioctl functions.
 *
 * The pins inside the structure are 3 digits numbers with the following format:
 *   pin = XYZ   where
 *
 * X  -> represents the gpio pins slot (9 or 8)
 * YZ -> Is the gpio physical number (from 1 to 46)
 *
 */
struct pin_conf {
    uint16_t irq_pin;
  	uint16_t vcc_pin;
  	uint16_t right_key_pin;
  	uint16_t start_key_pin;
  	uint16_t up_key_pin;
  	uint16_t down_key_pin;
  	uint16_t escape_key_pin;
  	uint16_t left_key_pin;
  };


#define KEYBOARD_RESET 0
#define KEYBOARD_CONFIG_MULTI_LINE 1
#define KEYBOARD_CONFIG_SINGLE_LINE 2
#define KEYBOARD_CONFIG_PINMUX 3

#define KEYBOARD_MAGIC (0xDA) //Magic number 0xDA is unused in this kernel currently


/* Define commnands to configure the input data mode
 *
 * IO_KEYBOARD_RESET:
 *    Deletes previous configuration, after using this command, the device must
 *    be configured again using the desired commands.
 *
 * KEYBOARD_CONFIG_MULTI_LINE:
 *    Uses one irq for each input line (GPIO), this is 6 irqs to handle each line separately.
 *    Since the BeagleBone splits its GPIO pins into four different banks (x from GPIOx_Y)
 *    the user must count on that each bank can handle up to two interrupts asociated
 *    with it. Therefore within this mode only 2 interrupts last for the whole
 *    system.  USE THIS MODE WITH CAUTION
 *
 * KEYBOARD_CONFIG_SINGLE_LINE:
 *    Uses only one GPIO pin for interrupting the system when a key is pressed.
 *    Then, within the interrupt handler, it polls the 6 GPIO pins asociated with
 *    each input data to find what pin is active and so retrieve the pressed
 *    key. This mode needs an extra pin connected to a free GPIO which will be
 *    the interrupt line for the system so 7 interrupt last for the whole BeagleBone
 *    system.
 *
 KEYBOARD_CONFIG_PINMUX:
 *    Configures the pins used by this devices so the user can pass a structure containing
 *    the pins tha are going to be used. This does not check for irq mode
 *    (SINGLE_LINE || MULTIPLE LINE) so the user must have this on mind and assign
 *    the correct pin configuration, this is:
 *       1 -> not overloading gpio banks with interrupts (2 per bank at most)
 *       2 -> Assign usable gpio pins for this device
 */
#define IO_KEYBOARD_RESET _IO(KEYBOARD_MAGIC, KEYBOARD_RESET)	//Reset the configuration
#define IO_KEYBOARD_CONFIG_MULTI_LINE _IO(KEYBOARD_MAGIC, KEYBOARD_CONFIG_MULTI_LINE)
#define IO_KEYBOARD_CONFIG_SINGLE_LINE _IO(KEYBOARD_MAGIC, KEYBOARD_CONFIG_SINGLE_LINE)
#define IO_KEYBOARD_CONFIG_PINMUX _IOW(KEYBOARD_MAGIC, KEYBOARD_CONFIG_PINMUX, struct pin_conf)

#endif
