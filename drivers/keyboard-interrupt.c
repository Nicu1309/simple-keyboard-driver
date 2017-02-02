#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include "keyboard-interrupt.h"
#include "keyboard-driver.h"

static struct keyboard_dev *dev;
#define AM33XX_CONTROL_BASE 0x44e10000

static const uint32_t pins8_offset[47] = {0,0,0x818,0x81C,0x808,0x80C,0,0,0,0,0x834,0x830,0,0x828,0x83C,0x838,0x82C,0x88C,0,0x884,0x880,0x814,0x810,0x804,0x800,0x87C,0x8E0,0x8E8,0x8E4,0x8EC,0,0,0,0,0,0,0,0,0x8B8,0x8BC,0x8B4,0x8B0,0x8A8,0x8AC,0x8A0,0x8A4};

static const uint32_t pins8_value[47] = {0,0,38,39,34,35,0,0,0,0,45,44,0,26,47,46,27,65,0,63,62,37,36,33,32,61,86,88,87,89,0,0,0,0,0,0,0,0,76,77,74,75,72,73,70,71};

static const uint32_t pins9_offset[47] = {0,0,0,0,0,0,0,0,0,0,0x870,0x878,0x874,0x848,0,0,0x95c,0,0,0,0,0,0,0,0x9AC,0,0x9A4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const uint32_t pins9_value[47] = {0,0,0,0,0,0,0,0,0,0,30,60,31,50,0,0,5,0,0,0,0,0,0,0,117,0,115,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};



irqreturn_t right_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	data->key=RIGHT;
	gpio_set_value(data->pins->right_key_pin, 0);
	wake_up_interruptible(&data->readers_queue);

	return IRQ_HANDLED;
}

irqreturn_t start_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	data->key=START;
	gpio_set_value(data->pins->start_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t up_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	data->key=UP;
	gpio_set_value(data->pins->up_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t down_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	data->key=DOWN;
	gpio_set_value(data->pins->down_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t escape_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	data->key=ESCAPE;
	gpio_set_value(data->pins->escape_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

irqreturn_t left_key_interrupt_handler(int irq, void* dev_id){
	struct keyboard_dev *data = (struct keyboard_dev*)dev_id;
	data->key=LEFT;
	gpio_set_value(data->pins->left_key_pin, 0);
   	wake_up_interruptible(&data->readers_queue);
	return IRQ_HANDLED;
}

int setup_pinmux(struct keyboard_pins *k_pins)
{
   int i;
   static u32 pins[] = { 					// offsets in AM335x ref man Table 9-10, upon pin names
			 					// pin names: from table/mode0 in BB ref manual
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

	/* This population must be done this way as C does not take them as constants */
	pins[0] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_VCC]);   			// VCC Pin -> Will serve as power for keyboard
	pins[2] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_KEY_RIGHT]);	   	// RIGHT_KEY Pin ->    will be interrupt
	pins[4] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_KEY_START]);   	// START_KEY Pin ->    will be interrupt
	pins[6] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_KEY_UP]);		  	// UP_KEY Pin ->    will be interrupt
	pins[8] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_KEY_DOWN]);	   	// DOWN_KEY Pin ->    will be interrupt
	pins[10] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_KEY_ESCAPE]);   	// ESCAPE_KEY Pin ->    will be interrupt
	pins[12] = (AM33XX_CONTROL_BASE + pins9_offset[GPIO_KEY_LEFT]);   	// LEFT_KEY Pin ->    will be interrupt


   for (i=0; i<GPIO_USED_NUM*2; i+=2) {	// map the mapped i/o addresses to kernel high memory
      void* addr = ioremap(pins[i], 4);

      if (NULL == addr)
         return -EBUSY;

      iowrite32(pins[i+1], addr); // write settings for each pin
      iounmap(addr);
   }

	/* Populate pin fields */
	k_pins->vcc_pin = pins9_value[GPIO_VCC];
	k_pins->right_key_pin = pins9_value[GPIO_KEY_RIGHT];
	k_pins->start_key_pin = pins9_value[GPIO_KEY_START];
	k_pins->up_key_pin = pins9_value[GPIO_KEY_UP];
	k_pins->down_key_pin = pins9_value[GPIO_KEY_DOWN];
	k_pins->escape_key_pin = pins9_value[GPIO_KEY_ESCAPE];
	k_pins->left_key_pin = pins9_value[GPIO_KEY_LEFT];

   return 0;
}

