
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	arm-linux-gcc -Wall mars_lcd1602_8wire_test.c -o mars_lcd1602_8wire_test

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_lcd1602_8wire_test

obj-m += mars_lcd1602_8wire_drv.o

