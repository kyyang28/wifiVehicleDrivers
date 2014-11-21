
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	arm-linux-gcc -Wall mars_leds_test.c -o mars_leds_test

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_leds_test

obj-m += mars_leds_drv.o

