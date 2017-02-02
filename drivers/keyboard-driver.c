#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <asm/io.h>

#include "keyboard-driver.h"
#include "keyboard-interrupt.h"

int keyboard_init(void);
void keyboard_exit(void);
ssize_t keyboard_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
long keyboard_unlocked_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
int keyboard_open(struct inode *inode, struct file *filp);
int keyboard_release(struct inode *inode, struct file *filp);

static dev_t devno;
static atomic_t tmp_atomic = ATOMIC_INIT(0);
struct keyboard_dev *dev;	//Keyboard device custom data
static struct class* keyboard_class;	//Class for /sys/

static struct file_operations keyboard_fops = {	//Struct for operations
	.owner = THIS_MODULE,
	.unlocked_ioctl = keyboard_unlocked_ioctl,
	.open = keyboard_open,
	.read = keyboard_read,
	.release = keyboard_release
};

int keyboard_init(void){
	int err, major;
	unsigned long dc;

	err = alloc_chrdev_region(&devno,0,COUNT,DEVICE_NAME);		//Obtain device number

	/* Check if device allocation is succesful */
	if (err < 0){
		printk(KERN_WARNING DEVICE_NAME ": Unable to obtain major number\n");
		return err;
	}
	major = MAJOR(devno);
	printk(KERN_INFO DEVICE_NAME ": Device successfully allocated, major number: %d \n", major);

	/* Create class for device */
	keyboard_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (keyboard_class < 0){
		unregister_chrdev_region(devno,COUNT);
		printk(KERN_ALERT DEVICE_NAME ": Unable to create class for device\n");
		return err;
	}

	printk(KERN_INFO DEVICE_NAME ": Class created\n");

	dev = kmalloc(sizeof(struct keyboard_dev), GFP_KERNEL);	//Allocate memory for the device struct, GFP_KERNEL flag for kernel context

	dc = device_create(keyboard_class, NULL, devno, NULL, DEVICE_NAME);
	if (dc < 0){
		class_destroy(keyboard_class);
        	unregister_chrdev_region(devno,COUNT);
		printk(KERN_ALERT DEVICE_NAME ": Unable to create device from class\n");
		return err;
	}

	printk(KERN_INFO DEVICE_NAME ": Device created\n");

	cdev_init(&dev->cdev, &keyboard_fops);			//Init the cdev struct contained inside dev

	/* Init the essential fields within the cdev struct */
	dev->cdev.owner = THIS_MODULE;

	/* Init the fields within the keyboard_dev struct */
	dev->configured = 0;
	dev->key = START;
	dev->readers_count = tmp_atomic;
	init_waitqueue_head(&dev->readers_queue);

	printk(KERN_INFO DEVICE_NAME ": Everything initialized \n");
	printk(KERN_INFO DEVICE_NAME ": Key value -> %c\n", dev->key+'0');

	dev->key = UNDEFINED_KEY;

	err = cdev_add(&dev->cdev,devno,1);	//Register char device into kernel
	if (err < 0){
		device_destroy(keyboard_class,devno);
		class_destroy(keyboard_class);
        	unregister_chrdev_region(devno,COUNT);
		printk(KERN_ALERT DEVICE_NAME ": Unable to register char device\n");
		return err;
	}

	return 0; 	//Success
}

int keyboard_open(struct inode *inode, struct file *filp){
	struct keyboard_dev *local_dev; /* device information */

	printk(KERN_ALERT DEVICE_NAME ": Opening\n");
	local_dev = container_of(inode->i_cdev, struct keyboard_dev, cdev);

	printk(KERN_ALERT DEVICE_NAME ": obtained struct\n");
	filp->private_data = local_dev; /* for other methods */
	/* No need to limit the open devices to one as multiple processes can read from the
	keyboard at the same time */
	printk(KERN_ALERT DEVICE_NAME ":returning\n");

	return 0;
}

