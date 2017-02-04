#ifndef keyboard_driver_h
#define keyboard_driver_h

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
;

#endif
