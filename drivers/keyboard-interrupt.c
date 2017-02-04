#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include "keyboard-driver.h"
#include "keyboard-interrupt.h"

#define AM33XX_CONTROL_BASE 0x44e10000

static struct keyboard_dev *dev;
static uint8_t pollable_bak = 0;	//Used to avoid bad intented behaviour from user

/* Structure used for custom configuration of gpio pins
 * It is initialized as default values but will be changed if the user wants to
 * by passing another structure from this type to ioctl system call
 */
static struct pin_conf pin_config = {
	.irq_pin = GPIO_POLL_IRQ,
	.vcc_pin = GPIO_VCC,
	.right_key_pin = GPIO_KEY_RIGHT,
	.start_key_pin = GPIO_KEY_START,
	.up_key_pin = GPIO_KEY_UP,
	.down_key_pin = GPIO_KEY_DOWN,
	.escape_key_pin = GPIO_KEY_ESCAPE,
	.left_key_pin = GPIO_KEY_LEFT,
};

/* These are the arrays containing offsets and value for the GPIO pines on the
 * BeagleBone.
 *
 * FOR P8_X:
 * 	Note that the first element and the second one are for DGND so they
 * 	cannot be mapped neither have value.
 *
 * FOR P9_X:
 * 	Note that the first 10 elements are for voltage output and ground. In addition
 *	this port has more pins since P9_41 and P9_42 have two modes each one (A and B)
 *	so this extra offsets and values have been appended at the end of the usual
 * 	pins. This follows the notation for pins published within "keyboard-public.h"
 */
static const uint32_t pins8_offset[46] = {0,0,0x818,0x81C,0x808,0x80C,0x890,0x894,
				0x89C,0x898,0x834,0x830,0x824,0x828,0x83C,0x838,0x82C,0x88C,0x820,0x884,
				0x880,0x814,0x810,0x804,0x800,0x87C,0x8E0,0x8E8,0x8E4,0x8EC,0x8D8,0x8DC,
				0x8D4,0x8CC,0x8D0,0x8C8,0x8C0,0x8C4,0x8B8,0x8BC,0x8B0,0x8B4,0x8A8,0x8AC,
				0x8A0,0x8A4
};
static const uint32_t pins8_value[46] = {0,0,38,39,34,35,66,67,69,68,45,44,23,26,
				47,46,27,65,22,63,62,37,36,33,32,61,86,88,87,89,10,11,9,81,8,80,78,79,76,
				77,74,75,72,73,70,71
};
static const uint32_t pins9_offset[48] = {0,0,0,0,0,0,0,0,0,0,0x870,0x878,0x874,
				0x848,0x840,0x84C,0x95C,0x958,0x97C,0x978,0x954,0x950,0x844,0x984,0x9AC,
				0x980,0x9A4,0x99C,0x994,0x998,0x990,0,0,0,0,0,0,0,0,0,0x9B4,0x964,0,0,0,
				0,0x9A8,0x9A0
};
static const uint32_t pins9_value[48] = {0,0,0,0,0,0,0,0,0,0,30,60,31,50,48,51,5,
				4,13,12,3,2,49,15,117,14,115,113,111,112,110,0,0,0,0,0,0,0,0,0,20,7,0,0,
				0,0,116,114
};

static int setup_pinmux(struct keyboard_pins *pins);
static int request_pins(struct keyboard_pins *pins);
static int request_irq_poll_pin(struct keyboard_pins *pins);
static int request_irq_poll_interrupt(struct keyboard_pins *pins);
static int request_interrupts(struct keyboard_pins *pins);
static void release_interrupts(struct keyboard_pins *pins);
static int request_pins(struct keyboard_pins *pins);
static void release_pins(struct keyboard_pins *pins);
static void translate_gpio_num(uint16_t gpio_num, uint32_t *val_ptr, uint32_t *off_ptr);

irqreturn_t polling_interrupt_handler(int irq, void* dev_id);
irqreturn_t start_key_interrupt_handler(int irq, void* dev_id);
irqreturn_t escape_key_interrupt_handler(int irq, void* dev_id);
irqreturn_t up_key_interrupt_handler(int irq, void* dev_id);
irqreturn_t down_key_interrupt_handler(int irq, void* dev_id);
irqreturn_t right_key_interrupt_handler(int irq, void* dev_id);
irqreturn_t left_key_interrupt_handler(int irq, void* dev_id);



