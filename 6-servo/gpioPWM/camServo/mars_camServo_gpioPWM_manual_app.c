
#include <stdio.h>
#include <unistd.h>             // usleep()
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>

static int iCamServoFd = -1;

static int open_mars_camServo_gpioPWM(void)
{
	iCamServoFd = open("/dev/mars_camServo_gpioPWM", O_RDWR);
	if (iCamServoFd < 0) {
		printf("Failed to open mars_camServo_gpioPWM device!\n");
        return -1;
	}
    return 0;
}

#if 1
static void printUsage(char *name)
{
    printf("Usage: %s <dutyCycle>\n", name);
    printf("\t dutyCycle = 1ms or 1.5ms or 2ms or 2.5ms\n");
}
#endif

int main(int argc, char **argv)
{
    int iRet = 0;
    int dutyCycle;
    char val;
    
    if (argc != 2) {
        printUsage(argv[0]);
        return -1;
    }

    dutyCycle = atoi(argv[1]);

    iRet = open_mars_camServo_gpioPWM();
    if (iRet < 0)
        return -1;
    
    val = 1;            // high level
    write(iCamServoFd, &val, 1);
    usleep(dutyCycle*1000);             // 1ms
    
    val = 0;            // low level
    write(iCamServoFd, &val, 1);
    usleep(20*1000 - dutyCycle*1000);
    
#if 0
    /* Change the usleep value to manually control the duty cycle */
    while (1) {
        val = 1;
        write(iCamServoFd, &val, 1);

        usleep(dutyCycle*1000);

        val = 0;
        write(iCamServoFd, &val, 1);
        usleep(100*1000 - dutyCycle*1000);
    }
#endif
    
    return 0;
}

