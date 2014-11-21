
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

/* 
 * To manually control the leds' on and off, 
 * simply comment the #undef MANUAL line
 */
#define MANUAL
//#undef MANUAL

#define USER_1                                  (0)
#define USER_2                                  (1)

#define DELAY_PERIOD                            (0x2000000)

#ifdef MANUAL
static void printUsage(char *name)
{
    printf("\n");
    printf("Usage: The correct testing format is: %s <usr1 | usr2> <on | off>\n", name);
    printf("\n");
}
#endif

#ifndef MANUAL
static void delay(int tick)
{
    int i;
    for (i = 0; i < tick; i++);
}
#endif

int main(int argc, char *argv[])
{
    int mars_leds_fd;
    unsigned int ledno;
    unsigned long status;

    mars_leds_fd = open("/dev/mars_leds", O_RDWR);
    if (mars_leds_fd < 0) {
        printf("Error to open /dev/mars_leds!\n");
        return -1;
    }

#ifdef MANUAL
    if (argc != 3) {
        printUsage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "usr1"))        // D24
        ledno = USER_1;
    else if (!strcmp(argv[1], "usr2"))   // D25
        ledno = USER_2;
    else {
        printUsage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[2], "on"))
        status = 1;
    else if (!strcmp(argv[2], "off"))
        status = 0;
    else {
        printUsage(argv[0]);
        return -1;    
    }

    ioctl(mars_leds_fd, ledno, status);
#else
    while (1) {
        /* usr1 led on */        
        ledno = 0;
        status = 0;
        ioctl(mars_leds_fd, ledno, status);     // led usr1
        
        delay(DELAY_PERIOD);

        /* usr1 led off */
        status = 1;
        ioctl(mars_leds_fd, ledno, status);     // led usr1

        delay(DELAY_PERIOD);

        /* usr2 led on */        
        ledno = 1;
        status = 0;
        ioctl(mars_leds_fd, ledno, status);     // led usr2
        
        delay(DELAY_PERIOD);
        
        /* usr2 led off */
        status = 1;
        ioctl(mars_leds_fd, ledno, status);     // led usr2
        
        delay(DELAY_PERIOD);
    }
#endif
    
    close(mars_leds_fd);
    return 0;
}

