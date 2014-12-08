
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	arm-linux-gcc -Wall mars_ultrasonic_1st_2nd_app.c -o mars_ultrasonic_1st_2nd_app #-lpthread

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_ultrasonic_1st_2nd_app

obj-m += mars_ultrasonic_1st_drv.o
obj-m += mars_ultrasonic_2nd_drv.o

