#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static int value = 0;
module_param(value, int, S_IRUGO);

struct gpio_consumer {
  struct gpio_desc *gpio;
};

static int gpio_consumer_probe(struct platform_device *pdev)
{
  struct gpio_consumer *gpio_consumer;

  printk("gpio consumer probe\n");
  gpio_consumer = devm_kzalloc(&pdev->dev, sizeof(struct gpio_consumer), GFP_KERNEL);
	if (!gpio_consumer) {
		dev_err(&pdev->dev, "Memory alloc failed\n");
		return -ENOMEM;
	}

  gpio_consumer->gpio = devm_gpiod_get_index_optional(&pdev->dev, NULL, 0, GPIOD_OUT_LOW);
  if (IS_ERR(gpio_consumer->gpio))
			return PTR_ERR(gpio_consumer->gpio);

  gpiod_set_value(gpio_consumer->gpio, value);

  platform_set_drvdata(pdev, gpio_consumer);

  return  0;
}

static int gpio_consumer_remove(struct platform_device *pdev)
{
  struct gpio_consumer *gpio_consumer;

  printk("gpio consumer remove\n");
  gpio_consumer = platform_get_drvdata(pdev);

  return 0;
}

static const struct of_device_id gpio_consumer_of_match[] = {
  { .compatible = "gpio-consumer" },
  {/* Sentinel */}
};

static struct platform_driver gpio_consumer_driver = {
  .probe = gpio_consumer_probe,
  .remove = gpio_consumer_remove,
  .driver = {
    .name = "gpio-consumer",
    .of_match_table = gpio_consumer_of_match,
  },
};

module_platform_driver(gpio_consumer_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("gpio consumer test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
