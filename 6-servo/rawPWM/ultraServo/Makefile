
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

manual_app:
	arm-linux-gcc -Wall mars_ultraServo_pwm_manual_app.c -o mars_ultraServo_pwm_manual_app

auto_app:
	arm-linux-gcc -Wall mars_ultraServo_pwm_auto_app.c -o mars_ultraServo_pwm_auto_app

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

clean_manualapp:
	rm -f mars_ultraServo_pwm_manual_app mars_ultraServo_pwm_auto_app

clean_autoapp:
	rm -f mars_ultraServo_pwm_auto_app

obj-m += mars_ultraServo_pwm_drv.o

