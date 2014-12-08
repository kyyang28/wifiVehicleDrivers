
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/device.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/pwm_camServo.h>
#include <linux/miscdevice.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include <mach/gpio.h>      /* arch/arm/plat-mxc/include/mach/gpio.h, including macro IMX_GPIO_NR and gpio_set_value etc  */

#define MARS_CAMSERVO_GPIOPWM_MAJOR                             0
#define MARS_CAMSERVO_GPIOPWM_NAME                              "mars_camServo_gpioPWM"

#define DATA_LOW_LEVEL                                          0
#define DATA_HIGH_LEVEL                                         1

#define MARS_CAMSERVO_GPIOPWM_DQ_PIN                            IMX_GPIO_NR(2,5)   // MX6Q_PAD_NANDF_D5__GPIO_2_5 (P1 pin24)

static int mars_camServo_gpioPWM_major = MARS_CAMSERVO_GPIOPWM_MAJOR;

struct mars_camServo_gpioPWM_dev {
    const char *camServo_gpioPWM_name;
    struct cdev cdev;
    struct class *camServo_gpioPWM_cls;
    struct device *camServo_gpioPWM_dev;
};

static struct mars_camServo_gpioPWM_dev *mars_camServo_gpioPWM_devp;

/* ############################################ GPIO related operations ############################################ */
static void Free_Mars_Gpio(unsigned gpio)
{
    gpio_free(gpio);
}

static int Request_Mars_Gpio(unsigned gpio, char *gpioReqName)
{
    int status = 0;

    status = gpio_request(gpio, gpioReqName);
    if (status < 0)
        pr_warning("Failed to request gpio for %s", gpioReqName);    

    return status;
}

static int Set_Mars_Gpio_Direction_Output(unsigned gpio, int level)
{
    int status = 0;

    status = gpio_direction_output(gpio, level);
    if (status < 0)
        pr_warning("Failed to setup the gpio's output direction\n");
    
    return status;
}

/* 1 = high, 0 = low */
static int Config_Mars_GPIO_Level(unsigned gpio, int level)
{
    gpio_set_value(gpio, level);
    return 0;
}
/* ##################### GPIO related operations ##################### */

static void mars_camServo_gpioPWM_free_gpio(void)
{
    Free_Mars_Gpio(MARS_CAMSERVO_GPIOPWM_DQ_PIN);
}

static void mars_camServo_gpioPWM_request_gpio(void)
{   
    Request_Mars_Gpio(MARS_CAMSERVO_GPIOPWM_DQ_PIN, "gpioPwmDQ\n");
}

static void mars_camServo_gpioPWM_gpio_dir_setup(void)
{
    Set_Mars_Gpio_Direction_Output(MARS_CAMSERVO_GPIOPWM_DQ_PIN, DATA_LOW_LEVEL);
}

static void mars_camServo_gpioPWM_gpio_data_level(unsigned gpio, int level)
{
    Config_Mars_GPIO_Level(gpio, level);
}

static void mars_camServo_gpioPWM_hw_init(void)
{
    mars_camServo_gpioPWM_free_gpio();
    mars_camServo_gpioPWM_request_gpio();
    mars_camServo_gpioPWM_gpio_dir_setup();
}

static void mars_camServo_gpioPWM_hw_exit(void)
{
    mars_camServo_gpioPWM_free_gpio();
}

static int mars_camServo_gpioPWM_open(struct inode *inode, struct file *file)
{
    /* Motors hardware related initialization */
    mars_camServo_gpioPWM_hw_init();
    return 0;
}

/* Based on the write api, the system allows to control the camServo_gpioPWM by using the duty ratio */
static ssize_t mars_camServo_gpioPWM_write(struct file *filp, const char __user *buf,
                                    size_t size, loff_t *ppos)
{
    char val;

    if (copy_from_user(&val, buf, 1))
		return -EFAULT;

    /* if the user passing 1 to the kernel, which the system will set the camServo_gpioPWM to high level, otherwise, to low level */
    if (val & 0x1) {
        mars_camServo_gpioPWM_gpio_data_level(MARS_CAMSERVO_GPIOPWM_DQ_PIN, DATA_HIGH_LEVEL);
    }else {
        mars_camServo_gpioPWM_gpio_data_level(MARS_CAMSERVO_GPIOPWM_DQ_PIN, DATA_LOW_LEVEL);
    }
    
    return 1;
}

