
KERN_DIR = /home/broadon/kun/marsboard/kernel/kernel_imx_prev

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	arm-linux-gcc -Wall mars_buttons_test.c -o mars_buttons_test

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_buttons_test

obj-m += gpio_keys.o

