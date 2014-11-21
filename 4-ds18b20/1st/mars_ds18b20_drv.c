
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

#define MARS_DS18B20_MAJOR                  0
#define MARS_DS18B20_NAME                   "mars_ds18b20"

#define OUTPUT_DIR                          0
#define INPUT_DIR                           1
#define DATA_LOW_LEVEL                      0
#define DATA_HIGH_LEVEL                     1

static int mars_ds18b20_major = MARS_DS18B20_MAJOR;
//static unsigned char temp_data[2];

struct mars_ds18b20_dev {
    const char *ds18b20_name;
    struct cdev cdev;
    struct class *ds18b20_cls;
    struct device *ds18b20_dev;
};

static struct mars_ds18b20_dev *mars_ds18b20_devp;

#define MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN                     IMX_GPIO_NR(6,8)                // NANDF_ALE

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

/* whichWay = 0 means output, otherwise 1 means input */
static int Set_Mars_Gpio_Direction(unsigned gpio, int whichWay)
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

/* 1 = high, 0 = low */
static int Config_Mars_GPIO_Level(unsigned gpio, int level)
{
    gpio_set_value(gpio, level);
    return 0;
}

static int Get_Mars_GPIO_Level(unsigned gpio)
{
    gpio_get_value(gpio);
    return 0;
}

/* ##################### GPIO related operations ##################### */

/* Mapping the mars gpio registers */
static void mars_ds18b20_hw_init(void)
{
    Free_Mars_Gpio(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    Request_Mars_Gpio(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, "ds18b20\n");
}

static void mars_ds18b20_hw_exit(void)
{
    Free_Mars_Gpio(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
}

/* ############################## DS18B20 Operations ############################## */
static unsigned char ds18b20_reset(void)
{
    unsigned char ret = 0;
            
    /* Config the GPIO_6_8 to output */
    Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);
    
    /* Send a rising edge(HIGH) to the ds18b20, and delay for 100ms */
    Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
    //udelay(100);
    
    /* Send a falling edge(LOW) to the ds18b20, and delay for 600ms */
    Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_LOW_LEVEL);
    udelay(750);    

    /* Send a rising edge to the ds18b20, delay for 100ms, and release the ds18b20 bus */
    Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
    udelay(80);
    //udelay(100);
    
    /* Check the reset status */
    /* Config the GPIO_6_8 to input so that we can check the reset status */
    Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, INPUT_DIR);
    
    /* if the bus status is HIGH level after releasing the ds18b20 bus, then reset is failed, otherwise successfully */
    //if (Get_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN))
    //    return -1;
    ret = Get_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);

    /* Config the GPIO_6_8 to output */
    Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);
    udelay(300);
    Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);

    return ret;
}

static void ds18b20_write_byte(unsigned char cmd)
{
    unsigned char i;

    /* Config the GPIO_6_8 to output */
    //Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);
    
    /* The ds18b20 bus starts from HIGH to LOW to generate the WRITE timing */
    for (i = 0; i < 8; i++) {
        /* Config the GPIO_6_8 to output */
        Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);

        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_LOW_LEVEL);
        udelay(10);
        
        if (cmd & 0x01)
            Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
        else
            Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_LOW_LEVEL);
        
        /* Holds the level at least 60ms */
        udelay(60);
        cmd >>= 1;

        /* Pull up to HIGH to release the ds18b20 bus */
        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
    }

    /* Pull up to HIGH to release the ds18b20 bus */
    //Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
}

static unsigned char ds18b20_read_byte(void)
{
    int i;
    unsigned char ret_data = 0;
    
    for (i = 0; i < 8; i++) {

        ret_data >>= 1;                     // Read LSB
        
        // 总线从高拉至低，只需维持低电平17ts，再把总线拉高，就产生读时序
        Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);
        
        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_LOW_LEVEL);
        udelay(5);
        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
        udelay(10);

        //ret_data >>= 1;

        Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, INPUT_DIR);
        
        if (Get_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN))
            ret_data |= 0x80;

        printk("Get_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN) = %d\n", Get_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN));

        udelay(60);
        
        /* Config the GPIO_6_8 to output */
        Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);
        
        /* Pull up to HIGH to release the ds18b20 bus */
        Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);
    }

    /* Config the GPIO_6_8 to output */
    //Set_Mars_Gpio_Direction(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, OUTPUT_DIR);

    /* Pull up to HIGH to release the ds18b20 bus */
    //Config_Mars_GPIO_Level(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, DATA_HIGH_LEVEL);

    return ret_data;
}