/**
 *		IRQ HANDLERS
 */

irqreturn_t polling_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	uint8_t pressed = UNDEFINED_KEY;
	printk(KERN_ALERT DEVICE_NAME ": POLLING... \n");

	/* Poll pins to get pressed key */
	if (gpio_get_value(data->pins.right_key_pin)) pressed = RIGHT;
	else if (gpio_get_value(data->pins.start_key_pin)) pressed = START;
	else if (gpio_get_value(data->pins.up_key_pin)) pressed = UP;
	else if (gpio_get_value(data->pins.down_key_pin)) pressed = DOWN;
	else if (gpio_get_value(data->pins.left_key_pin)) pressed = LEFT;
	else if (gpio_get_value(data->pins.escape_key_pin)) pressed = ESCAPE;
	data->key = pressed;

	/* ACK irq */
	gpio_set_value(data->pins.poll_interrupt_pin, 0);

	/* Wake up readers */
	wake_up_interruptible(&data->readers_queue);

	return IRQ_HANDLED;
}

irqreturn_t right_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	printk(KERN_ALERT DEVICE_NAME ": RIGHT_KEY PRESSED \n");
	data->key=RIGHT;
	gpio_set_value(data->pins.right_key_pin, 0);
	wake_up_interruptible(&data->readers_queue);

	return IRQ_HANDLED;
}

