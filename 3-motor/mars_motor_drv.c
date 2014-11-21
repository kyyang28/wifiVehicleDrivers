
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

#define MARS_MOTOR_MAJOR                    0
#define MARS_MOTOR_NAME                     "mars_motor"

#define DATA_LOW_LEVEL                      0
#define DATA_HIGH_LEVEL                     1

#if 1
#define WIFI_VEHICLE_MOVE_FORWARD           0
#define WIFI_VEHICLE_MOVE_BACKWARD          1
/* DONOT use number '2', since ioctl is taken this number. If you have to use 2, then use magic number */
/* We don't use ioctl's magic number here, because our app is Windows Qt, it cannot use <sys/ioctl> header file */
#define WIFI_VEHICLE_MOVE_LEFT              3
#define WIFI_VEHICLE_MOVE_RIGHT             4
#define WIFI_VEHICLE_STOP                   5
#define WIFI_VEHICLE_BUZZER_ON              6
#define WIFI_VEHICLE_BUZZER_OFF             7
#else
#define WIFI_VEHICLE_MAGIC                  'k'
#define WIFI_VEHICLE_MOVE_FORWARD           _IO(WIFI_VEHICLE_MAGIC, 0)
#define WIFI_VEHICLE_MOVE_BACKWARD          _IO(WIFI_VEHICLE_MAGIC, 1)
#define WIFI_VEHICLE_MOVE_LEFT              _IO(WIFI_VEHICLE_MAGIC, 2)
#define WIFI_VEHICLE_MOVE_RIGHT             _IO(WIFI_VEHICLE_MAGIC, 3)
#define WIFI_VEHICLE_STOP                   _IO(WIFI_VEHICLE_MAGIC, 4)
#define WIFI_VEHICLE_BUZZER_ON              _IO(WIFI_VEHICLE_MAGIC, 5)
#define WIFI_VEHICLE_BUZZER_OFF             _IO(WIFI_VEHICLE_MAGIC, 6)
#endif

static int mars_motor_major = MARS_MOTOR_MAJOR;

struct mars_motor_dev {
    const char *motor_name;
    struct cdev cdev;
    struct class *motor_cls;
    struct device *motor_dev;
};

static struct mars_motor_dev *mars_motor_devp;

/* buzzer */
#define MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN                     IMX_GPIO_NR(6,11)                // NANDF_CS0

/* motor */
#define MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN              IMX_GPIO_NR(6,7)                 // NANDF_CLE   IN1
#define MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN             IMX_GPIO_NR(6,9)                 // NANDF_WP_B  IN2
#define MARS_MOTOR_ENA_ENA_NANDF_RB0_GPIO_6_10_PIN              IMX_GPIO_NR(6,10)                // NANDF_RB0
#define MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN               IMX_GPIO_NR(2,0)                 // NANDF_D0    IN3
#define MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN               IMX_GPIO_NR(2,1)                 // NANDF_D1    IN4
#define MARS_MOTOR_ENB_ENB_NANDF_D2_GPIO_2_2_PIN                IMX_GPIO_NR(2,2)                 // NANDF_D2

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

/* ################################## Buzzer ops ################################## */
/* Since buzzer is controlled by NANDF_CS0, we use the GPIO_6_11 */
/* Disable buzzer */
static void mars_buzzer_off(void)
{
    Config_Mars_GPIO_Level(MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN, DATA_LOW_LEVEL);
}

/* Enable buzzer */
static void mars_buzzer_on(void)
{
    Config_Mars_GPIO_Level(MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN, DATA_HIGH_LEVEL);
}
/* ################################## buzzer ops ################################## */

/* ################################## motor ops ################################## */
static void mars_motor_free_gpio(void)
{
    Free_Mars_Gpio(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN);
    Free_Mars_Gpio(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN);
    Free_Mars_Gpio(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN);
    Free_Mars_Gpio(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN);
    Free_Mars_Gpio(MARS_MOTOR_ENA_ENA_NANDF_RB0_GPIO_6_10_PIN);
    Free_Mars_Gpio(MARS_MOTOR_ENB_ENB_NANDF_D2_GPIO_2_2_PIN);
}

