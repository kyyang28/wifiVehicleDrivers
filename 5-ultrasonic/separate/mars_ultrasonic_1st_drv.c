
#include <linux/kernel.h> /* printk */
#include <linux/fs.h> /* struct file, struct file_operations */
#include <linux/init.h> /* module_init, module_exit */
#include <linux/cdev.h> /* struct cdev, cdev_init, ... */
#include <linux/module.h>
#include <linux/moduleparam.h> /* module_param */
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME                         "mars_ultrasonic_1st"

#define ECHO1                               IMX_GPIO_NR(7,9)                // J10 pin32, ECHO1
#define TRIG1                               IMX_GPIO_NR(2,8)                // J10 pin34, TRIG1

#define GET_ECHO(x)                         (gpio_get_value(x))
#define SET_TRIG(x, data)                   (gpio_set_value(x, data))

#define ENABLE_ULTRA(TRIG) { \
    do { \
        SET_TRIG(TRIG, 0); \
        SET_TRIG(TRIG, 1); \
        udelay(20); \
        SET_TRIG(TRIG, 0); \
    }while (0); \
}

#define GET_HIGHLVL_DURATION(ECHO, HIGHLVL_DUR) { \
    do { \
        int timeout = 0; \
        while (GET_ECHO(ECHO) == 0); \
        while ((GET_ECHO(ECHO) != 0) && (timeout++ < 1000)) { \
            udelay(10); \
            HIGHLVL_DUR++; \
        } \
    }while (0); \
}

static int highlvl_duration = 0;

static void read_data(void)
{
    /* Ultrasonic 1 */
    ENABLE_ULTRA(TRIG1);
    //printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    GET_HIGHLVL_DURATION(ECHO1, highlvl_duration);
    //printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* Avoid the reflect signal to influence the echo signal */
    mdelay(60);
}

static ssize_t ultrasonic_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
    read_data();

	if (copy_to_user(buf, &highlvl_duration, sizeof(highlvl_duration)))
		return -EFAULT;

	return sizeof(highlvl_duration);
}

static int ultrasonic1_gpio_config(void)
{    
	gpio_free(ECHO1);
	gpio_free(TRIG1);

    if (gpio_request(ECHO1, "Ultra_echo1\n"))
        return -EBUSY;
    else
        printk("Request Ultra_echo successfully!\n");
    
    if (gpio_request(TRIG1, "Ultra_trig1\n"))
        return -EBUSY;
    else
        printk("Request Ultra_trig successfully!\n");
        
    gpio_direction_output(TRIG1, 0);
    gpio_direction_input(ECHO1);
    return 0;
}

static void ultrasonic1_gpio_free(void)
{
	gpio_free(ECHO1);
	gpio_free(TRIG1);
}

static int ultrasonic_open(struct inode *inode, struct file *file)
{
	printk("ultrasonic open\n");
    ultrasonic1_gpio_config();
	return 0;
}

static int ultrasonic_release(struct inode *inode, struct file *file)
{
	printk("ultrasonic release\n");    
    ultrasonic1_gpio_free();
	return 0; 
}

static struct file_operations ultrasonic_fops =
{
	.owner   = THIS_MODULE,
	.open    = ultrasonic_open,
	.read    = ultrasonic_read,
	.release = ultrasonic_release,
};

static struct miscdevice ultrasonic_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &ultrasonic_fops,
};


static int __init ultrasonic_init(void)
{
	int i_ret = misc_register(&ultrasonic_misc);
	if(i_ret == 0){
		printk ("ultrasonic is installed\n");
	}else{
		printk("error: Failed to install ultrasonic\n");
	}
	return i_ret;
}

static void __exit ultrasonic_exit(void)
{
	misc_deregister(&ultrasonic_misc);  
	printk("ultrasonic device is uninstalled\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);
MODULE_LICENSE("GPL");

