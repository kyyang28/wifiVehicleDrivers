
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

manual_app:
	arm-linux-gcc -Wall mars_camServo_horizontal_pwm_manual_app.c -o mars_camServo_horizontal_pwm_manual_app

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

clean_manualapp:
	rm -f mars_camServo_horizontal_pwm_manual_app

obj-m += mars_camServo_horizontal_pwm_drv.o

