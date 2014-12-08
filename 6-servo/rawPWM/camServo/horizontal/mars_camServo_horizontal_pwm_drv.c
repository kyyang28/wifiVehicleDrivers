
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

#define DEVICE_NAME                     "mars_camServo_horizontal_pwm"

/* Camera servo using pwm1(P2 pin26 PWM1) */

//#define NS_IN_1HZ				        (1000000000UL)
#define MARS_PWM_IOCTL_SET_FREQ		    1
#define MARS_PWM_IOCTL_STOP			    0

struct mars_pwm_camServo_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
};

static struct semaphore lock;
static struct mars_pwm_camServo_data *pb;

static void mars_pwm_set_freq(unsigned long freq) {
	//int period_ns = NS_IN_1HZ / freq;
	pwm_config(pb->pwm, freq, pb->period);
	pwm_enable(pb->pwm);
}

static void mars_pwm_stop(void)
{    
	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);
}

static int mars_camServo_pwm_open(struct inode *inode, struct file *file) {
	if (!down_trylock(&lock))
		return 0;
	else
		return -EBUSY;
}

static int mars_camServo_pwm_close(struct inode *inode, struct file *file) {
	up(&lock);
	return 0;
}

static long mars_camServo_pwm_ioctl(struct file *filep, unsigned int cmd,
		unsigned long arg)
{
	switch (cmd) {
		case MARS_PWM_IOCTL_SET_FREQ:
			if (arg == 0)
				return -EINVAL;
			mars_pwm_set_freq(arg);
			break;

		case MARS_PWM_IOCTL_STOP:
		default:
			mars_pwm_stop();
			break;
	}

	return 0;
}

static struct file_operations mars_camServo_pwm_ops = {
	.owner			= THIS_MODULE,
	.open			= mars_camServo_pwm_open,
	.release		= mars_camServo_pwm_close, 
	.unlocked_ioctl	= mars_camServo_pwm_ioctl,
};

static struct miscdevice mars_camServo_pwm_misc_dev = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name   = DEVICE_NAME,
	.fops   = &mars_camServo_pwm_ops,
};

static int mars_pwm_camServo_probe(struct platform_device *pdev)
{
	struct platform_pwm_camServo_data *data = pdev->dev.platform_data;
	int ret;

	if (!data) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	if (data->init) {
		ret = data->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}

	pb = kzalloc(sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

    /* init pb's members */
    pb->period = data->pwm_period_ns;
    pb->dev = &pdev->dev;
    pb->pwm = pwm_request(data->pwm_id, "camServo");
	if (IS_ERR(pb->pwm)) {
		dev_err(&pdev->dev, "unable to request PWM for camServo\n");
		ret = PTR_ERR(pb->pwm);
		goto err_pwm;
	} else
		dev_dbg(&pdev->dev, "got pwm for camServo\n");

    printk("[%s] pb->period = %u\n", __FUNCTION__, pb->period);
    
	//platform_set_drvdata(pdev, pb);

    mars_pwm_stop();

	sema_init(&lock, 1);
    ret = misc_register(&mars_camServo_pwm_misc_dev);
    if (ret)
        goto err_pwm;

    printk("%s is invoked!\n", __FUNCTION__);
	return 0;
    
err_pwm:
	kfree(pb);
err_alloc:
    if (data->exit)
        data->exit(&pdev->dev);
    return ret;
}

static int mars_pwm_camServo_remove(struct platform_device *pdev)
{
	struct platform_pwm_camServo_data *data = pdev->dev.platform_data;
	//struct mars_pwm_camServo_data *pb = dev_get_drvdata(&pdev->dev);

    misc_deregister(&mars_camServo_pwm_misc_dev);
    pwm_config(pb->pwm, 0, pb->period);
    pwm_disable(pb->pwm);
    pwm_free(pb->pwm);
    kfree(pb);
	if (data->exit)
		data->exit(&pdev->dev);
    
    printk("%s is moving away!\n", __FUNCTION__);
    return 0;
}

static struct platform_driver mars_pwm_camServo_driver = {
	.driver		= {
		.name	= "pwm-camServo",
		.owner	= THIS_MODULE,
	},
	.probe		= mars_pwm_camServo_probe,
	.remove		= mars_pwm_camServo_remove,
};

static int __init mars_pwm_camServo_init(void)
{
	return platform_driver_register(&mars_pwm_camServo_driver);
}
module_init(mars_pwm_camServo_init);

static void __exit mars_pwm_camServo_exit(void)
{
	platform_driver_unregister(&mars_pwm_camServo_driver);
}
module_exit(mars_pwm_camServo_exit);

MODULE_DESCRIPTION("PWM based camServo Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-camServo");

