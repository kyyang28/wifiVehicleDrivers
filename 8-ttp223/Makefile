
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	arm-linux-gcc -Wall mars_ttp223_app.c -o mars_ttp223_app

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_ttp223_app

obj-m += mars_ttp223_drv.o

