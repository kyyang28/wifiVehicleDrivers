
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

#define DEVICE_NAME                         "mars_ultrasonic"

#define ECHO                                IMX_GPIO_NR(7,9)                // J10 pin32, ECHO
#define TRIG                                IMX_GPIO_NR(2,8)                // J10 pin34, TRIG

#define OUTPUT_DIR                          0
#define INPUT_DIR                           1

#define GET_ECHO(x)                         (gpio_get_value(x))
#define SET_TRIG(x, data)                   (gpio_set_value(x, data))

static int read_data(void)
{
    int highlvl_duration = 0;

    SET_TRIG(TRIG, 0);
    SET_TRIG(TRIG, 1);
	udelay(20);
    SET_TRIG(TRIG, 0);
	
	while (GET_ECHO(ECHO) == 0);
	while (GET_ECHO(ECHO) != 0) {
		udelay(10);
		highlvl_duration++;
	}
    
	mdelay(60);                     //防止发射信号对回响信号的影响
    
	return highlvl_duration;
}

static ssize_t ultrasonic_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
    int highlvl_duration = 0;
	highlvl_duration = read_data();

	if (copy_to_user(buf, &highlvl_duration, sizeof(highlvl_duration)))
		return -EFAULT;

	return sizeof(highlvl_duration);
}


static int ultrasonic_open(struct inode *inode, struct file *file)
{
	printk("ultrasonic open\n");    
	gpio_free(ECHO);
	gpio_free(TRIG);

    if (gpio_request(ECHO, "Ultra_echo\n"))
        return -EBUSY;
    else
        printk("Request Ultra_echo successfully!\n");
    
    if (gpio_request(TRIG, "Ultra_trig\n"))
        return -EBUSY;
    else
        printk("Request Ultra_trig successfully!\n");
        
    gpio_direction_output(TRIG, OUTPUT_DIR);
    gpio_direction_input(ECHO);
	return 0;
}

static int ultrasonic_release(struct inode *inode, struct file *file)
{
	printk("ultrasonic release\n");    
	gpio_free(TRIG);
	gpio_free(ECHO);
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

