
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int mars_ttp223_fd;
    unsigned int mars_ttp223_val = 0;

    mars_ttp223_fd = open("/dev/mars_ttp223", O_RDWR);
    if (mars_ttp223_fd < 0) {
        printf("Error to open /dev/mars_ttp223!\n");
        return -1;
    }

    while (1) {
        read(mars_ttp223_fd, &mars_ttp223_val, sizeof(mars_ttp223_val));
        printf("mars_ttp223_val = %u\n", mars_ttp223_val);
        sleep(1);
    }
    
    close(mars_ttp223_fd);
    return 0;
}