static void mars_motor_request_gpio(void)
{   
    Request_Mars_Gpio(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, "out1\n");
    Request_Mars_Gpio(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, "out2\n");
    Request_Mars_Gpio(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, "out3\n");
    Request_Mars_Gpio(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, "out4\n");
    Request_Mars_Gpio(MARS_MOTOR_ENA_ENA_NANDF_RB0_GPIO_6_10_PIN, "en_a\n");
    Request_Mars_Gpio(MARS_MOTOR_ENB_ENB_NANDF_D2_GPIO_2_2_PIN, "en_b\n");
}

static void mars_motor_gpio_dir_setup(void)
{
    Set_Mars_Gpio_Direction_Output(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_LOW_LEVEL);
    Set_Mars_Gpio_Direction_Output(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_LOW_LEVEL);
    Set_Mars_Gpio_Direction_Output(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_LOW_LEVEL);
    Set_Mars_Gpio_Direction_Output(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_LOW_LEVEL);
    Set_Mars_Gpio_Direction_Output(MARS_MOTOR_ENA_ENA_NANDF_RB0_GPIO_6_10_PIN, DATA_LOW_LEVEL);
    Set_Mars_Gpio_Direction_Output(MARS_MOTOR_ENB_ENB_NANDF_D2_GPIO_2_2_PIN, DATA_LOW_LEVEL);
}

static void mars_motor_gpio_data_level(unsigned gpio, int level)
{
    Config_Mars_GPIO_Level(gpio, level);
}
/* ################################## motor ops ################################## */

/*
 *  desc: In order to move the vehicle forward, we do the following:
 *      OUT1 = low level
 *      OUT2 = high level
 *      OUT3 = low level
 *      OUT4 = high level
 */
static void mars_motor_move_forward(void)
{
    mars_motor_gpio_data_level(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_LOW_LEVEL);         // OUT1 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_HIGH_LEVEL);       // OUT2 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_LOW_LEVEL);          // OUT3 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_HIGH_LEVEL);         // OUT4 = 1
}

/*
 *  desc: In order to move the vehicle backward, we do the following:
 *      OUT1 = high level
 *      OUT2 = low level
 *      OUT3 = high level
 *      OUT4 = low level
 */
static void mars_motor_move_backward(void)
{
    mars_motor_gpio_data_level(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_HIGH_LEVEL);       // OUT1 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_LOW_LEVEL);       // OUT2 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_HIGH_LEVEL);        // OUT3 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_LOW_LEVEL);         // OUT4 = 0
}

/*
 *  desc: In order to move the vehicle left, we do the following:
 *      OUT1 = low level
 *      OUT2 = high level
 *      OUT3 = high level
 *      OUT4 = high level
 *  note: OUT1, OUT2 control the right tire
 *        OUT3, OUT4 control the left tire
 *        so to control the vehicle move left, we simply set the out1 to low, out2 to high, 
 *        and keep the out3, out4 both to low or high, here, we set the out3, out4 to high
 *        because we wanna light the left LED up based on the schematic diagram.
 */
static void mars_motor_move_left(void)
{
    mars_motor_gpio_data_level(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_LOW_LEVEL);         // OUT1 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_HIGH_LEVEL);       // OUT2 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_HIGH_LEVEL);         // OUT3 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_HIGH_LEVEL);         // OUT4 = 1
}

static void mars_motor_move_right(void)
{
    mars_motor_gpio_data_level(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_HIGH_LEVEL);        // OUT1 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_HIGH_LEVEL);       // OUT2 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_LOW_LEVEL);          // OUT3 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_HIGH_LEVEL);         // OUT4 = 1
}

static void mars_motor_stop_ledoff(void)
{
    mars_motor_gpio_data_level(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_LOW_LEVEL);         // OUT1 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_LOW_LEVEL);        // OUT2 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_LOW_LEVEL);          // OUT3 = 0
    mars_motor_gpio_data_level(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_LOW_LEVEL);          // OUT4 = 0
}