ssize_t keyboard_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
	/* Blocking IO */
	struct keyboard_dev *local_dev = filp->private_data; /* device information */
	unsigned long retval;
	uint8_t pressed_key;

	if (count <= 0) return -EINVAL;	//Invalid argument
	if (!local_dev->configured) return -EFAULT;
	printk(KERN_ALERT DEVICE_NAME ":INSIDE KERNEL READING....\n");
	printk(KERN_ALERT DEVICE_NAME ": Increment readers count\n");
	/* Increment readers count */
	atomic_inc(&local_dev->readers_count);

	printk(KERN_ALERT DEVICE_NAME ": Queueing reader\n");
	/* Wait for key to be pressed through interrupt handler */
	wait_event_interruptible(local_dev->readers_queue,(atomic_read(&local_dev->readers_count) > 0 && (local_dev->key != UNDEFINED_KEY)));

	printk(KERN_ALERT DEVICE_NAME ": Awake reader \n");

	/* Decrement readers count and read key as this has been waken up */
	pressed_key = local_dev->key;
	printk(KERN_ALERT DEVICE_NAME ": HAN PULSADO LA TECLA: %c \n", pressed_key+'0');
	if (!atomic_dec_and_test (&local_dev->readers_count)){
			local_dev->key = UNDEFINED_KEY;	//All readers have already read the pressed key
	}

	/* Copy pressed key to user buffer */
	pressed_key += '0';	//ASCII code of number
	printk(KERN_ALERT DEVICE_NAME ": DESPUES DE MODIFICAR HAN PULSADO LA TECLA: %c \n", pressed_key);
	if (copy_to_user(buf, &pressed_key, sizeof(uint8_t))) retval = -EFAULT;
	else retval = NUM_CHARS_PER_KEY;

	return retval;
}

long keyboard_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	struct keyboard_dev *local_dev = filp->private_data; /* device information */
	int ret;
	printk(KERN_ALERT DEVICE_NAME ":INSIDE KERNEL CONFIGURING....\n");
	printk(KERN_ALERT DEVICE_NAME ": THE KEY IS %c \n",local_dev->key + '0');

	switch (cmd) {				//TODO Would be great if could be added command for retrieving pin config
		case IO_KEYBOARD_RESET://Reset data
			printk(KERN_ALERT DEVICE_NAME ": RESET DEVICE COMMAND RECEIVED \n");
			if (atomic_read(&local_dev->readers_count) == 0) {
				printk(KERN_ALERT DEVICE_NAME ": !!!!!!!!!!! Preset key !!!!!!! \n");
				local_dev->key = UNDEFINED_KEY;
				printk(KERN_ALERT DEVICE_NAME ": !!!!!!!!!!! Key set !!!!!!! \n");
				ret = 0;
			} else ret = -EINVAL;
			break;

		case IO_KEYBOARD_CONFIG://Configure pins, then arg is a pointer to keyboard_pins struct
			printk(KERN_ALERT DEVICE_NAME ": CONFIG DEVICE COMMAND RECEIVED \n");
			printk(KERN_ALERT DEVICE_NAME ": THE KEY IS %c \n",local_dev->key + '0');
			if (local_dev->configured & 0x1) {
				ret = -EINVAL;  //If already configured return
			} else {
				printk(KERN_ALERT DEVICE_NAME ": INITIALIZING SYSTEM.... \n");
				ret = init_system(filp->private_data);
				local_dev->configured = 0x1;
			}
			break;

		default:	/* Invalid command */
			ret = -ENOTTY;
	}

	return ret;
}


int keyboard_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT DEVICE_NAME ": CLOSING THIS DEVICE !!!!!!!!!!!!!!\n");
	return 0;
}


void keyboard_exit(void){

	/* Delete char device from kernel space */
	printk(KERN_ALERT DEVICE_NAME ": Deleting char device...\n");
	cdev_del(&dev->cdev);
	printk(KERN_ALERT DEVICE_NAME ": Char device deleted\n");

	/* Free memory used */
	printk(KERN_ALERT DEVICE_NAME ": Freeing memory...\n");
	kfree(dev);
	printk(KERN_ALERT DEVICE_NAME ": Memory freed\n");

	/* Destroy device from /sys */
	printk(KERN_ALERT DEVICE_NAME ": Destroying device from /sys/ ...\n");
	device_destroy(keyboard_class,devno);
	printk(KERN_ALERT DEVICE_NAME ": Device destroyed\n");

	/* Destroy class from /sys */
	printk(KERN_ALERT DEVICE_NAME ": Destroying class from /sys/ ...\n");
	class_destroy(keyboard_class);
	printk(KERN_ALERT DEVICE_NAME ": Class destroyed\n");

	/* Unregister device */
	printk(KERN_ALERT DEVICE_NAME ": Unregistering driver...\n");
	unregister_chrdev_region(devno,COUNT);
	printk(KERN_ALERT DEVICE_NAME ": Driver unregistered succesfully\n");

	printk(KERN_ALERT DEVICE_NAME ": EXITING MODULE NOW!\n");

	return;
}


MODULE_AUTHOR("David Nicuesa Aranda | david.nicuesa.aranda@gmail.com");
MODULE_LICENSE("GPL v2"); //Just license
MODULE_DESCRIPTION("This module is intended to include a driver for the simple keyboard on the BeagleBone");
MODULE_VERSION("1.0");

module_init(keyboard_init);	//What to do when including the module
module_exit(keyboard_exit);	//What to do when removing the module