irqreturn_t start_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	printk(KERN_ALERT DEVICE_NAME ": START_KEY PRESSED \n");
	data->key=START;
	gpio_set_value(data->pins.start_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t up_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	printk(KERN_ALERT DEVICE_NAME ": UP_KEY PRESSED \n");
	data->key=UP;
	gpio_set_value(data->pins.up_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t down_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	printk(KERN_ALERT DEVICE_NAME ": DOWN_KEY PRESSED \n");
	data->key=DOWN;
	gpio_set_value(data->pins.down_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t escape_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	printk(KERN_ALERT DEVICE_NAME ": ESCAPE_KEY PRESSED \n");
	data->key=ESCAPE;
	gpio_set_value(data->pins.escape_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t left_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	printk(KERN_ALERT DEVICE_NAME ": LEFT_KEY PRESSED \n");
	data->key=LEFT;
	gpio_set_value(data->pins.left_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}


/*
 *		CONFIG GLOBAL FUNCTIONS
 */
int init_system(struct keyboard_dev *device){
	int err;
	printk(KERN_ALERT DEVICE_NAME ": INTERRUPT.C \n");
	printk(KERN_ALERT DEVICE_NAME ": STORING DEV POINTER  \n");
	dev = device; //Store pointer to device struct

	/* Store if is in pollable mode */
	pollable_bak = device->is_pollable;
	printk(KERN_ALERT DEVICE_NAME ": IS_POLLABLE? -> %d\n",pollable_bak);

	printk(KERN_ALERT DEVICE_NAME ": SETTING UP PINMUX.. \n");
 	err = setup_pinmux(&device->pins);
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to apply pinmux settings.\n");
	  goto err_return;
	}

	printk(KERN_ALERT DEVICE_NAME ": REQUESTING PINS... \n");
	err = request_pins(&device->pins);
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIOS.\n");
	  goto err_return;
	}

	/* IF IT IS CONFIGURED AS POLLABLE BY INTERRUPT, THEN AN EXTRA PIN IS
	 * NEEDED TO DO SO
	 */
	if (pollable_bak) {
		/* Request pin for irq */
		err = request_irq_poll_pin(&device->pins);
		if (err < 0) {
			printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO for interrupt.\n");
			goto err_return;
		}
		/* Request IRQ */
		err = request_irq_poll_interrupt(&device->pins);
	} else {
		/* Device configured as multiple interrupt lines so request all interrupts */
		err = request_interrupts(&device->pins);
	}

	err_return:
		return err;
}

int shutdown_system(void){
	release_interrupts(&dev->pins);
	release_pins(&dev->pins);
	return 0;
}

int populate_config(struct pin_conf *user_conf){
	pin_config.irq_pin = user_conf->irq_pin;
	pin_config.vcc_pin = user_conf->vcc_pin;
	pin_config.right_key_pin = user_conf->right_key_pin;
	pin_config.start_key_pin = user_conf->start_key_pin;
	pin_config.up_key_pin = user_conf->up_key_pin;
	pin_config.down_key_pin = user_conf->down_key_pin;
	pin_config.escape_key_pin = user_conf->escape_key_pin;
	pin_config.left_key_pin = user_conf->left_key_pin;
	return 0;
}


/*
 *		CONFIG LOCAL FUNCTIONS
 */
static int setup_pinmux(struct keyboard_pins *k_pins){
	int i;
	void* addr;
	uint32_t pins[] = { 					// Memory mapped gpios in AM335x
																// calculated as (AM33XX_CONTROL_BASE + offset) ref man Table 9-10,
																// upon pin names names: from table/mode0 in BB ref manual
		0,
		OUTPUT_PULLUP,                					//	mode 7 (gpio), PULLUP, OUTPUT
		0,
		INPUT_PULLDOWN,     						//      mode 7 (gpio), PULLDOWN, INPUT
		0,
		INPUT_PULLDOWN,  						//      mode 7 (gpio), PULLDOWN, INPUT
		0,
		INPUT_PULLDOWN,  						//      mode 7 (gpio), PULLDOWN, INPUT
		0,
		INPUT_PULLDOWN,  						//      mode 7 (gpio), PULLDOWN, INPUT
		0,
		INPUT_PULLDOWN,  						//      mode 7 (gpio), PULLDOWN, INPUT
		0,
		INPUT_PULLDOWN,							//      mode 7 (gpio), PULLDOWN, INPUT
	};
	printk(KERN_ALERT DEVICE_NAME ": DONE WITH PIN ARRAY \n");
	printk(KERN_ALERT DEVICE_NAME ": POPULATE ARRAY WITH PIN NUMBERS \n");

	/* This populates both mmap&configuration array (pins) and the real on board
	 * values for the gpios used with this driver.
	 */
	translate_gpio_num(pin_config.vcc_pin, (uint32_t *)&k_pins->vcc_pin, &pins[0]); // VCC Pin -> Will serve as power for keyboard
	translate_gpio_num(pin_config.right_key_pin, (uint32_t *)&k_pins->right_key_pin, &pins[2]); // RIGHT_KEY Pin ->    will be interrupt
	translate_gpio_num(pin_config.start_key_pin, (uint32_t *)&k_pins->start_key_pin, &pins[4]); // START_KEY Pin ->    will be interrupt
	translate_gpio_num(pin_config.up_key_pin, (uint32_t *)&k_pins->up_key_pin, &pins[6]); // UP_KEY Pin ->    will be interrupt
	translate_gpio_num(pin_config.down_key_pin, (uint32_t *)&k_pins->down_key_pin, &pins[8]); // DOWN_KEY Pin ->    will be interrupt
	translate_gpio_num(pin_config.escape_key_pin, (uint32_t *)&k_pins->escape_key_pin, &pins[10]); // ESCAPE_KEY Pin ->    will be interrupt
	translate_gpio_num(pin_config.left_key_pin, (uint32_t *)&k_pins->left_key_pin, &pins[12]); // LEFT_KEY Pin ->    will be interrupt

	printk(KERN_ALERT DEVICE_NAME ": DONE WITH POPULATING PIN NUMBERS \n");
	printk(KERN_ALERT DEVICE_NAME ": STARTING POPULATING PIN FIELDS WITHIN PIN STRUCT \n");

	for (i=0; i<GPIO_USED_NUM*2; i+=2) {	// map the mapped i/o addresses to kernel high memory
		addr = ioremap(pins[i], 4);
		printk(KERN_ALERT DEVICE_NAME ": GPIO NUMBER %d\n",pins[i]);
		if (NULL == addr)
			return -EBUSY;

		iowrite32(pins[i+1], addr); // write settings for each pin
		iounmap(addr);
	}

	/* IF DEVICE IS CONFIGURED AS POLLABLE, THEN IT IS NEEDED TO CONFIGURE THE
	* IRQ PIN
	*/
	if (pollable_bak){
		uint32_t gpio;
		translate_gpio_num(pin_config.irq_pin, (uint32_t *)&k_pins->poll_interrupt_pin, &gpio); // IRQ_POLL Pin ->    will be interrupt
		addr = ioremap(gpio, 4);
		printk(KERN_ALERT DEVICE_NAME ": GPIO NUMBER %d\n",gpio);
		if (NULL == addr)
			return -EBUSY;

		iowrite32(INPUT_PULLDOWN, addr); // write settings for pin
		iounmap(addr);
	}

	return 0;
}

static int request_irq_poll_pin(struct keyboard_pins *pins){
	int err;
	err = gpio_request_one(pins->poll_interrupt_pin, GPIOF_IN, DEVICE_NAME " gpio_poll_irq");
	if (err < 0) {
		printk(KERN_ALERT DEVICE_NAME " : failed to request IRQ_POLL pin %d.\n",
		 pins->poll_interrupt_pin);
		return err;
	}
	printk(KERN_ALERT DEVICE_NAME " : REQUESTED IRQ_POLL pin %d.\n",
	 pins->poll_interrupt_pin);
	return 0;
}

static int request_irq_poll_interrupt(struct keyboard_pins *pins){
	int err,irq_num;
	/* POLLING INTERRUPT */
	printk(KERN_ALERT DEVICE_NAME ": REQUEST IRQ FOR POLLING \n");
	irq_num = gpio_to_irq(pins->poll_interrupt_pin);	//Request interrupt number
	printk(KERN_ALERT DEVICE_NAME ": IRQ NUMBER POLL IRQ PIN -> %d  \n", irq_num);
	if (irq_num < 0) {
		printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_POLL_IRQ pin %d.\n",
		 pins->poll_interrupt_pin);
		err = irq_num;
		goto err_return;
	}
	pins->poll_interrupt_irq = irq_num;							//Match interrupt to handler
	printk(KERN_ALERT DEVICE_NAME ": OBTAINED IRQ FOR POLLING \n");
	printk(KERN_ALERT DEVICE_NAME ": REQUEST CONTEXT IRQ FOR POLLING \n");
	err = request_any_context_irq(
			irq_num,
			polling_interrupt_handler,
			IRQF_TRIGGER_RISING | IRQF_DISABLED,
			DEVICE_NAME,
			(void*)dev
		);
	if (err < 0) {
			printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
				 irq_num, pins->poll_interrupt_pin);
			goto err_return_free_irq;
	}
	printk(KERN_ALERT DEVICE_NAME ": CONFIGURED IRQ FOR POLLING \n");
	return 0;
	err_return_free_irq:
		disable_irq(pins->poll_interrupt_irq);
	err_return:
		return err;
}

