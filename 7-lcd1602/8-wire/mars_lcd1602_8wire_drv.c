
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
#include <linux/delay.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/delay.h>

#include <mach/gpio.h>      /* arch/arm/plat-mxc/include/mach/gpio.h, including macro IMX_GPIO_NR and gpio_set_value etc  */

struct lcd1602_dev {
    struct cdev cdev;
};

MODULE_AUTHOR("Charles Yang <charlesyang28@gmail.com>");
MODULE_LICENSE("GPL");

static int mars_lcd1602_major = 0;
static int mars_lcd1602_minor = 0;

static struct lcd1602_dev *mars_lcd1602_dev = NULL;
static struct class *mars_lcd1602_cls = NULL;

static void mars_lcd1602_drv_cleanup_module(void);

#define MARS_LCD1602_SD4_DAT3__GPIO_2_11_PIN                IMX_GPIO_NR(2,11)   // LCD1602_RS   pin01 
#define MARS_LCD1602_GPIO_18__GPIO_7_13_PIN                 IMX_GPIO_NR(7,13)   // LCD1602_RW   pin20
#define MARS_LCD1602_GPIO_17__GPIO_7_12_PIN                 IMX_GPIO_NR(7,12)   // LCD1602_E    pin24

#define MARS_LCD1602_CSI0_DAT4__GPIO_5_22_PIN               IMX_GPIO_NR(5,22)   // LCD1602_D0   pin25
#define MARS_LCD1602_CSI0_DAT5__GPIO_5_23_PIN               IMX_GPIO_NR(5,23)   // LCD1602_D1   pin27
#define MARS_LCD1602_CSI0_DAT6__GPIO_5_24_PIN               IMX_GPIO_NR(5,24)   // LCD1602_D2   pin29
#define MARS_LCD1602_CSI0_DAT7__GPIO_5_25_PIN               IMX_GPIO_NR(5,25)   // LCD1602_D3   pin31
#define MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN                IMX_GPIO_NR(2,3)   // LCD1602_D4   J10, pin20
#define MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN                IMX_GPIO_NR(2,4)   // LCD1602_D5   J10, pin22
//#define MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN                IMX_GPIO_NR(4,11)   // LCD1602_D4   pin33
//#define MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN                IMX_GPIO_NR(4,10)   // LCD1602_D5   pin35
#define MARS_LCD1602_KEY_ROW4__GPIO_4_15_PIN                IMX_GPIO_NR(4,15)   // LCD1602_D6   pin37
#define MARS_LCD1602_KEY_COL4__GPIO_4_14_PIN                IMX_GPIO_NR(4,14)   // LCD1602_D7   pin39

#define LCD1602_WRITE_CMD                                   (0)
#define LCD1602_WRITE_DATA                                  (1)

static void Free_Mars_Lcd1602_Gpio(unsigned gpio)
{    
    gpio_free(gpio);
}

static int Request_Mars_Lcd1602_Gpio(unsigned gpio, char *gpioReqName)
{
    int status = 0;

    status = gpio_request(gpio, gpioReqName);
    if (status < 0)
        pr_warning("Failed to request gpio for %s", gpioReqName);    

    return status;
}

static int Set_Mars_Lcd1602_Gpio_Direction_Output(unsigned gpio, int level)
{
    int status = 0;

    status = gpio_direction_output(gpio, level);
    if (status < 0)
        pr_warning("Failed to setup the gpio's output direction\n");

    return status;
}

/* 1 = high, 0 = low */
static int Config_Mars_Lcd1602_Gpio_Level(unsigned gpio, int level)
{
    gpio_set_value(gpio, level);
    return 0;
}

static void mars_lcd1602_free_all_gpios(void)
{
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_SD4_DAT3__GPIO_2_11_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_GPIO_18__GPIO_7_13_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT4__GPIO_5_22_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT5__GPIO_5_23_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT6__GPIO_5_24_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT7__GPIO_5_25_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_ROW4__GPIO_4_15_PIN);
    Free_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_COL4__GPIO_4_14_PIN);
}

