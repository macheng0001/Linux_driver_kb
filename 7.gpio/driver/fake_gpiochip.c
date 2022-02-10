#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

struct fake_gpiochip {
  struct gpio_chip chip;
};

static int fake_gpiochip_request(struct gpio_chip *chip, unsigned offset)
{
  printk("fake gpiochip request\n");
  return 0;
}

static void fake_gpiochip_free(struct gpio_chip *chip, unsigned offset)
{
  printk("fake gpiochip free\n");
}

static int fake_gpiochip_get_value(struct gpio_chip *chip, unsigned offset)
{
  printk("fake gpiochip get value\n");
  return 0;
}

static void fake_gpiochip_set_value(struct gpio_chip *chip, unsigned offset, int value)
{
  printk("fake gpiochip set value\n");
}

static int fake_gpiochip_get_direction(struct gpio_chip *chip, unsigned offset)
{
  printk("fake gpiochip get direction\n");
  return 0;
}

static int fake_gpiochip_direction_input(struct gpio_chip *chip, unsigned offset)
{
  printk("fake gpiochip set direction input\n");
  return 0;
}

static int fake_gpiochip_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
  printk("fake gpiochip set direction output\n");
  return 0;
}

static int fake_gpiochip_probe(struct platform_device *pdev)
{
  struct fake_gpiochip *fake_chip;
  int ret;

  printk("fake gpiochip probe\n");
  fake_chip = devm_kzalloc(&pdev->dev, sizeof(struct fake_gpiochip), GFP_KERNEL);
	if (!fake_chip) {
		dev_err(&pdev->dev, "Memory alloc failed\n");
		return -ENOMEM;
	}

  fake_chip->chip.label = pdev->name;
  fake_chip->chip.base = -1;
  fake_chip->chip.owner = THIS_MODULE;
  fake_chip->chip.ngpio = 8;
  fake_chip->chip.can_sleep = 1;
  fake_chip->chip.request = fake_gpiochip_request;
  fake_chip->chip.free = fake_gpiochip_free;
  fake_chip->chip.get = fake_gpiochip_get_value;
  fake_chip->chip.set = fake_gpiochip_set_value;
  fake_chip->chip.get_direction = fake_gpiochip_get_direction;
  fake_chip->chip.direction_input = fake_gpiochip_direction_input;
  fake_chip->chip.direction_output = fake_gpiochip_direction_output;

  ret = gpiochip_add(&fake_chip->chip);
	if (ret) {
		dev_err(&pdev->dev,
			"Could not register gpio chip %d\n", ret);
		return ret;
	}

  platform_set_drvdata(pdev, fake_chip);

  return  0;
}

static int fake_gpiochip_remove(struct platform_device *pdev)
{
  struct fake_gpiochip *fake_chip;

  fake_chip = platform_get_drvdata(pdev);
  gpiochip_remove(&fake_chip->chip);

  return 0;
}

static const struct of_device_id fake_gpiochip_of_match[] = {
  { .compatible = "fake-gpio-chip" },
  {/* Sentinel */}
};

static struct platform_driver fake_gpiochip_driver = {
  .probe = fake_gpiochip_probe,
  .remove = fake_gpiochip_remove,
  .driver = {
    .name = "fake-gpio-chip",
    .of_match_table = fake_gpiochip_of_match,
  },
};

module_platform_driver(fake_gpiochip_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("gpio chip test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
