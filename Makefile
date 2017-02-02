# export the environment variables CC and KDIR properly set
obj-m := simple_keyboard.o

# set KDIR to your kernel source root dir
KDIR=$(SE2)/TP6/Trabajo_Propio/kernel-rt/kernel

simple_keyboard-objs := keyboard-driver.o keyboard-interrupt.o

default:
	make ARCH=arm CROSS_COMPILE=${CC} -C ${KDIR} M=$(PWD) modules
