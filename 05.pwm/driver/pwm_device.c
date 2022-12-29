#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

static int pwmdevice_test_probe(struct platform_device *pdev)
{
    struct pwm_device *pwm;

    printk("pwmdevice test probe\n");
    pwm = pwm_get(&pdev->dev, NULL);
    if (IS_ERR(pwm)) {
        int err = PTR_ERR(pwm);
        dev_err(&pdev->dev, "pwm_get failed: %d\n", err);
        return err;
    }

    platform_set_drvdata(pdev, pwm);

    pwm_config(pwm, 500000, 1000000);
    pwm_enable(pwm);

    return 0;
}

static int pwmdevice_test_remove(struct platform_device *pdev)
{
    struct pwm_device *pwm;

    printk("pwmdevice test remove\n");
    pwm = platform_get_drvdata(pdev);
    pwm_disable(pwm);
    pwm_put(pwm);

    return 0;
}

static const struct of_device_id pwmdevice_test_of_match[] = {
    {.compatible = "pwmdevice-test"}, {/* Sentinel */}};

static struct platform_driver pwmdevice_test_driver = {
    .probe = pwmdevice_test_probe,
    .remove = pwmdevice_test_remove,
    .driver =
        {
            .name = "pwmdevice-test",
            .of_match_table = pwmdevice_test_of_match,
        },
};

module_platform_driver(pwmdevice_test_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("pwmdevice test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