static unsigned int ds18b20_ops(void)
{
    unsigned char msByte = 0, lsByte = 0;
    unsigned int temp;
    
    if (ds18b20_reset()) {
        printk(KERN_WARNING "[%s] ds18b20 is failed at line %d!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    //udelay(120);

    ds18b20_write_byte(0xCC);                   // Skip ROM
    ds18b20_write_byte(0x44);                   // Start temperature conversion

    //udelay(5);

    if (ds18b20_reset()) {
        printk(KERN_WARNING "[%s] ds18b20 is failed at line %d!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    //udelay(200);

    ds18b20_write_byte(0xCC);                   // Skip ROM
    ds18b20_write_byte(0xBE);                   // Read scratchpad

#if 0
    temp_data[0] = ds18b20_read_byte();         // Read the 0th byte in the scratchpad
    printk("%s, temp_data[0] = %d\n", __FUNCTION__, ds18b20_read_byte());

    temp_data[1] = ds18b20_read_byte();         // Read the 1st byte in the scratchpad
    printk("%s, temp_data[1] = %d\n", __FUNCTION__, ds18b20_read_byte());

    return 0;
#else
    lsByte = ds18b20_read_byte();            // Read the 0st byte in the scratchpad
    msByte = ds18b20_read_byte();            // Read the 1st byte in the scratchpad
    temp = msByte;
    temp <<= 8;
    temp |= lsByte;

    return temp;
#endif    
}

static int mars_ds18b20_open(struct inode *inode, struct file *filp)
{    
    mars_ds18b20_hw_init();
    
    if (ds18b20_reset()) {
        printk(KERN_WARNING "[%s] ds18b20 is failed at line %d!\n", __FUNCTION__, __LINE__);
        return -1;
    }

	printk(KERN_NOTICE "%s successfully!!\n", __FUNCTION__);
    return 0;
}

static ssize_t mars_ds18b20_read(struct file *filp, char __user *buf, 
                                size_t size, loff_t *ppos)
{
#if 0    
    unsigned long err;
    int ret = 0;

    ret = ds18b20_ops();
    if (ret)
        return -EFAULT;
    
    err = copy_to_user(buf, &temp_data, sizeof(temp_data));
    return err ? -EFAULT : min(sizeof(temp_data), size);
#else
    unsigned int temp;

    temp = ds18b20_ops();

    if (copy_to_user(buf, &temp, sizeof(temp)))
        return -EFAULT;

    return sizeof(temp);
#endif
}

static int mars_ds18b20_close(struct inode *inode, struct file *filp)
{
    mars_ds18b20_hw_exit();
    return 0;
}

static struct file_operations mars_ds18b20_fops = {
    .owner              = THIS_MODULE,
    .open               = mars_ds18b20_open,
    .read               = mars_ds18b20_read,
    .release            = mars_ds18b20_close,    
};

static void mars_ds18b20_setup_cdev(struct mars_ds18b20_dev *dev, 
        int minor)
{
    int error;
    dev_t devno = MKDEV(mars_ds18b20_major, minor);
    
    /* Initializing cdev */
    cdev_init(&dev->cdev, &mars_ds18b20_fops);
    dev->cdev.owner = THIS_MODULE;

    /* Adding cdev */
    error = cdev_add(&dev->cdev, devno, 1);

    if (error) {
        printk(KERN_NOTICE "[KERNEL(%s)]Error %d adding leds", __FUNCTION__, error);
    }
}

static int __init ds18b20_drv_init(void)
{
	int ret = 0;
    dev_t devno = MKDEV(mars_ds18b20_major, 0);

    /* Allocating mars_ds18b20_dev structure dynamically */
    mars_ds18b20_devp = kmalloc(sizeof(struct mars_ds18b20_dev), GFP_KERNEL);
    if (!mars_ds18b20_devp) {
        return -ENOMEM;
    }

    memset(mars_ds18b20_devp, 0, sizeof(struct mars_ds18b20_dev));

    mars_ds18b20_devp->ds18b20_name = MARS_DS18B20_NAME;

    /* Register char devices region */
    if (mars_ds18b20_major) {
        ret = register_chrdev_region(devno, 1, mars_ds18b20_devp->ds18b20_name);
    }else {
        /* Allocating major number dynamically */
        ret = alloc_chrdev_region(&devno, 0, 1, mars_ds18b20_devp->ds18b20_name);
        mars_ds18b20_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;
    
    /* Helper function to initialize and add cdev structure */
    mars_ds18b20_setup_cdev(mars_ds18b20_devp, 0);

    /* mdev - automatically create the device node */
    mars_ds18b20_devp->ds18b20_cls = class_create(THIS_MODULE, mars_ds18b20_devp->ds18b20_name);
    if (IS_ERR(mars_ds18b20_devp->ds18b20_cls))
        return PTR_ERR(mars_ds18b20_devp->ds18b20_cls);

    mars_ds18b20_devp->ds18b20_dev = device_create(mars_ds18b20_devp->ds18b20_cls, NULL, devno, NULL, mars_ds18b20_devp->ds18b20_name);    
	if (IS_ERR(mars_ds18b20_devp->ds18b20_dev)) {
        class_destroy(mars_ds18b20_devp->ds18b20_cls);
        cdev_del(&mars_ds18b20_devp->cdev);
        unregister_chrdev_region(devno, 1);
        kfree(mars_ds18b20_devp);
		return PTR_ERR(mars_ds18b20_devp->ds18b20_dev);
	}
        
	printk(MARS_DS18B20_NAME" is initialized!!\n");
    
    return ret;
}
module_init(ds18b20_drv_init);

static void __exit ds18b20_drv_exit(void)
{
    device_destroy(mars_ds18b20_devp->ds18b20_cls, MKDEV(mars_ds18b20_major, 0));
    class_destroy(mars_ds18b20_devp->ds18b20_cls);
    cdev_del(&mars_ds18b20_devp->cdev);
    unregister_chrdev_region(MKDEV(mars_ds18b20_major, 0), 1);
    kfree(mars_ds18b20_devp);    
	printk(MARS_DS18B20_NAME" is leaving away!!\n");
}
module_exit(ds18b20_drv_exit);

