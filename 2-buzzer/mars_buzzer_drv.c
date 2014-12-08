
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

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include <mach/gpio.h>      /* arch/arm/plat-mxc/include/mach/gpio.h, including macro IMX_GPIO_NR and gpio_set_value etc  */

struct buzzer_dev {
    struct cdev cdev;
    unsigned int status;
};

MODULE_AUTHOR("Charles Yang <charlesyang28@gmail.com>");
MODULE_LICENSE("GPL");

static int mars_buzzer_major = 0;
static int mars_buzzer_minor = 0;

static struct buzzer_dev *mars_buzzer_dev = NULL;
static struct class *mars_buzzer_cls = NULL;

static void mars_buzzer_drv_cleanup_module(void);

#define MARS_BUZZER_GPIO6_11_PIN                IMX_GPIO_NR(6,11)                 // NANDF_CS0

#define BUZZER                                (0)

static void Free_Mars_Buzzer_Gpio(unsigned gpio)
{    
    gpio_free(gpio);
}

static int Request_Mars_Buzzer_Gpio(unsigned gpio, char *gpioReqName)
{
    int status = 0;

    status = gpio_request(gpio, gpioReqName);
    if (status < 0)
        pr_warning("Failed to request gpio for %s", gpioReqName);    

    return status;
}

/* whichWay = 0 means output, otherwise 1 means input */
static int Set_Mars_Buzzer_Gpio_Direction(unsigned gpio, int whichWay)
{
    int status = 0;

    if (0 == whichWay) {    
        status = gpio_direction_output(gpio, whichWay);
        if (status < 0)
            pr_warning("Failed to setup the gpio's output direction\n");
    }else {
        status = gpio_direction_input(gpio);
        if (status < 0)
            pr_warning("Failed to setup the gpio's input direction\n");
    }
    
    return status;
}

static int mars_buzzer_init(void)
{
    int status;

    Free_Mars_Buzzer_Gpio(MARS_BUZZER_GPIO6_11_PIN);

    status = Request_Mars_Buzzer_Gpio(MARS_BUZZER_GPIO6_11_PIN, "buzzer\n");

    status = Set_Mars_Buzzer_Gpio_Direction(MARS_BUZZER_GPIO6_11_PIN, 0);     // 0 = output
    
    return status;
}

/* 1 = high, 0 = low */
static int Config_Mars_Buzzer_GPIO_Level(unsigned gpio, int level)
{
    gpio_set_value(gpio, level);
    return 0;
}

static void mars_buzzer_ctl(unsigned long status)
{
    if (status) {
        Config_Mars_Buzzer_GPIO_Level(MARS_BUZZER_GPIO6_11_PIN, 1);
    }else { 
        Config_Mars_Buzzer_GPIO_Level(MARS_BUZZER_GPIO6_11_PIN, 0);
    }
}

static int mars_buzzer_open(struct inode *inode, struct file *filp)
{
    struct buzzer_dev *tmp;

    tmp = container_of(inode->i_cdev, struct buzzer_dev, cdev);
    filp->private_data = tmp;    
    return 0;
}

static long mars_buzzer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case BUZZER:
        mars_buzzer_dev->status = arg;
        mars_buzzer_ctl(mars_buzzer_dev->status);
        break;

    default:
        return -ENODEV;
    }
    
    return 0;
}

static int mars_buzzer_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations mars_buzzer_fops = {
    .owner              = THIS_MODULE,
    .open               = mars_buzzer_open,
    .unlocked_ioctl     = mars_buzzer_ioctl,
    .release            = mars_buzzer_release,
};
    
static void mars_setup_cdev(struct cdev *buzzer_cdev)
{
    int res, dev = MKDEV(mars_buzzer_major, mars_buzzer_minor);
    cdev_init(buzzer_cdev, &mars_buzzer_fops);
    buzzer_cdev->owner    = THIS_MODULE;
    buzzer_cdev->ops      = &mars_buzzer_fops;
    res = cdev_add(buzzer_cdev, dev, 1);
    if (res)
		printk(KERN_NOTICE "Error %d adding mars_buzzer\n", res);
}

static int __init mars_buzzer_drv_init(void)
{
    int res;
    dev_t dev;

    /* Step 1: Acquiring the device major */
    if (mars_buzzer_major) {
        dev = MKDEV(mars_buzzer_major, mars_buzzer_minor);
        res = register_chrdev_region(dev, 1, "mars_buzzer");
    }else {
        res = alloc_chrdev_region(&dev, mars_buzzer_minor, 1, "mars_buzzer");
        mars_buzzer_major = MAJOR(dev);
    }

    if (res < 0) {
        printk(KERN_WARNING "mars_buzzer: cannot get major %d\n", mars_buzzer_major);
        return res;
    }
    
    /* Step 2: Setup cdev */
    mars_buzzer_dev = kzalloc(sizeof(struct buzzer_dev), GFP_KERNEL);
    if (!mars_buzzer_dev) {
        res = -ENOMEM;
        goto fail;
    }
    
    mars_setup_cdev(&mars_buzzer_dev->cdev);

    /* Step 3: Create class */
    mars_buzzer_cls = class_create(THIS_MODULE, "mars_buzzer");
    device_create(mars_buzzer_cls, NULL, dev, NULL, "mars_buzzer");

    /* Step 4: Hardware related setup */
    mars_buzzer_init();

    printk(KERN_NOTICE "+------ %s is invoked successfully! ------+\n", __FUNCTION__);
    return 0;
    
fail:
    mars_buzzer_drv_cleanup_module();
    return res;
    
}
module_init(mars_buzzer_drv_init);

static void __exit mars_buzzer_drv_exit(void)
{
    mars_buzzer_drv_cleanup_module();
    printk(KERN_NOTICE "+------ %s is invoked successfully! ------+\n", __FUNCTION__);
}
module_exit(mars_buzzer_drv_exit);

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
static void mars_buzzer_drv_cleanup_module(void)
{
	dev_t devno = MKDEV(mars_buzzer_major, mars_buzzer_minor);

    if (mars_buzzer_cls) {
        device_destroy(mars_buzzer_cls, MKDEV(mars_buzzer_major, mars_buzzer_minor));
        class_destroy(mars_buzzer_cls);
    }
    
	/* Get rid of our char dev entries */
	if (mars_buzzer_dev) {
		cdev_del(&mars_buzzer_dev->cdev);
		kfree(mars_buzzer_dev);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1); 
}