static int request_interrupts(struct keyboard_pins *pins){
	int err,irq_num;

	/* RIGHT KEY INTERRUPT */
	printk(KERN_ALERT DEVICE_NAME ": REQUEST IRQ \n");
	irq_num = gpio_to_irq(pins->right_key_pin);	//Request interrupt number
	printk(KERN_ALERT DEVICE_NAME ": IRQ NUMBER RIGHT PIN -> %d  \n", irq_num);
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_RIGHT_KEY pin %d.\n",
		 pins->right_key_pin);
	  err = irq_num;
	  goto err_return_irq;
	}
	pins->right_key_irq = irq_num;							//Match interrupt to handler
	printk(KERN_ALERT DEVICE_NAME ": REQUEST CONTEXT IRQ \n");
	err = request_any_context_irq(
      irq_num,
      right_key_interrupt_handler,
      IRQF_TRIGGER_RISING | IRQF_DISABLED,
      DEVICE_NAME,
      (void*)dev
   	);
	if (err < 0) {
      printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
         irq_num, pins->right_key_pin);
      goto err_return_irq;
	}

	/* START KEY INTERRUPT */
	irq_num = gpio_to_irq(pins->start_key_pin);	//Request interrupt number
	if (irq_num < 0) {
		printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_START_KEY pin %d.\n",
		 pins->start_key_pin);
		err = irq_num;
		goto err_return_irq_free_right;
	}
	pins->start_key_irq = irq_num;				//Match interrupt to handler
	err = request_any_context_irq(
			irq_num,
			start_key_interrupt_handler,
			IRQF_TRIGGER_RISING | IRQF_DISABLED,
			DEVICE_NAME,
			(void*)dev
		);
	if (err < 0) {
			printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
				 irq_num, pins->start_key_pin);
			goto err_return_irq_free_right;
	}

	/* UP KEY INTERRUPT */
	irq_num = gpio_to_irq(pins->up_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_UP_KEY pin %d.\n",
		 pins->up_key_pin);
	  err = irq_num;
	  goto err_return_irq_free_start;
	}
	pins->up_key_irq = irq_num;				//Match interrupt to handler
	err = request_any_context_irq(
      irq_num,
      up_key_interrupt_handler,
      IRQF_TRIGGER_RISING | IRQF_DISABLED,
      DEVICE_NAME,
      (void*)dev
   	);
	if (err < 0) {
      printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
         irq_num, pins->up_key_pin);
      goto err_return_irq_free_start;
	}

	/* DOWN KEY INTERRUPT */
	irq_num = gpio_to_irq(pins->down_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_DOWN_KEY pin %d.\n",
		 pins->down_key_pin);
	  err = irq_num;
	  goto err_return_irq_free_up;
	}
	pins->down_key_irq = irq_num;								//Match interrupt to handler
	err = request_any_context_irq(
      irq_num,
      down_key_interrupt_handler,
      IRQF_TRIGGER_RISING | IRQF_DISABLED,
      DEVICE_NAME,
      (void*)dev
   	);
	if (err < 0) {
      printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
         irq_num, pins->down_key_pin);
      goto err_return_irq_free_up;
	}

	/* ESCAPE KEY INTERRUPT */
	irq_num = gpio_to_irq(pins->escape_key_pin);	//Request interrupt number
	if (irq_num < 0) {
		printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_ESCAPE_KEY pin %d.\n",
		 pins->escape_key_pin);
		err = irq_num;
		goto err_return_irq_free_down;
	}
	pins->escape_key_irq = irq_num;						//Match interrupt to handler
	err = request_any_context_irq(
			irq_num,
			escape_key_interrupt_handler,
			IRQF_TRIGGER_RISING | IRQF_DISABLED,
			DEVICE_NAME,
			(void*)dev
		);
	if (err < 0) {
			printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
				 irq_num, pins->escape_key_pin);
			goto err_return_irq_free_down;
	}

	/* LEFT KEY INTERRUPT */
	irq_num = gpio_to_irq(pins->left_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_LEFT_KEY pin %d.\n",
		 pins->left_key_pin);
	  err = irq_num;
	  goto err_return_irq_free_escape;
	}
	pins->left_key_irq = irq_num;									//Match interrupt to handler
	err = request_any_context_irq(
      irq_num,
      left_key_interrupt_handler,
      IRQF_TRIGGER_RISING | IRQF_DISABLED,
      DEVICE_NAME,
      (void*)dev
   	);
	if (err < 0) {
      printk(KERN_ALERT DEVICE_NAME " : failed to enable IRQ %d for pin %d.\n",
         irq_num, pins->left_key_pin);
      goto err_return_irq_free_escape;
	}

	return 0;

	err_return_irq_free_escape:
		disable_irq(pins->escape_key_irq);
		free_irq(pins->escape_key_irq, (void*)dev);
	err_return_irq_free_down:
		disable_irq(pins->down_key_irq);
		free_irq(pins->down_key_irq, (void*)dev);
	err_return_irq_free_up:
		disable_irq(pins->up_key_irq);
		free_irq(pins->up_key_irq, (void*)dev);
	err_return_irq_free_start:
		disable_irq(pins->start_key_irq);
		free_irq(pins->start_key_irq, (void*)dev);
	err_return_irq_free_right:
		disable_irq(pins->right_key_irq);
		free_irq(pins->right_key_irq, (void*)dev);
	err_return_irq:
		release_pins(pins);
		return err;
}

