
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	gcc -Wall mars_motor_app.c -o mars_motor_app -lncurses

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_motor_app

obj-m += mars_motor_drv.o

