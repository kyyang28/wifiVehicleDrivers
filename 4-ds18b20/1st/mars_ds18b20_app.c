
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define TEMP_CONVERT_RATIO_FOR_9BIT             0.5
#define TEMP_CONVERT_RATIO_FOR_10BIT            0.25
#define TEMP_CONVERT_RATIO_FOR_11BIT            0.125
#define TEMP_CONVERT_RATIO_FOR_12BIT            0.0625

#define DS18B20_DEV                             "/dev/mars_ds18b20"

int main(int argc, char *argv[])
{
    int iFd;
#if 0    
    float decimal_value = 0;    // 温度数值,decimal_value为小数部分的值
    float temperature = 0;
    unsigned char result[2];    // 从ds18b20读出的结果，result[0]存放低八位
    unsigned char integer_value = 0;
#else    
    float temperature;
    unsigned int tmp = 0;
#endif

    /* Step 1: Open device */
    iFd = open(DS18B20_DEV, O_RDWR);
    if (iFd < 0) {
        printf("[USER]Error: Open %s is failed!\n", DS18B20_DEV);
        return -1;
    }else {
        printf("[USER]Open %s successfully!\n", DS18B20_DEV);
    }

#if 0    
    while (1) {
        read(iFd, &result, sizeof(result));
        integer_value = ((result[0] & 0xf0) >> 4) | ((result[1] & 0x07) << 4);

        // 精确到0.25度
        decimal_value = 0.5 * ((result[0] & 0x0f) >> 3) + TEMP_CONVERT_RATIO_FOR_10BIT * ((result[0] & 0x07) >> 2);
        temperature = (float)integer_value + decimal_value;
        printf("result[0] = %d\n", result[0]);
        printf("result[1] = %d\n", result[1]);
        printf("Current Temperature: %6.2f\n", temperature);

        sleep(1);
    }
#else    
    /* Step 2: Read the temperature */
    read(iFd, &tmp, sizeof(tmp));

    temperature = tmp * TEMP_CONVERT_RATIO_FOR_12BIT;
    printf("tmp: %d\n", tmp);
    printf("the current temperature is: %f\n", temperature);
#endif
    
    return 0;
}

