
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LCD1602_WRITE_CMD                                   (0)
#define LCD1602_WRITE_DATA                                  (1)

int main(int argc, char *argv[])
{
    int mars_lcd1602_fd;

    mars_lcd1602_fd = open("/dev/mars_lcd1602", O_RDWR);
    if (mars_lcd1602_fd < 0) {
        printf("Error to open /dev/mars_lcd1602!\n");
        return -1;
    }

    //while (1) 
    {
        //LcdCommandWrite(0x01);  // 屏幕清空，光标位置归零
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_CMD, 0x01);
        usleep(10);

#if 1
        //LcdCommandWrite(0x80+3);
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_CMD, 0x80+2);      // Cursor at first line, pos 4
        usleep(10);

        //LcdDataWrite('Y');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'I');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, ' ');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'a');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'm');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, ' ');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'C');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'h');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'a');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'r');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'l');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 'e');
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, 's');
        sleep(2);
#endif        
    }

    while (1);

    close(mars_lcd1602_fd);
    return 0;
}

