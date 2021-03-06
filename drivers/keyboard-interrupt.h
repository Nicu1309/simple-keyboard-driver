#ifndef keyboard_interrupt_h
#define keyboard_interrupt_h

#include "keyboard-public.h"
#include <linux/wait.h>
#include <linux/cdev.h>

/* Bit 5: 1 - Input, 0 - Output
 * Bit 4: 1 - Pull up, 0 - Pull down
 * Bit 3: 1 - Pull disabled, 0 - Pull enabled
 * Bit 2 \
 * Bit 1 |- Mode
 * Bit 0 /
*/
#define OUTPUT_PULLUP 0x7 | (2 << 3)
#define INPUT_PULLUP 0x7 | (2 << 3) | (1 << 5)
#define OUTPUT_PULLDOWN 0x7
#define INPUT_PULLDOWN 0x7 | (1 << 5)

/* These are the PIN NUMBERS
 * If header changes, then these numbers must change to as well as the
 * offset/value arrays used in "keyboard-interrupt.c"
 *
 * They are initialized following the standard defined in "keyboard-public.h"
 */
#define GPIO_VCC 911
#define GPIO_KEY_RIGHT 912
#define GPIO_KEY_START 913
#define GPIO_KEY_UP 914
#define GPIO_KEY_DOWN 917
#define GPIO_KEY_ESCAPE 925
#define GPIO_KEY_LEFT 927
#define GPIO_POLL_IRQ 931
#define GPIO_USED_NUM 7	//Number of used gpio pins

/* Mask to get the config parameters within the keyboard_dev struct */
#define MASK_POLLABLE	0x01
#define MASK_CONFIGURED 0x02

/* Pin structure, it is shared since user could config the pins used for this
 * device and it is done by passing this structure to ioctl functions
 */
struct keyboard_pins {
  	uint8_t poll_interrupt_pin;
  	uint16_t poll_interrupt_irq;
  	uint8_t vcc_pin;
  	uint16_t right_key_irq;
  	uint8_t right_key_pin;
  	uint16_t start_key_irq;
  	uint8_t start_key_pin;
  	uint16_t up_key_irq;
  	uint8_t up_key_pin;
  	uint16_t down_key_irq;
  	uint8_t down_key_pin;
  	uint16_t escape_key_irq;
  	uint8_t escape_key_pin;
  	uint16_t left_key_irq;
  	uint8_t left_key_pin;
  };

struct keyboard_dev {
  uint8_t key;
  wait_queue_head_t readers_queue;
  struct cdev cdev;
  atomic_t readers_count;
  uint8_t is_pollable :1;		//Indicates if get data comes from polling or interrupt (b0)
  uint8_t configured	:1;		//Indicates if already configured (b1)
  struct keyboard_pins pins;
};

int init_system(struct keyboard_dev *);
int shutdown_system(void);
int populate_config(struct pin_conf *user_conf);

#endif
