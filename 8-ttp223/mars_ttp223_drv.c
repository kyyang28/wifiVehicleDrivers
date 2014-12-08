
#include <linux/module.h>  
#include <linux/kernel.h>
#include <linux/init.h> 
#include <linux/fs.h>   
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>             /* kmalloc/kfree */
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>  
#include <asm/irq.h>

MODULE_LICENSE("GPL");

#define MARS_TTP223_MAJOR                    0
#define MARS_TTP223_NAME                     "mars_ttp223"

/* ttp223 */
#define MARS_TTP223_NANDF_D3_GPIO_2_3_PIN                     IMX_GPIO_NR(2,3)  // Mars J10 Pin20 TTP223

static int mars_ttp223_major = MARS_TTP223_MAJOR;

struct mars_ttp223_dev {
    const char *ttp223_name;
    struct cdev cdev;
    struct class *ttp223_cls;
    struct device *ttp223_dev;
};

static struct mars_ttp223_dev *mars_ttp223_devp;

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

static int Set_Mars_Gpio_Direction_Input(unsigned gpio)
{
    int status = 0;

    status = gpio_direction_input(gpio);
    if (status < 0)
        pr_warning("Failed to setup the gpio's input direction\n");
    
    return status;
}

static int Acquire_Mars_GPIO_Level(unsigned gpio)
{
    return gpio_get_value(gpio);
}
/* ##################### GPIO related operations ##################### */

/* ################################## TTP223 Ops ################################## */
static void mars_ttp223_free_gpio(unsigned gpio)
{
    Free_Mars_Gpio(gpio);
}

static void mars_ttp223_request_gpio(unsigned gpio, char *gpioReqName)
{
    Request_Mars_Gpio(gpio, gpioReqName);
}

static void mars_ttp223_gpio_dir_setup(unsigned gpio)
{
    Set_Mars_Gpio_Direction_Input(gpio);
}

static int mars_ttp223_read_gpio_level(unsigned gpio)
{
    return Acquire_Mars_GPIO_Level(gpio);
}
/* ################################## TTP223 Ops ################################## */

static void mars_ttp223_hw_init(void)
{
    mars_ttp223_free_gpio(MARS_TTP223_NANDF_D3_GPIO_2_3_PIN);
    mars_ttp223_request_gpio(MARS_TTP223_NANDF_D3_GPIO_2_3_PIN, "ttp223\n");
    mars_ttp223_gpio_dir_setup(MARS_TTP223_NANDF_D3_GPIO_2_3_PIN);
}

static void mars_ttp223_hw_exit(void)
{
    mars_ttp223_free_gpio(MARS_TTP223_NANDF_D3_GPIO_2_3_PIN);
}

/* ################################## ttp223 ops ################################## */
static int mars_ttp223_open(struct inode *inode, struct file *filp)
{
    /* Motors hardware related initialization */
    mars_ttp223_hw_init();
    return 0;
}

static ssize_t mars_ttp223_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned int ttp223_val = 0;
    
    ttp223_val = mars_ttp223_read_gpio_level(MARS_TTP223_NANDF_D3_GPIO_2_3_PIN);
    
    if (copy_to_user(buf, &ttp223_val, sizeof(ttp223_val)))
        return -EFAULT;
    
    return sizeof(ttp223_val);    
}

static int mars_ttp223_close(struct inode *inode, struct file *filp)
{
    mars_ttp223_hw_exit();
    return 0;
}

static struct file_operations mars_ttp223_fops = {
    .owner              = THIS_MODULE,
    .open               = mars_ttp223_open,
    .read               = mars_ttp223_read,
    .release            = mars_ttp223_close,
};

static void mars_ttp223_setup_cdev(struct mars_ttp223_dev *dev, 
        int minor)
{
    int error;
    dev_t devno = MKDEV(mars_ttp223_major, minor);
    
    /* Initializing cdev */
    cdev_init(&dev->cdev, &mars_ttp223_fops);
    dev->cdev.owner = THIS_MODULE;

    /* Adding cdev */
    error = cdev_add(&dev->cdev, devno, 1);

    if (error)
        printk(KERN_NOTICE "[KERNEL(%s)]Error %d adding leds", __FUNCTION__, error);
}

static int __init ttp223_drv_init(void)
{
	int ret = 0;
    dev_t devno = MKDEV(mars_ttp223_major, 0);

    /* Allocating mars_ttp223_dev structure dynamically */
    mars_ttp223_devp = kmalloc(sizeof(struct mars_ttp223_dev), GFP_KERNEL);
    if (!mars_ttp223_devp) {
        return -ENOMEM;
    }

    memset(mars_ttp223_devp, 0, sizeof(struct mars_ttp223_dev));

    mars_ttp223_devp->ttp223_name = MARS_TTP223_NAME;

    /* Register char devices region */
    if (mars_ttp223_major) {
        ret = register_chrdev_region(devno, 1, mars_ttp223_devp->ttp223_name);
    }else {
        /* Allocating major number dynamically */
        ret = alloc_chrdev_region(&devno, 0, 1, mars_ttp223_devp->ttp223_name);
        mars_ttp223_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    
    /* Helper function to initialize and add cdev structure */
    mars_ttp223_setup_cdev(mars_ttp223_devp, 0);

    /* mdev - automatically create the device node */
    mars_ttp223_devp->ttp223_cls = class_create(THIS_MODULE, mars_ttp223_devp->ttp223_name);
    if (IS_ERR(mars_ttp223_devp->ttp223_cls))
        return PTR_ERR(mars_ttp223_devp->ttp223_cls);

    mars_ttp223_devp->ttp223_dev = device_create(mars_ttp223_devp->ttp223_cls, NULL, devno, NULL, mars_ttp223_devp->ttp223_name);    
	if (IS_ERR(mars_ttp223_devp->ttp223_dev)) {
        class_destroy(mars_ttp223_devp->ttp223_cls);
        cdev_del(&mars_ttp223_devp->cdev);
        unregister_chrdev_region(devno, 1);
        kfree(mars_ttp223_devp);
		return PTR_ERR(mars_ttp223_devp->ttp223_dev);
	}
    
	printk(MARS_TTP223_NAME" is initialized!!\n");
    
    return ret;
}
module_init(ttp223_drv_init);

static void __exit ttp223_drv_exit(void)
{
    device_destroy(mars_ttp223_devp->ttp223_cls, MKDEV(mars_ttp223_major, 0));
    class_destroy(mars_ttp223_devp->ttp223_cls);
    cdev_del(&mars_ttp223_devp->cdev);
    unregister_chrdev_region(MKDEV(mars_ttp223_major, 0), 1);
    kfree(mars_ttp223_devp);    
	printk(MARS_TTP223_NAME" is left away!!\n");
}
module_exit(ttp223_drv_exit);

