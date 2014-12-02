
KERN_DIR = /home/broadon/kun/projects/system/kernel/linux-3.0.15

all:
	make -C $(KERN_DIR) M=`pwd` modules

app:
	arm-linux-gcc -Wall mars_buzzer_pwm_app.c -o mars_buzzer_pwm_app

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

cleanapp:
	rm -f mars_buzzer_pwm_app

obj-m += mars_buzzer_pwm_drv.o