static int mars_camServo_gpioPWM_close(struct inode *inode, struct file *file)
{
    mars_camServo_gpioPWM_hw_exit();
    return 0;
}

static struct file_operations mars_camServo_gpioPWM_fops = {
	.owner			= THIS_MODULE,
	.open			= mars_camServo_gpioPWM_open,
    .write          = mars_camServo_gpioPWM_write,
	.release		= mars_camServo_gpioPWM_close, 
};

static void mars_camServo_gpioPWM_setup_cdev(struct mars_camServo_gpioPWM_dev *dev, 
        int minor)
{
    int error;
    dev_t devno = MKDEV(mars_camServo_gpioPWM_major, minor);
    
    /* Initializing cdev */
    cdev_init(&dev->cdev, &mars_camServo_gpioPWM_fops);
    dev->cdev.owner = THIS_MODULE;

    /* Adding cdev */
    error = cdev_add(&dev->cdev, devno, 1);

    if (error) {

        printk(KERN_NOTICE "[KERNEL(%s)]Error %d adding leds", __FUNCTION__, error);
    }
}

static int __init mars_gpioPWM_camServo_init(void)
{
	int ret = 0;
    dev_t devno = MKDEV(mars_camServo_gpioPWM_major, 0);

    /* Allocating mars_camServo_gpioPWM_dev structure dynamically */
    mars_camServo_gpioPWM_devp = kmalloc(sizeof(struct mars_camServo_gpioPWM_dev), GFP_KERNEL);
    if (!mars_camServo_gpioPWM_devp) {
        return -ENOMEM;
    }

    memset(mars_camServo_gpioPWM_devp, 0, sizeof(struct mars_camServo_gpioPWM_dev));

    mars_camServo_gpioPWM_devp->camServo_gpioPWM_name = MARS_CAMSERVO_GPIOPWM_NAME;

    /* Register char devices region */
    if (mars_camServo_gpioPWM_major) {
        ret = register_chrdev_region(devno, 1, mars_camServo_gpioPWM_devp->camServo_gpioPWM_name);
    }else {
        /* Allocating major number dynamically */
        ret = alloc_chrdev_region(&devno, 0, 1, mars_camServo_gpioPWM_devp->camServo_gpioPWM_name);
        mars_camServo_gpioPWM_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    
    /* Helper function to initialize and add cdev structure */
    mars_camServo_gpioPWM_setup_cdev(mars_camServo_gpioPWM_devp, 0);

    /* mdev - automatically create the device node */
    mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls = class_create(THIS_MODULE, mars_camServo_gpioPWM_devp->camServo_gpioPWM_name);
    if (IS_ERR(mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls))
        return PTR_ERR(mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls);

    mars_camServo_gpioPWM_devp->camServo_gpioPWM_dev = device_create(mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls, NULL, devno, NULL, mars_camServo_gpioPWM_devp->camServo_gpioPWM_name);    
	if (IS_ERR(mars_camServo_gpioPWM_devp->camServo_gpioPWM_dev)) {
        class_destroy(mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls);
        cdev_del(&mars_camServo_gpioPWM_devp->cdev);
        unregister_chrdev_region(devno, 1);
        kfree(mars_camServo_gpioPWM_devp);
		return PTR_ERR(mars_camServo_gpioPWM_devp->camServo_gpioPWM_dev);
	}

	printk(MARS_CAMSERVO_GPIOPWM_NAME" is initialized!!\n");
    
    return ret;
}
module_init(mars_gpioPWM_camServo_init);

static void __exit mars_gpioPWM_camServo_exit(void)
{
    device_destroy(mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls, MKDEV(mars_camServo_gpioPWM_major, 0));
    class_destroy(mars_camServo_gpioPWM_devp->camServo_gpioPWM_cls);
    cdev_del(&mars_camServo_gpioPWM_devp->cdev);
    unregister_chrdev_region(MKDEV(mars_camServo_gpioPWM_major, 0), 1);
    kfree(mars_camServo_gpioPWM_devp);    
	printk(MARS_CAMSERVO_GPIOPWM_NAME" is left away!!\n");
}
module_exit(mars_gpioPWM_camServo_exit);

MODULE_DESCRIPTION("gpio simulated PWM camServo Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpioPWM-camServo");