int init_system(struct keyboard_dev *device){
	int err;
	printk(KERN_ALERT DEVICE_NAME ": INTERRUPT.C \n");
	printk(KERN_ALERT DEVICE_NAME ": STORING DEV POINTER  \n");
	dev = device; //Store pointer to device struct

	printk(KERN_ALERT DEVICE_NAME ": SETTING UP PINMUX.. \n");
 	err = setup_pinmux(dev->pins);
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to apply pinmux settings.\n");
	  goto err_return;
	}

	printk(KERN_ALERT DEVICE_NAME ": REQUESTING PINS... \n");
	err = request_pins(dev->pins);
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIOS.\n");
	  goto err_return;
	}

err_return:
	return err;
}

int shutdown_system(void){
	/* VCC PIN */
	gpio_free(dev->pins->vcc_pin);

	/* RIGHT KEY PIN */
	disable_irq(dev->pins->right_key_irq);
	free_irq(dev->pins->right_key_irq, (void*)dev);
	gpio_free(dev->pins->right_key_pin);

	/*START KEY PIN*/
	disable_irq(dev->pins->start_key_irq);
	free_irq(dev->pins->start_key_irq,(void*)dev);
	gpio_free(dev->pins->start_key_pin);

	/*UP KEY PIN*/
	disable_irq(dev->pins->up_key_irq);
	free_irq(dev->pins->up_key_irq,(void*)dev);
	gpio_free(dev->pins->up_key_pin);

	/*DOWN KEY PIN*/
	disable_irq(dev->pins->down_key_irq);
	free_irq(dev->pins->down_key_irq,(void*)dev);
	gpio_free(dev->pins->down_key_pin);

	/*ESCAPE KEY PIN*/
	disable_irq(dev->pins->escape_key_irq);
	free_irq(dev->pins->escape_key_irq,(void*)dev);
	gpio_free(dev->pins->escape_key_pin);

	/*LEFT KEY PIN*/
	disable_irq(dev->pins->left_key_irq);
	free_irq(dev->pins->left_key_irq,(void*)dev);
	gpio_free(dev->pins->left_key_pin);

	return 0;

}

int request_pins(struct keyboard_pins *pins){
	int err,irq_num;

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
	printk(KERN_ALERT DEVICE_NAME ": REQUEST IRQ \n");
	irq_num = gpio_to_irq(pins->right_key_pin);	//Request interrupt number
	printk(KERN_ALERT DEVICE_NAME ": IRQ NUMBER RIGHT PIN -> %d  \n", irq_num);
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_RIGHT_KEY pin %d.\n",
		 pins->right_key_pin);
	  err = irq_num;
	  goto err_return_free_vcc;
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
      goto err_return_free_vcc;
	}

	/* Request START pin */
	err = gpio_request_one(pins->start_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_start");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_START_KEY pin %d.\n",
		 pins->start_key_pin);
	  goto err_return_free_right;
	}
	irq_num = gpio_to_irq(pins->start_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_START_KEY pin %d.\n",
		 pins->start_key_pin);
	  err = irq_num;
	  goto err_return_free_right;
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
      goto err_return_free_right;
	}

	/* Request UP pin */
	err = gpio_request_one(pins->up_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_up");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_UP_KEY pin %d.\n",
		 pins->up_key_pin);
	  goto err_return_free_start;
	}
	irq_num = gpio_to_irq(pins->up_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_UP_KEY pin %d.\n",
		 pins->up_key_pin);
	  err = irq_num;
	  goto err_return_free_start;
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
      goto err_return_free_start;
	}

	/* Request DOWN pin */
	err = gpio_request_one(pins->down_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_down");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_DOWN_KEY pin %d.\n",
		 pins->down_key_pin);
	  goto err_return_free_up;
	}
	irq_num = gpio_to_irq(pins->down_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_DOWN_KEY pin %d.\n",
		 pins->down_key_pin);
	  err = irq_num;
	  goto err_return_free_up;
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
      goto err_return_free_up;
	}


	/* Request ESCAPE pin */
	err = gpio_request_one(pins->escape_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_escape");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_ESCAPE_KEY pin %d.\n",
		 pins->escape_key_pin);
	  goto err_return_free_down;
	}
	irq_num = gpio_to_irq(pins->escape_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_ESCAPE_KEY pin %d.\n",
		 pins->escape_key_pin);
	  err = irq_num;
	  goto err_return_free_down;
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
      goto err_return_free_down;
	}

	/* Request LEFT pin */
	err = gpio_request_one(pins->left_key_pin, GPIOF_IN, DEVICE_NAME " gpio_key_left");
	if (err < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request GPIO_LEFT_KEY pin %d.\n",
		 pins->left_key_pin);
	  goto err_return_free_escape;
	}
	irq_num = gpio_to_irq(pins->left_key_pin);	//Request interrupt number
	if (irq_num < 0) {
	  printk(KERN_ALERT DEVICE_NAME " : failed to request interrupt for GPIO_LEFT_KEY pin %d.\n",
		 pins->left_key_pin);
	  err = irq_num;
	  goto err_return_free_escape;
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