#if 0
static void mars_motor_stop_ledon(void)
{
    mars_motor_gpio_data_level(MARS_MOTOR_IN1_OUT1_NANDF_CLE_GPIO_6_7_PIN, DATA_HIGH_LEVEL);        // OUT1 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN2_OUT2_NANDF_WP_B_GPIO_6_9_PIN, DATA_HIGH_LEVEL);       // OUT2 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN3_OUT3_NANDF_D0_GPIO_2_0_PIN, DATA_HIGH_LEVEL);         // OUT3 = 1
    mars_motor_gpio_data_level(MARS_MOTOR_IN4_OUT4_NANDF_D1_GPIO_2_1_PIN, DATA_HIGH_LEVEL);         // OUT4 = 1
}
#endif

static void mars_motor_hw_init(void)
{
    mars_motor_free_gpio();
    mars_motor_request_gpio();
    mars_motor_gpio_dir_setup();
}

static void mars_motor_hw_exit(void)
{
    mars_motor_free_gpio();
}

/* ################################## motor ops ################################## */

static int mars_motor_open(struct inode *inode, struct file *filp)
{
    /* Motors hardware related initialization */
    mars_motor_hw_init();
    return 0;
}

/* Based on the write api, the system allows to control the motor speed by using the duty ratio */
static ssize_t mars_motor_write(struct file *filp, const char __user *buf,
                                    size_t size, loff_t *ppos)
{
    char val;

    if (copy_from_user(&val, buf, 1))
		return -EFAULT;

    /* if the user passing 1 to the kernel, which the system will set the motor to high level, otherwise, to low level */
    if (val & 0x1) {
        mars_motor_gpio_data_level(MARS_MOTOR_ENA_ENA_NANDF_RB0_GPIO_6_10_PIN, DATA_HIGH_LEVEL);
        mars_motor_gpio_data_level(MARS_MOTOR_ENB_ENB_NANDF_D2_GPIO_2_2_PIN, DATA_HIGH_LEVEL);
    }else {
        mars_motor_gpio_data_level(MARS_MOTOR_ENA_ENA_NANDF_RB0_GPIO_6_10_PIN, DATA_LOW_LEVEL);
        mars_motor_gpio_data_level(MARS_MOTOR_ENB_ENB_NANDF_D2_GPIO_2_2_PIN, DATA_LOW_LEVEL);
    }
    
    return 1;
}

static long mars_motor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)    
{
    switch (cmd) {
    case WIFI_VEHICLE_MOVE_FORWARD:
        //mars_motor_stop_ledoff();
        mars_motor_move_forward();        /* led all off */
        //mdelay(20);
        //mars_motor_stop_ledoff();
        break;

    case WIFI_VEHICLE_MOVE_BACKWARD:
        //mars_motor_stop_ledoff();
        mars_motor_move_backward();       /* led all on */
        //mdelay(20);
        //mars_motor_stop_ledoff();
        break;

    case WIFI_VEHICLE_MOVE_LEFT:
        //mars_motor_stop_ledoff();
        mars_motor_move_left();           /* left led on */
        //mdelay(20);
        //mars_motor_stop_ledoff();
        break;

    case WIFI_VEHICLE_MOVE_RIGHT:
        //mars_motor_stop_ledoff();
        mars_motor_move_right();          /* right led on */
        //mdelay(20);
        //mars_motor_stop_ledoff();
        break;
       
    case WIFI_VEHICLE_STOP:
        mars_motor_stop_ledoff();         /* mars_motor_stop_ledon is fine as well */
        break;

    case WIFI_VEHICLE_BUZZER_ON:
        mars_buzzer_on();
        break;

    case WIFI_VEHICLE_BUZZER_OFF:
        mars_buzzer_off();
        break;

    default:
        break;
    }
    
    return 0;
}

static int mars_motor_close(struct inode *inode, struct file *filp)
{
    mars_motor_hw_exit();
    return 0;
}

