
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
#include <linux/pwm_buzzer.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include <mach/gpio.h>      /* arch/arm/plat-mxc/include/mach/gpio.h, including macro IMX_GPIO_NR and gpio_set_value etc  */

struct mars_pwm_buzzer_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
};

static int mars_pwm_buzzer_probe(struct platform_device *pdev)
{
	struct platform_pwm_buzzer_data *data = pdev->dev.platform_data;
	struct mars_pwm_buzzer_data *pb;
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
    pb->pwm = pwm_request(data->pwm_id, "buzzer");
	if (IS_ERR(pb->pwm)) {
		dev_err(&pdev->dev, "unable to request PWM for buzzer\n");
		ret = PTR_ERR(pb->pwm);
		goto err_pwm;
	} else
		dev_dbg(&pdev->dev, "got pwm for buzzer\n");

    printk("[%s] pb->period = %u\n", __FUNCTION__, pb->period);

	platform_set_drvdata(pdev, pb);

    printk("%s is invoked!\n", __FUNCTION__);
	return 0;

err_pwm:
	kfree(pb);
err_alloc:
    if (data->exit)
        data->exit(&pdev->dev);
    return ret;
}

static int mars_pwm_buzzer_remove(struct platform_device *pdev)
{
	struct platform_pwm_buzzer_data *data = pdev->dev.platform_data;
	struct mars_pwm_buzzer_data *pb = dev_get_drvdata(&pdev->dev);
    
    pwm_config(pb->pwm, 0, pb->period);
    pwm_disable(pb->pwm);
    pwm_free(pb->pwm);
    kfree(pb);
	if (data->exit)
		data->exit(&pdev->dev);
    
    printk("%s is moving away!\n", __FUNCTION__);
    return 0;
}

static struct platform_driver mars_pwm_buzzer_driver = {
	.driver		= {
		.name	= "pwm-buzzer",
		.owner	= THIS_MODULE,
	},
	.probe		= mars_pwm_buzzer_probe,
	.remove		= mars_pwm_buzzer_remove,
};

static int __init mars_pwm_backlight_init(void)
{
	return platform_driver_register(&mars_pwm_buzzer_driver);
}
module_init(mars_pwm_backlight_init);

static void __exit mars_pwm_backlight_exit(void)
{
	platform_driver_unregister(&mars_pwm_buzzer_driver);
}
module_exit(mars_pwm_backlight_exit);

MODULE_DESCRIPTION("PWM based Buzzer Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-buzzer");

