
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

struct mars_ds18b20_dev {
    const char *ds18b20_name;
    struct cdev cdev;
    struct class *ds18b20_cls;
    struct device *ds18b20_dev;
};

static struct mars_ds18b20_dev *mars_ds18b20_devp;

#define MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN                     IMX_GPIO_NR(6,8)                // NANDF_ALE

/* Mapping the mars gpio registers */
static int mars_ds18b20_hw_init(void)
{
    int status = 0;
    //gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    status = gpio_request(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, "ds18b20\n");
    if (status < 0) {
        printk("Failed to request gpio for ds18b20\n");
        status = -EBUSY;
    }else {
        printk("Request gpio for ds18b20 successfully!\n");
    }    

    return status;
}

static void mars_ds18b20_hw_exit(void)
{
    gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
}

/* ############################## DS18B20 Operations ############################## */
static int ds18b20_reset(void)
{
    int retval = 0;
  
    gpio_direction_output(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);      // 1 = high level
  
    //gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
    udelay(2);

    /* Pull down the ds18b20 and reset it */
    gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 0);

    /* Hold the low level to 500us */
    udelay(500);

    /* Release the ds18b20 bus line */
    gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
    udelay(60);

    /* MUST free and request the gpio so that we can config the gpio to input mode */
    gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    retval = gpio_request(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, "ds18b20\n");
    if (retval < 0) {
        printk("Failed to request gpio for ds18b20\n");
        return -EBUSY;
    }else {
        //printk("[%s, %d]Request gpio for ds18b20 successfully!\n", __FUNCTION__, __LINE__);
    }
  
    /* DS18B20 sends a low level pulse width if the reset is successfully, and hold it for 60 to 240us */
    gpio_direction_input(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    retval = gpio_get_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);

    udelay(500);

    /* Free and request the gpio AGAIN to config to output mode */
    gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    retval = gpio_request(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, "ds18b20\n");
    if (retval < 0) {
        printk("Failed to request gpio for ds18b20\n");
        return -EBUSY;
    }else {
        //printk("[%s, %d]Request gpio for ds18b20 successfully!\n", __FUNCTION__, __LINE__);
    }
    
    gpio_direction_output(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);      // 1 = high level
    //gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
  
    return retval;  
}

static void ds18b20_write_byte(unsigned char cmd)
{
    int i = 0;  
  
    gpio_direction_output(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);

    for (i = 0; i < 8; i++) {  
        /* The write timing is happened when the bus line pull from high to low  */
        gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
        udelay(2);
        gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 0);
        gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, cmd & 0x01);
        udelay(60);
        cmd >>= 1;  
    }

    /* Release the ds18b20 bus line */
    gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
}

static unsigned char ds18b20_read_byte(void)
{
    int i;
    unsigned char data = 0;
    int retval = 0;
      
    for (i = 0; i < 8; i++) {  
        /* 
         *  The read timing is happened when the bus line pull from high to low.
         *  Hold the low level pulse to 17ts, and pull up the bus to high.
         */
        gpio_direction_output(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 0);
        gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
        udelay(2);  
        gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 0);
        udelay(2);  
        gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
        udelay(8);
        
        data >>= 1;

        /* MUST free and request the gpio so that we can config the gpio to input mode */
        gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
        retval = gpio_request(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, "ds18b20\n");
        if (retval < 0) {
            printk("Failed to request gpio for ds18b20\n");
            return -EBUSY;
        }else {
            //printk("[%s, %d]Request gpio for ds18b20 successfully!\n", __FUNCTION__, __LINE__);
        }

        gpio_direction_input(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
        if (gpio_get_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN))
            data |= 0x80;
        udelay(80);  
    }

    /* Free and request the gpio AGAIN to config to output mode */
    gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    retval = gpio_request(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, "ds18b20\n");
    if (retval < 0) {
        printk("Failed to request gpio for ds18b20\n");
        return -EBUSY;
    }else {
        //printk("[%s, %d]Request gpio for ds18b20 successfully!\n", __FUNCTION__, __LINE__);
    }

    /* Release the ds18b20 bus line */
    gpio_direction_output(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
    //gpio_set_value(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN, 1);
    
    return data;  
}

static int mars_ds18b20_open(struct inode *inode, struct file *filp)
{
    int flag = 0;
    int ret = 0;
    ret = mars_ds18b20_hw_init();
    if (ret)
        return ret;
  
    flag = ds18b20_reset();
    if (flag & 0x01) {  
        printk(KERN_WARNING "[%s] ds18b20 is failed at line %d!\n", __FUNCTION__, __LINE__);
        return -1;  
    }  
	printk(KERN_NOTICE "%s successfully!!\n", __FUNCTION__);
    return 0;
}

static ssize_t mars_ds18b20_read(struct file *filp, char __user *buf, 
                                size_t size, loff_t *ppos)
{
    int flag;
    unsigned char msByte = 0, lsByte = 0;
    unsigned int temp;
  
    flag = ds18b20_reset();
    if (flag & 0x01) {
        printk(KERN_WARNING "ds18b20 init failed\n");
        return -1;
    }
      
    ds18b20_write_byte(0xcc);
    ds18b20_write_byte(0x44);
  
    flag = ds18b20_reset();
    if (flag & 0x01)
        return -1;
  
    ds18b20_write_byte(0xcc);
    ds18b20_write_byte(0xbe);

    lsByte = ds18b20_read_byte();       // LSB of the temperature
    msByte = ds18b20_read_byte();       // MSB of the temperature
    temp = msByte;
    temp <<= 8;
    temp |= lsByte;

    if (copy_to_user(buf, &temp, sizeof(temp)))
        return -EFAULT;

    return sizeof(temp);
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
        printk(KERN_NOTICE "[KERNEL(%s)]Error %d adding ds18b20", __FUNCTION__, error);
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
    
    return 0;
}
module_init(ds18b20_drv_init);

static void __exit ds18b20_drv_exit(void)
{
    //gpio_free(MARS_DS18B20_NANDF_ALE_GPIO_6_8_PIN);
    device_destroy(mars_ds18b20_devp->ds18b20_cls, MKDEV(mars_ds18b20_major, 0));
    class_destroy(mars_ds18b20_devp->ds18b20_cls);
    cdev_del(&mars_ds18b20_devp->cdev);
    unregister_chrdev_region(MKDEV(mars_ds18b20_major, 0), 1);
    kfree(mars_ds18b20_devp);    
	printk(MARS_DS18B20_NAME" is leaving away!!\n");
}
module_exit(ds18b20_drv_exit);

