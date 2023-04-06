#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>

static const struct of_device_id pinctrl_consumer_of_match[];
static int pinctrl_consumer_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;
    match = of_match_device(pinctrl_consumer_of_match, &pdev->dev);
    if (!match)
        return -EINVAL;
    if ((unsigned long)match->data) {
        struct gpio_desc *gpio =
            devm_gpiod_get_index_optional(&pdev->dev, NULL, 0, GPIOD_OUT_LOW);
        if (IS_ERR(gpio)) {
            return PTR_ERR(gpio);
        }
        gpiod_set_value(gpio, 0);
    }
    printk("%s\n", __FUNCTION__);
    return 0;
}

static int pinctrl_consumer_remove(struct platform_device *pdev)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

static const struct of_device_id pinctrl_consumer_of_match[] = {
    {.compatible = "xm,pinctrl-consumer"},
    {
        .compatible = "xm,pinctrl-consumer-gpio",
        .data = (void *)1,
    },
    {},
};

static struct platform_driver pinctrl_consumer_driver = {
    .probe = pinctrl_consumer_probe,
    .remove = pinctrl_consumer_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "pinctrl-consumer",
            .of_match_table = pinctrl_consumer_of_match,
        },
};

module_platform_driver(pinctrl_consumer_driver);
MODULE_LICENSE("GPL");