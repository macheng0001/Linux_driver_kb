#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/input.h>

struct input_device_data {
    struct gpio_desc *gpio;
    struct timer_list timer;
    atomic_t key_value;
    struct input_dev *inputdev;
    unsigned int code;
};

static void debounce_timer_handler(unsigned long data)
{
    struct input_device_data *p = (void *)data;
    int value;

    value = gpiod_get_value(p->gpio);
    input_report_key(p->inputdev, p->code, value);
    input_sync(p->inputdev);
}

static irqreturn_t gpio_interrupt_handler(int irq, void *dev)
{
    struct input_device_data *p = dev;

    printk("gpio interrupt handler\n");
    mod_timer(&p->timer, jiffies + msecs_to_jiffies(50));

    return IRQ_HANDLED;
}

static int input_device_probe(struct platform_device *pdev)
{
    struct input_device_data *p;
    int ret;

    printk("input device probe\n");
    p = devm_kzalloc(&pdev->dev, sizeof(struct input_device_data), GFP_KERNEL);
    if (!p) {
        dev_err(&pdev->dev, "Memory alloc failed\n");
        return -ENOMEM;
    }

    p->gpio = devm_gpiod_get_index_optional(&pdev->dev, NULL, 0, GPIOD_IN);
    if (IS_ERR(p->gpio))
        return PTR_ERR(p->gpio);

    ret = devm_request_irq(
        &pdev->dev, gpiod_to_irq(p->gpio), gpio_interrupt_handler,
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "input_device", p);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to request IRQ: %d\n", ret);
        return ret;
    }

    setup_timer(&p->timer, debounce_timer_handler, (unsigned long)p);

    p->inputdev = devm_input_allocate_device(&pdev->dev);
    if (!p->inputdev) {
        dev_err(&pdev->dev, "failed to allocate input device\n");
        return -ENOMEM;
    }

    p->code = BTN_0;
    p->inputdev->name = "input_button";
    __set_bit(EV_REP, p->inputdev->evbit);
    __set_bit(EV_KEY, p->inputdev->evbit);
    __set_bit(BTN_0, p->inputdev->keybit);

    ret = input_register_device(p->inputdev);
    if (ret) {
        dev_err(&pdev->dev, "Unable to register input device, error: %d\n",
                ret);
        return ret;
    }

    platform_set_drvdata(pdev, p);

    return 0;
}

static int input_device_remove(struct platform_device *pdev)
{
    struct input_device_data *p;

    printk("input device remove\n");
    p = platform_get_drvdata(pdev);
    input_unregister_device(p->inputdev);

    return 0;
}

static const struct of_device_id input_device_of_match[] = {
    {.compatible = "input-device"}, {/* Sentinel */}};

static struct platform_driver input_device_driver = {
    .probe = input_device_probe,
    .remove = input_device_remove,
    .driver =
        {
            .name = "input-device",
            .of_match_table = input_device_of_match,
        },
};

module_platform_driver(input_device_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("input device test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
