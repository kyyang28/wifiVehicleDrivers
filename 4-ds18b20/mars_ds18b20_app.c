
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

static void ds18b20_delay(int i)  
{
    int j, k;  
    for (j = 0; j < i; j++)  
        for (k = 0; k < 300000; k++) ;  
}

int main(int argc, char *argv[])
{
    int iFd;  
    unsigned int tmp = 0;
    float temperature = 0;  

    /* Step 1: Open device */
    iFd = open(DS18B20_DEV, O_RDWR);
    if (iFd < 0) {
        printf("[USER]Error: Open %s is failed!\n", DS18B20_DEV);
        return -1;
    }else {
        printf("[USER]Open %s successfully!\n", DS18B20_DEV);
    }

    while (1) {
        read(iFd, &tmp, sizeof(tmp));

        temperature = tmp * TEMP_CONVERT_RATIO_FOR_12BIT;
        //printf("tmp: %d\n", tmp);
        printf("the current temperature is: %1.4f\n", temperature);

        ds18b20_delay(500);
    }
        
    return 0;
}