static int mars_lcd1602_request_all_gpios(void)
{
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_SD4_DAT3__GPIO_2_11_PIN, "LCD1602_RS\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_GPIO_18__GPIO_7_13_PIN, "LCD1602_RW\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, "LCD1602_E\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT4__GPIO_5_22_PIN, "LCD1602_D0\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT5__GPIO_5_23_PIN, "LCD1602_D1\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT6__GPIO_5_24_PIN, "LCD1602_D2\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_CSI0_DAT7__GPIO_5_25_PIN, "LCD1602_D3\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN, "LCD1602_D4\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN, "LCD1602_D5\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_ROW4__GPIO_4_15_PIN, "LCD1602_D6\n");
    Request_Mars_Lcd1602_Gpio(MARS_LCD1602_KEY_COL4__GPIO_4_14_PIN, "LCD1602_D7\n");
    return 0;
}

static int mars_lcd1602_setup_all_gpios_direction(void)
{
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_SD4_DAT3__GPIO_2_11_PIN, 0);        // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_GPIO_18__GPIO_7_13_PIN, 0);         // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 0);         // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_CSI0_DAT4__GPIO_5_22_PIN, 0);       // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_CSI0_DAT5__GPIO_5_23_PIN, 0);       // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_CSI0_DAT6__GPIO_5_24_PIN, 0);       // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_CSI0_DAT7__GPIO_5_25_PIN, 0);       // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN, 0);        // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN, 0);        // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_KEY_ROW4__GPIO_4_15_PIN, 0);        // 0 = low
    Set_Mars_Lcd1602_Gpio_Direction_Output(MARS_LCD1602_KEY_COL4__GPIO_4_14_PIN, 0);        // 0 = low
    return 0;
}

static int mars_lcd1602_init(void)
{
    mars_lcd1602_free_all_gpios();
    mars_lcd1602_request_all_gpios();
    mars_lcd1602_setup_all_gpios_direction();
    udelay(100);
    return 0;
}

static int mars_lcd1602_exit(void)
{
    mars_lcd1602_free_all_gpios();
    return 0;
}

static void LcdWriteCommand(int value)
{
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT4__GPIO_5_22_PIN, value & 0x01);
    value >>= 1;
    
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT5__GPIO_5_23_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT6__GPIO_5_24_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT7__GPIO_5_25_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_ROW4__GPIO_4_15_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_COL4__GPIO_4_14_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 0);     // Set LCD1602_Enable to LOW
    mdelay(1);
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 1);     // Set LCD1602_Enable to HIGH
    mdelay(1);                          // delay 1ms
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 0);     // Set LCD1602_Enable to LOW
    mdelay(1);                          // delay 1ms
}

static void LcdWriteData(int value)
{
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_SD4_DAT3__GPIO_2_11_PIN, 1);        // RS = HIGH
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_18__GPIO_7_13_PIN, 0);         // RW = LOW

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT4__GPIO_5_22_PIN, value & 0x01);
    value >>= 1;
    
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT5__GPIO_5_23_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT6__GPIO_5_24_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_CSI0_DAT7__GPIO_5_25_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_ROW2__GPIO_4_11_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_COL2__GPIO_4_10_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_ROW4__GPIO_4_15_PIN, value & 0x01);
    value >>= 1;

    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_KEY_COL4__GPIO_4_14_PIN, value & 0x01);
    value >>= 1;
    
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 0);     // Set LCD1602_Enable to LOW
    mdelay(1);
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 1);     // Set LCD1602_Enable to HIGH
    mdelay(1);                          // delay 1ms
    Config_Mars_Lcd1602_Gpio_Level(MARS_LCD1602_GPIO_17__GPIO_7_12_PIN, 0);     // Set LCD1602_Enable to LOW
    mdelay(1);                          // delay 1ms
}

static int mars_lcd1602_open(struct inode *inode, struct file *filp)
{
    struct lcd1602_dev *tmp;

    tmp = container_of(inode->i_cdev, struct lcd1602_dev, cdev);
    filp->private_data = tmp;

    LcdWriteCommand(0x38);  // 设置为 8-bit接口，2行显示，5x7文字大小
    mdelay(50);
    
    //LcdWriteCommand(0x38);  // 设置为 8-bit接口，2行显示，5x7文字大小
    //mdelay(50);

    //LcdWriteCommand(0x38);  // 设置为 8-bit接口，2行显示，5x7文字大小
    //mdelay(50);

    LcdWriteCommand(0x0C);  // 显示设置 开启显示屏，光标显示，无闪烁
    mdelay(20);

    LcdWriteCommand(0x06);  // 输入方式设定 自动增量，没有显示移位
    mdelay(20);

    LcdWriteCommand(0x01);  // 屏幕清空，光标位置归零
    mdelay(20);

    //LcdWriteCommand(0x80);  // 显示设置 开启显示屏，光标显示，无闪烁
    //mdelay(20);

    return 0;
}