static struct file_operations mars_motor_fops = {
    .owner              = THIS_MODULE,
    .open               = mars_motor_open,
    .write              = mars_motor_write,
    .unlocked_ioctl     = mars_motor_ioctl,
    .release            = mars_motor_close,
};

static void mars_motor_setup_cdev(struct mars_motor_dev *dev, 
        int minor)
{
    int error;
    dev_t devno = MKDEV(mars_motor_major, minor);
    
    /* Initializing cdev */
    cdev_init(&dev->cdev, &mars_motor_fops);
    dev->cdev.owner = THIS_MODULE;

    /* Adding cdev */
    error = cdev_add(&dev->cdev, devno, 1);

    if (error) {

        printk(KERN_NOTICE "[KERNEL(%s)]Error %d adding leds", __FUNCTION__, error);
    }
}

/* Mapping the mars gpio registers */
static int mars_buzzer_hw_init(void)
{
    int status;

    Free_Mars_Gpio(MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN);

    status = Request_Mars_Gpio(MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN, "buzzer\n");

    status = Set_Mars_Gpio_Direction_Output(MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN, DATA_LOW_LEVEL);     // 0 = output
    
    return status;
}

static void mars_buzzer_hw_exit(void)
{
    Free_Mars_Gpio(MARS_BUZZER_NANDF_CS0_GPIO_6_11_PIN);
}

static int __init motor_drv_init(void)
{
	int ret = 0;
    dev_t devno = MKDEV(mars_motor_major, 0);

    /* Allocating mars_motor_dev structure dynamically */
    mars_motor_devp = kmalloc(sizeof(struct mars_motor_dev), GFP_KERNEL);
    if (!mars_motor_devp) {
        return -ENOMEM;
    }

    memset(mars_motor_devp, 0, sizeof(struct mars_motor_dev));

    mars_motor_devp->motor_name = MARS_MOTOR_NAME;

    /* Register char devices region */
    if (mars_motor_major) {
        ret = register_chrdev_region(devno, 1, mars_motor_devp->motor_name);
    }else {
        /* Allocating major number dynamically */
        ret = alloc_chrdev_region(&devno, 0, 1, mars_motor_devp->motor_name);
        mars_motor_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    
    /* Helper function to initialize and add cdev structure */
    mars_motor_setup_cdev(mars_motor_devp, 0);

    /* mdev - automatically create the device node */
    mars_motor_devp->motor_cls = class_create(THIS_MODULE, mars_motor_devp->motor_name);
    if (IS_ERR(mars_motor_devp->motor_cls))
        return PTR_ERR(mars_motor_devp->motor_cls);

    mars_motor_devp->motor_dev = device_create(mars_motor_devp->motor_cls, NULL, devno, NULL, mars_motor_devp->motor_name);    
	if (IS_ERR(mars_motor_devp->motor_dev)) {
        class_destroy(mars_motor_devp->motor_cls);
        cdev_del(&mars_motor_devp->cdev);
        unregister_chrdev_region(devno, 1);
        kfree(mars_motor_devp);
		return PTR_ERR(mars_motor_devp->motor_dev);
	}
    
    /* Buzzer hardware related initialization */
    mars_buzzer_hw_init();

    /* Enable buzzer */
    mars_buzzer_on();
    
    //ssleep(1);
    mdelay(300);
    
    /* Disable buzzer */
    mars_buzzer_off();

	printk(MARS_MOTOR_NAME" is initialized!!\n");
    
    return ret;
}
module_init(motor_drv_init);

static void __exit motor_drv_exit(void)
{
    mars_buzzer_hw_exit();
    device_destroy(mars_motor_devp->motor_cls, MKDEV(mars_motor_major, 0));
    class_destroy(mars_motor_devp->motor_cls);
    cdev_del(&mars_motor_devp->cdev);
    unregister_chrdev_region(MKDEV(mars_motor_major, 0), 1);
    kfree(mars_motor_devp);    
	printk(MARS_MOTOR_NAME" is left away!!\n");
}
module_exit(motor_drv_exit);

