
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MARS_PWM_IOCTL_SET_FREQ		                1
#define MARS_PWM_IOCTL_STOP			                0

#define MARS_PWM_ULTRASERVO_DUTY_TIME_70K           700000            // 180°
#define MARS_PWM_ULTRASERVO_DUTY_TIME_100K          1000000           // 135°
#define MARS_PWM_ULTRASERVO_DUTY_TIME_148K          1480000           // 90°      
#define MARS_PWM_ULTRASERVO_DUTY_TIME_180K          1800000           // 45°
#define MARS_PWM_ULTRASERVO_DUTY_TIME_240K          2400000           // 0°

static int iFdServo = -1;
static int iFdUltra = -1;
static int iFdBuzzer = -1;
static void open_mars_pwm_ultraServo(void)
{
	iFdServo = open("/dev/mars_ultraServo_pwm", O_RDWR);
	if (iFdServo < 0) {
		perror("open mars_pwm_ultraServo device");
		exit(1);
	}
    
	iFdUltra = open("/dev/mars_ultrasonic", O_RDWR);
	if (iFdUltra < 0) {
		perror("open mars_ultrasonic device");
		exit(1);
	}
    
	iFdBuzzer = open("/dev/mars_buzzer", O_RDWR);
	if (iFdBuzzer < 0) {
		perror("open mars_buzzer device");
		exit(1);
	}
}

static void set_mars_pwm_ultraServo_freq(int freq)
{
	// this IOCTL command is the key to set frequency
	int ret = ioctl(iFdServo, MARS_PWM_IOCTL_SET_FREQ, freq);
	if(ret < 0) {
		perror("set the frequency of the ultraServo");
		exit(1);
	}
}

static void stop_mars_pwm_ultraServo(void)
{
	// this IOCTL command is the key to set frequency
	int ret = ioctl(iFdServo, MARS_PWM_IOCTL_STOP);
	if(ret < 0) {
		perror("Failed to stop the ultraServo");
		exit(1);
	}
}

static float getUltraDistance(void)
{
    int i;
    int highlvl_duration[2];
    float Ultr_Temp1 = 0;
    float distance = 0;
    
    memset(highlvl_duration, 0, sizeof(highlvl_duration));
        
    Ultr_Temp1 = 0;
    /* 测量5次求平均值的方法来尽量精准测量距离 */
    for (i = 0; i < 5; i++) {
        read(iFdUltra, &highlvl_duration, sizeof(highlvl_duration));
        //printf("[%s] j = %d\n", __FUNCTION__, j);
        Ultr_Temp1 += 340 / 2 * highlvl_duration[0] * 10;                          //模块最大可测距3m
        //printf("[%s]%1.4f\n", __FUNCTION__, Ultr_Temp);
    }
    
    distance = Ultr_Temp1 / 5 / 1000000 * 100;               // 乘以100 为了转换为 cm, 不乘100就是米的单位
    //printf("[%s][Ultra1] %1.4f cm\n", __FUNCTION__, gul_distance1);
    return distance;
}
    
int main(int argc, char **argv)
{
    float gul_distance1 = 0;
    
	open_mars_pwm_ultraServo();

    set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_148K);       // 90°

    /* Right now the servo is 90 degree */
    while (1) {
        /* Move the vehicle forward */
        /* Detect the front distance */
        /* If distance is less than 35cm
         *   - stop the vehicle.
         * else
         *   - 
         */
        /* Detect the left distance */
        /* If the distance is less than 35cm, stop the vehicle */
        /* Detect the right distance */        
        /* If the distance is less than 35cm, stop the vehicle */
        /* Buzzer on */

        printf("[START]Wifi vehicle is moving forward!\n");

        gul_distance1 = getUltraDistance();
        if (gul_distance1 < 35) {
            printf("The distance is less than 35cm!\n");
            printf("Wifi vehicle is stopped!\n");

            set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_240K);       // 0°
            gul_distance1 = getUltraDistance();
            if (gul_distance1 < 35) {
                printf("The distance is less than 35cm!\n");
                printf("Wifi vehicle is stopped!\n");

                set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_70K);   // 180°
                gul_distance1 = getUltraDistance();
                if (gul_distance1 < 35) {
                    printf("The distance is less than 35cm!\n");
                    printf("Wifi vehicle is stopped!\n");
                    break;
                }else {
                    printf("Wifi vehicle turned right!\n");
                    printf("Wifi vehicle is moving forward!\n");                        
                }
            }else {
                printf("Wifi vehicle turned left!\n");
                printf("Wifi vehicle is moving forward!\n");
            }
        }else {
            printf("The distance is greater than 35cm!\n");
            printf("Wifi vehicle is moving forward!\n");
        }
    }

#if 0
    /* Right now the servo is 90 degree */
    while (1) {
        /** Detect the front distance.
         *  if the distance is less than 35cm
         *      - stop the vehicle
         *      - turn the servo left by 45 degree(now at 45 degree) and detect the distance again
         *      - if the distance is greater than 35cm
         *          @ move the vehicle forward
         *        else
         *          @ stop the vehicle
         *          @ turn the servo left by 45 degree(now at 0 degree) and detect the distance again
         *              # if the distance is greater than 35cm
         *                  - turn the vehicle left
         *                  - move the vehicle forward
         *                else
         *                  - stop the vehicle
         *                  - turn the servo right by 135 degree(now at 135 degree) and detect the distance again
         *                      # if the distance is greater than 35cm
         *                          - turn the vehicle right
         *                          - move the vehicle forward
         *                        else
         *                          - stop the vehicle
         *                          - turn the servo right by 45 degree(now at 180 degree) and detect the distance again
         *                              # if the distance is greater than 35cm
         *                                  - turn the vehicle right
         *                                  - move the vehicle forward
         *                                else
         *                                  - stop the vehicle
         *  else
         *      - move the vehicle forward
         */
        printf("[START]Wifi vehicle is moving forward!\n");
        
        gul_distance1 = getUltraDistance();
        if (gul_distance1 < 35) {
            printf("The distance is less than 35cm!\n");
            printf("Wifi vehicle is stopped!\n");
            set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_180K);           // 45°
            printf("Ultrasonic is now at 45 degree!\n");

            gul_distance1 = getUltraDistance();
            if (gul_distance1 < 35) {
                printf("Wifi vehicle is stopped!\n");
                set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_240K);       // 0°
                printf("Ultrasonic is now at 0 degree!\n");

                gul_distance1 = getUltraDistance();
                if (gul_distance1 < 35) {
                    printf("Wifi vehicle is stopped!\n");
                    set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_100K);   // 135°
                    printf("Ultrasonic is now at 135 degree!\n");

                    gul_distance1 = getUltraDistance();
                    if (gul_distance1 < 35) {
                        printf("Wifi vehicle is stopped!\n");
                        set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_70K);   // 180°
                        printf("Ultrasonic is now at 180 degree!\n");
                    }else {
                        printf("The distance is greater than 35cm!\n");
                        printf("Wifi vehicle is moving forward!\n");
                    }
                }else {
                    printf("The distance is greater than 35cm!\n");
                    printf("Wifi vehicle is moving forward!\n");
                }
            }else {
                printf("The distance is greater than 35cm!\n");
                printf("Wifi vehicle is moving forward!\n");
            }

        }else {
            printf("The distance is greater than 35cm!\n");
            printf("Wifi vehicle is moving forward!\n");
        }
    }
#endif
    
    stop_mars_pwm_ultraServo();

    return 0;
}
