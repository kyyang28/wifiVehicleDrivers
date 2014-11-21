
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#define BUZZER                                  (0)

static void printUsage(char *name)
{
    printf("\n");
    printf("Usage: The correct testing format is: %s <buzzer> <on | off>\n", name);
    printf("\n");
}

int main(int argc, char *argv[])
{
    int mars_buzzer_fd;
    unsigned int buzzer;
    unsigned long status;

    mars_buzzer_fd = open("/dev/mars_buzzer", O_RDWR);
    if (mars_buzzer_fd < 0) {
        printf("Error to open /dev/mars_buzzer!\n");
        return -1;
    }

    if (argc != 3) {
        printUsage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "buzzer"))
        buzzer = BUZZER;
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

    ioctl(mars_buzzer_fd, buzzer, status);
    
    close(mars_buzzer_fd);
    return 0;
}