static void release_pins(struct keyboard_pins *pins){
	gpio_free(pins->vcc_pin);
	gpio_free(pins->escape_key_pin);
	gpio_free(pins->left_key_pin);
	gpio_free(pins->down_key_pin);
	gpio_free(pins->up_key_pin);
	gpio_free(pins->start_key_pin);
	gpio_free(pins->right_key_pin);
	if (pollable_bak) gpio_free(pins->poll_interrupt_pin);
}

static void release_interrupts(struct keyboard_pins *pins){
	/* If the device is configured as pollable, then there is only one interrupt */
	if (pollable_bak) {
		disable_irq(pins->poll_interrupt_irq);
		free_irq(pins->poll_interrupt_irq, (void*)dev);
	} else {
		/* START KEY DISABLE IRQ */
		disable_irq(pins->start_key_irq);
		free_irq(pins->start_key_irq, (void*)dev);

		/* RIGHT KEY DISABLE IRQ */
		disable_irq(pins->right_key_irq);
		free_irq(pins->right_key_irq, (void*)dev);

		/* UP KEY DISABLE IRQ */
		disable_irq(pins->up_key_irq);
		free_irq(pins->up_key_irq, (void*)dev);

		/* DOWN KEY DISABLE IRQ */
		disable_irq(pins->down_key_irq);
		free_irq(pins->down_key_irq, (void*)dev);

		/* ESC KEY DISABLE IRQ */
		disable_irq(pins->escape_key_irq);
		free_irq(pins->escape_key_irq, (void*)dev);

		/* LEFT KEY DISABLE IRQ */
		disable_irq(pins->left_key_irq);
		free_irq(pins->left_key_irq, (void*)dev);
	}

}

