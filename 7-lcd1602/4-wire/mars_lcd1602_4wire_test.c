
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LCD1602_WRITE_CMD                                   (0)
#define LCD1602_WRITE_DATA                                  (1)
//#define LCD1602_WRITE_CHAR                                  (3)
//#define LCD1602_WRITE_STRING                                (4)

static char str1[] = "Welcome to";
static char str2[] = "yang-workshop";
static char str3[] = "this is the";
static char str4[] = "4-bit interface";

static int mars_lcd1602_fd;

#if 0
static void lcd1602_delay(int i)  
{
    int j, k;  
    for (j = 0; j < i; j++)  
        for (k = 0; k < 100000; k++);  
}
#endif

static void mars_lcd1602_set_xy(int x, int y)
{
    int addr;

    if (y == 0)
        addr = 0x80 + x;        // line 1, pos x
    else
        addr = 0xC0 + x;        // line 2, pos x

    ioctl(mars_lcd1602_fd, LCD1602_WRITE_CMD, addr);
}

#if 0
static void mars_lcd1602_write_char(int x, int y, int dat)
{
    mars_lcd1602_set_xy(x, y);
    ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, dat);
}
#endif

static void mars_lcd1602_write_string(int x, int y, char *s)
{
    mars_lcd1602_set_xy(x, y);      // setup the address
    while (*s) {
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_DATA, *s);
        s++;
    }
}

int main(int argc, char *argv[])
{
    mars_lcd1602_fd = open("/dev/mars_lcd1602", O_RDWR);
    if (mars_lcd1602_fd < 0) {
        printf("Error to open /dev/mars_lcd1602!\n");
        return -1;
    }
    
    //while (1) 
    {
#if 1        
        ioctl(mars_lcd1602_fd, LCD1602_WRITE_CMD, 0x01);        // Clear screen
        usleep(50);

        mars_lcd1602_write_string(3, 0, str1);
        usleep(50);

        mars_lcd1602_write_string(1, 1, str2);
        usleep(5000);

        ioctl(mars_lcd1602_fd, LCD1602_WRITE_CMD, 0x01);        // Clear screen
        usleep(50);
        
        mars_lcd1602_write_string(0, 0, str3);
        usleep(50);

        mars_lcd1602_write_string(0, 1, str4);
        usleep(5000);
#else 
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
#endif
    }

    while (1);

    close(mars_lcd1602_fd);
    return 0;
}

