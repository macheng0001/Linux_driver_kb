#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>

static int type = 0;
module_param(type, int, S_IRUGO);

enum bottom_half_type {
  BOTTOM_HALF_NONE,
  BOTTOM_HALF_TASKLET,
  BOTTOM_HALF_WORKQUEUE,
  BOTTOM_HALF_THREAD,
  BOTTOM_HALF_MAX,
};

struct gpio_interrupt_data {
  struct device *dev;
  struct gpio_desc *gpio;
  struct tasklet_struct tasklet;
  struct work_struct work;
  enum bottom_half_type type;
};

static irqreturn_t gpio_interrupt_handler(int irq, void *dev)
{
  struct gpio_interrupt_data *p = dev;

  printk("gpio interrupt handler:%d\n", p->type);

  if (p->type == BOTTOM_HALF_TASKLET) {
    tasklet_schedule(&p->tasklet);
  } else if (type == BOTTOM_HALF_WORKQUEUE) {
    schedule_work(&p->work);
  }

  if (p->type == BOTTOM_HALF_THREAD)
    return IRQ_WAKE_THREAD;
  else
    return IRQ_HANDLED;
}

static irqreturn_t threaded_irq_handler(int irq, void *data)
{
  struct gpio_interrupt_data *p = data;

  printk("threaded irq handler:%d\n", p->type);

  return IRQ_HANDLED;
}

static void tasklet_handler(unsigned long data)
{
  struct gpio_interrupt_data *p = (void *)data;

  printk("tasklet handler:%d\n", p->type);
}

static void workqueue_handler(struct work_struct *work)
{
  struct gpio_interrupt_data *p = container_of(work, struct gpio_interrupt_data, work);

  printk("workqueue handler:%d\n", p->type);
}

static int gpio_interrupt_probe(struct platform_device *pdev)
{
  struct gpio_interrupt_data *p;
  int ret;

  printk("gpio interrupt probe\n");
  p = devm_kzalloc(&pdev->dev, sizeof(struct gpio_interrupt_data), GFP_KERNEL);
	if (!p) {
		dev_err(&pdev->dev, "Memory alloc failed\n");
		return -ENOMEM;
	}

  p->gpio = devm_gpiod_get_index_optional(&pdev->dev, NULL, 0, GPIOD_IN);
  if (IS_ERR(p->gpio))
			return PTR_ERR(p->gpio);

  p->dev = &pdev->dev;
  p->type = type;

  if (type == BOTTOM_HALF_THREAD) {
    ret = devm_request_threaded_irq(p->dev, gpiod_to_irq(p->gpio), gpio_interrupt_handler,
              threaded_irq_handler, IRQF_TRIGGER_RISING, "gpio_interrupt", p);
    if (ret < 0){
      dev_err(&pdev->dev, "Failed to request IRQ: %d\n", ret);
      return -1;
    }
  } else {
    ret = devm_request_irq(p->dev, gpiod_to_irq(p->gpio), gpio_interrupt_handler,
              IRQF_TRIGGER_RISING, "gpio_interrupt", p);
    if (ret < 0){
      dev_err(&pdev->dev, "Failed to request IRQ: %d\n", ret);
      return -1;
    }
  }

  if (type == BOTTOM_HALF_TASKLET) {
    tasklet_init(&p->tasklet, tasklet_handler, (unsigned long)p);
  } else if (type == BOTTOM_HALF_WORKQUEUE) {
    INIT_WORK(&p->work, workqueue_handler);
  }

  platform_set_drvdata(pdev, p);

  return  0;
}

static int gpio_interrupt_remove(struct platform_device *pdev)
{
  struct gpio_interrupt_data *p;

  printk("gpio interrupt remove\n");
  p = platform_get_drvdata(pdev);

  return 0;
}

static const struct of_device_id gpio_interrupt_of_match[] = {
  { .compatible = "gpio-interrupt" },
  {/* Sentinel */}
};

static struct platform_driver gpio_interrupt_driver = {
  .probe = gpio_interrupt_probe,
  .remove = gpio_interrupt_remove,
  .driver = {
    .name = "gpio-interrupt",
    .of_match_table = gpio_interrupt_of_match,
  },
};

module_platform_driver(gpio_interrupt_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("gpio interrupt test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