static int request_pins(struct keyboard_pins *pins){
	int err;

	printk(KERN_ALERT DEVICE_NAME ": REQUEST VCC PIN \n");
	/* Request VCC pin */
	err = gpio_request_one(pins->vcc_pin, GPIOF_OUT_INIT_HIGH,
	  DEVICE_NAME " gpio_vcc");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_VCC pin %d.\n",
		 pins->vcc_pin);
	  goto err_return;
	}

	/* Request RIGHT pin */
	printk(KERN_ALERT DEVICE_NAME ": REQUEST RIGHT PIN \n");
	err = gpio_request_one(pins->right_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_right");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_RIGHT_KEY pin %d.\n",
		 pins->right_key_pin);
	  goto err_return_free_vcc;
	}

	/* Request START pin */
	err = gpio_request_one(pins->start_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_start");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_START_KEY pin %d.\n",
		 pins->start_key_pin);
	  goto err_return_free_right;
	}

	/* Request UP pin */
	err = gpio_request_one(pins->up_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_up");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_UP_KEY pin %d.\n",
		 pins->up_key_pin);
	  goto err_return_free_start;
	}

	/* Request DOWN pin */
	err = gpio_request_one(pins->down_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_down");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_DOWN_KEY pin %d.\n",
		 pins->down_key_pin);
	  goto err_return_free_up;
	}

	/* Request ESCAPE pin */
	err = gpio_request_one(pins->escape_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_escape");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_ESCAPE_KEY pin %d.\n",
		 pins->escape_key_pin);
	  goto err_return_free_down;
	}

	/* Request LEFT pin */
	err = gpio_request_one(pins->left_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_left");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_LEFT_KEY pin %d.\n",
		 pins->left_key_pin);
	  goto err_return_free_escape;
	}

	return 0;		//Success

	err_return_free_escape:
		gpio_free(pins->escape_key_pin);
	err_return_free_down:
		gpio_free(pins->down_key_pin);
	err_return_free_up:
		gpio_free(pins->up_key_pin);
	err_return_free_start:
		gpio_free(pins->start_key_pin);
	err_return_free_right:
		gpio_free(pins->right_key_pin);
	err_return_free_vcc:
		gpio_free(pins->vcc_pin);
	err_return:
		return err;
}

static void translate_gpio_num(uint16_t gpio_num, uint32_t *val_ptr, uint32_t *mmap_ptr){
	int num = (gpio_num % 100) - 1;
	// pin is in P9
	if (gpio_num >= 900) {
		// Check gpio_num range
		if((num) >= 0 && (num) <= 47) {
			*mmap_ptr = AM33XX_CONTROL_BASE + pins9_offset[num];
			*val_ptr = pins9_value[num];
			return;
		}
	}
	// pin is in P8
	else if(gpio_num >= 800) {
		if((num) >= 0 && (num) <= 45) {
			*mmap_ptr = AM33XX_CONTROL_BASE + pins8_offset[num];
			*val_ptr = pins8_value[num];
			return;
		}
	}
	*mmap_ptr = 0;
	*val_ptr = 0;
	return;
	}
