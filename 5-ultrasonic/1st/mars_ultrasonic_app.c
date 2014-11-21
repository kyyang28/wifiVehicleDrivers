
#include <stdio.h>   
#include <stdlib.h>   
#include <unistd.h>   
#include <linux/ioctl.h>   
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEVICE_NAME             "/dev/mars_ultrasonic"

int main(int argc, char *argv[])
{  
    int iFd;
    int i, highlvl_duration = 0;
    float gul_distance = 0;
    float Ultr_Temp = 0;

    iFd = open(DEVICE_NAME, O_RDWR);  
    if (iFd < 0) {  
        perror("open device failed\n");  
        exit(1);  
    }

    while (1) 
    {  
        Ultr_Temp = 0;

        /* 测量5次求平均值的方法来尽量精准测量距离 */
        for (i = 0; i < 5; i++) {
            read(iFd, &highlvl_duration, sizeof(highlvl_duration));
            //printf("[%s] j = %d\n", __FUNCTION__, j);
            Ultr_Temp += 340 / 2 * highlvl_duration * 10;                          //模块最大可测距3m
            //printf("[%s]%1.4f\n", __FUNCTION__, Ultr_Temp);
        }

        gul_distance = Ultr_Temp / 5 / 1000000 * 100;               // 乘以100 为了转换为 cm, 不乘100就是米的单位
        printf("[%s]%1.4f cm\n", __FUNCTION__, gul_distance);

        sleep(1);
    }

    return 0;
}  

