obj-m+=gpiointerrupt.o  // make the change of gpiointerrupt.o with the file name you want  ,I am just this as example 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
