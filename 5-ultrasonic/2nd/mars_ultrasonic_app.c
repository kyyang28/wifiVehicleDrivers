
#include <stdio.h>   
#include <stdlib.h>   
#include <unistd.h>   
#include <linux/ioctl.h>   
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#define ULTRA_DEVICE_NAME               "/dev/mars_ultrasonic"
#define BUZZER_DEVICE_NAME              "/dev/mars_buzzer"
#define BUZZER                          0
#define BUZZER_HIGH                     1
#define BUZZER_LOW                      0

int main(int argc, char *argv[])
{  
    int iFd_ultra, iFd_buzzer;
    int i;
    int highlvl_duration[2];
    float gul_distance1 = 0;
    float gul_distance2 = 0;
    float Ultr_Temp1 = 0;
    float Ultr_Temp2 = 0;

    iFd_ultra = open(ULTRA_DEVICE_NAME, O_RDWR);  
    if (iFd_ultra < 0) {  
        printf("Failed to open /dev/mars_ultrasonic!\n");
        return -1;
    }

#if 1
    iFd_buzzer = open(BUZZER_DEVICE_NAME, O_RDWR);  
    if (iFd_buzzer < 0) {  
        printf("Failed to open /dev/mars_buzzer!\n");
        return -1;
    }
#endif

    memset(highlvl_duration, 0, sizeof(highlvl_duration));

    while (1) 
    {
        Ultr_Temp1 = 0;
        Ultr_Temp2 = 0;

        /* 测量5次求平均值的方法来尽量精准测量距离 */
        for (i = 0; i < 5; i++) {
            read(iFd_ultra, &highlvl_duration, sizeof(highlvl_duration));
            //printf("[%s] j = %d\n", __FUNCTION__, j);
            Ultr_Temp1 += 340 / 2 * highlvl_duration[0] * 10;                          //模块最大可测距3m
            Ultr_Temp2 += 340 / 2 * highlvl_duration[1] * 10;                          //模块最大可测距3m
            //printf("[%s]%1.4f\n", __FUNCTION__, Ultr_Temp);
        }

        gul_distance1 = Ultr_Temp1 / 5 / 1000000 * 100;               // 乘以100 为了转换为 cm, 不乘100就是米的单位
        gul_distance2 = Ultr_Temp2 / 5 / 1000000 * 100;               // 乘以100 为了转换为 cm, 不乘100就是米的单位
        printf("[%s][Ultra1] %1.4f cm\n", __FUNCTION__, gul_distance1);
        printf("[%s][Ultra2] %1.4f cm\n", __FUNCTION__, gul_distance2);

        if (gul_distance2 < 16) {
            ioctl(iFd_buzzer, BUZZER, BUZZER_HIGH);
        }else {
            ioctl(iFd_buzzer, BUZZER, BUZZER_LOW);
        }
        
        //sleep(1);
    }

    return 0;
}  