static long mars_lcd1602_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    //printk("cmd = %d\n", cmd);
    //printk("arg = %d\n", arg);

    switch (cmd) {
    case LCD1602_WRITE_CMD:
        LcdWriteCommand((int)arg);
        break;

    case LCD1602_WRITE_DATA:
        LcdWriteData((int)arg);
        break;

    default:
        return -ENODEV;
    }
    
    return 0;
}

static int mars_lcd1602_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations mars_lcd1602_fops = {
    .owner              = THIS_MODULE,
    .open               = mars_lcd1602_open,
    .unlocked_ioctl     = mars_lcd1602_ioctl,
    .release            = mars_lcd1602_release,
};
    
static void mars_setup_cdev(struct cdev *lcd1602_cdev)
{
    int res, dev = MKDEV(mars_lcd1602_major, mars_lcd1602_minor);
    cdev_init(lcd1602_cdev, &mars_lcd1602_fops);
    lcd1602_cdev->owner    = THIS_MODULE;
    lcd1602_cdev->ops      = &mars_lcd1602_fops;
    res = cdev_add(lcd1602_cdev, dev, 1);
    if (res)
		printk(KERN_NOTICE "Error %d adding mars_lcd1602\n", res);
}

static int __init mars_lcd1602_drv_init(void)
{
    int res;
    dev_t dev;

    /* Step 1: Acquiring the device major */
    if (mars_lcd1602_major) {
        dev = MKDEV(mars_lcd1602_major, mars_lcd1602_minor);
        res = register_chrdev_region(dev, 1, "mars_lcd1602");
    }else {
        res = alloc_chrdev_region(&dev, mars_lcd1602_minor, 1, "mars_lcd1602");
        mars_lcd1602_major = MAJOR(dev);
    }

    if (res < 0) {
        printk(KERN_WARNING "mars_lcd1602: cannot get major %d\n", mars_lcd1602_major);
        return res;
    }
    
    /* Step 2: Setup cdev */
    mars_lcd1602_dev = kzalloc(sizeof(struct lcd1602_dev), GFP_KERNEL);
    if (!mars_lcd1602_dev) {
        res = -ENOMEM;
        goto fail;
    }
    
    mars_setup_cdev(&mars_lcd1602_dev->cdev);

    /* Step 3: Create class */
    mars_lcd1602_cls = class_create(THIS_MODULE, "mars_lcd1602");
    device_create(mars_lcd1602_cls, NULL, dev, NULL, "mars_lcd1602");

    /* Step 4: Hardware related setup */
    mars_lcd1602_init();

    printk(KERN_NOTICE "+------ %s is invoked successfully! ------+\n", __FUNCTION__);
    return 0;
    
fail:
    mars_lcd1602_drv_cleanup_module();
    return res;
    
}
module_init(mars_lcd1602_drv_init);

static void __exit mars_lcd1602_drv_exit(void)
{    
    mars_lcd1602_exit();
    mars_lcd1602_drv_cleanup_module();
    printk(KERN_NOTICE "+------ %s is invoked successfully! ------+\n", __FUNCTION__);
}
module_exit(mars_lcd1602_drv_exit);

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
static void mars_lcd1602_drv_cleanup_module(void)
{
	dev_t devno = MKDEV(mars_lcd1602_major, mars_lcd1602_minor);
    
    if (mars_lcd1602_cls) {
        device_destroy(mars_lcd1602_cls, MKDEV(mars_lcd1602_major, mars_lcd1602_minor));
        class_destroy(mars_lcd1602_cls);
    }
    
	/* Get rid of our char dev entries */
	if (mars_lcd1602_dev) {
		cdev_del(&mars_lcd1602_dev->cdev);
		kfree(mars_lcd1602_dev);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1); 
}

