
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#define CAMLEDS                                 (0)

static void printUsage(char *name)
{
    printf("\n");
    printf("Usage: The correct testing format is: %s <camleds> <on | off>\n", name);
    printf("\n");
}

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

    if (argc != 3) {
        printUsage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "camleds"))
        ledno = CAMLEDS;
    else {
        printUsage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[2], "on"))
        status = 0;
    else if (!strcmp(argv[2], "off"))
        status = 1;
    else {
        printUsage(argv[0]);
        return -1;    
    }

    ioctl(mars_leds_fd, ledno, status);
    
    close(mars_leds_fd);
    return 0;
}

