
#include <stdio.h>   
#include <stdlib.h>   
#include <unistd.h>   
#include <linux/ioctl.h>   
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
//#include <pthread.h>

#define ULTRA_1ST_DEVICE_NAME           "/dev/mars_ultrasonic_1st"
#define ULTRA_2ND_DEVICE_NAME           "/dev/mars_ultrasonic_2nd"
#define BUZZER_DEVICE_NAME              "/dev/mars_buzzer"
#define BUZZER                          0
#define BUZZER_HIGH                     1
#define BUZZER_LOW                      0

static int iFd_ultra_1st, iFd_ultra_2nd, iFd_buzzer;
static int i;
static int ultra_1st_highlvl_duration, ultra_2nd_highlvl_duration;
static float gul_distance1 = 0;
static float gul_distance2 = 0;
static float Ultr_Temp1 = 0;
static float Ultr_Temp2 = 0;

#if 0
static void ultra_1st_routine(void)
{
    while (1) {
        Ultr_Temp1 = 0;

        /* 测量5次求平均值的方法来尽量精准测量距离 */
        for (i = 0; i < 5; i++) {
            read(iFd_ultra_1st, &ultra_1st_highlvl_duration, sizeof(ultra_1st_highlvl_duration));
            //printf("[%s] j = %d\n", __FUNCTION__, j);
            Ultr_Temp1 += 340 / 2 * ultra_1st_highlvl_duration * 10;                          //模块最大可测距3m
            //printf("[%s]%1.4f\n", __FUNCTION__, Ultr_Temp);
        }

        gul_distance1 = Ultr_Temp1 / 5 / 1000000 * 100;               // 乘以100 为了转换为 cm, 不乘100就是米的单位
        printf("[%s][Ultra_1st] %1.4f cm\n", __FUNCTION__, gul_distance1);
    }
}

static void ultra_2nd_routine(void)
{
    while (1) {
        Ultr_Temp2 = 0;

        /* 测量5次求平均值的方法来尽量精准测量距离 */
        for (i = 0; i < 5; i++) {
            read(iFd_ultra_2nd, &ultra_2nd_highlvl_duration, sizeof(ultra_2nd_highlvl_duration));
            //printf("[%s] j = %d\n", __FUNCTION__, j);
            Ultr_Temp2 += 340 / 2 * ultra_2nd_highlvl_duration * 10;                          //模块最大可测距3m
            //printf("[%s]%1.4f\n", __FUNCTION__, Ultr_Temp);
        }

        gul_distance2 = Ultr_Temp2 / 5 / 1000000 * 100;               // 乘以100 为了转换为 cm, 不乘100就是米的单位
        printf("[%s][Ultra_2nd] %1.4f cm\n", __FUNCTION__, gul_distance2);

        /* buzzer */
        if (gul_distance2 < 16) {
            ioctl(iFd_buzzer, BUZZER, BUZZER_HIGH);
        }else {
            ioctl(iFd_buzzer, BUZZER, BUZZER_LOW);
        }
    }
}
#endif

int main(int argc, char *argv[])
{  
#if 0
    pthread_t ultra_1st;
    pthread_t ultra_2nd;
    int ret;
#endif
    
    iFd_ultra_1st = open(ULTRA_1ST_DEVICE_NAME, O_RDWR);  
    if (iFd_ultra_1st < 0) {  
        printf("Failed to open /dev/mars_ultrasonic_1st!\n");
        return -1;
    }

    iFd_ultra_2nd = open(ULTRA_2ND_DEVICE_NAME, O_RDWR);  
    if (iFd_ultra_2nd < 0) {  
        printf("Failed to open /dev/mars_ultrasonic_2nd!\n");
        return -1;
    }

    iFd_buzzer = open(BUZZER_DEVICE_NAME, O_RDWR);  
    if (iFd_buzzer < 0) {  
        printf("Failed to open /dev/mars_buzzer!\n");
        return -1;
    }

    while (1) {
        Ultr_Temp1 = 0;
        Ultr_Temp2 = 0;

        /* 测量5次求平均值的方法来尽量精准测量距离 */
        for (i = 0; i < 5; i++) {
            read(iFd_ultra_1st, &ultra_1st_highlvl_duration, sizeof(ultra_1st_highlvl_duration));
            //printf("[%s] j = %d\n", __FUNCTION__, j);
            Ultr_Temp1 += 340 / 2 * ultra_1st_highlvl_duration * 10;                          //模块最大可测距3m
            //printf("[%s]%1.4f\n", __FUNCTION__, Ultr_Temp);
        }

        /* 测量5次求平均值的方法来尽量精准测量距离 */
        for (i = 0; i < 5; i++) {
            read(iFd_ultra_2nd, &ultra_2nd_highlvl_duration, sizeof(ultra_2nd_highlvl_duration));
            //printf("[%s] j = %d\n", __FUNCTION__, j);
            Ultr_Temp2 += 340 / 2 * ultra_2nd_highlvl_duration * 10;                          //模块最大可测距3m
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
    }

#if 0
    ret = pthread_create(&ultra_1st, NULL, (void *)ultra_1st_routine, NULL);
    if (ret!=0) {  
        printf("Create ultra_1st_routine pthread error!\n");
        return -1;
    }

    ret = pthread_create(&ultra_2nd, NULL, (void *)ultra_2nd_routine, NULL);
    if (ret!=0) {  
        printf("Create ultra_2nd_routine pthread error!\n");
        return -1;
    }
    pthread_join(ultra_1st, NULL);
    pthread_join(ultra_2nd, NULL);
#endif            
    return 0;
}  

