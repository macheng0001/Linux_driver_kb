#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

struct test_pwmchip {
  struct pwm_chip chip;
};

static int pwmchip_config(struct pwm_chip *chip, struct pwm_device *pwm, int duty_ns, int period_ns)
{
  printk("pwmchip_config duty:%d, period:%d\n", duty_ns, period_ns);

  return 0;
}

static int pwmchip_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
  printk("pwmchip_enable\n");

  return 0;
}

static void pwmchip_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
  printk("pwmchip_disable\n");
}

static const struct pwm_ops pwmchip_ops = {
  .owner = THIS_MODULE,
  .config = pwmchip_config,
  .enable = pwmchip_enable,
  .disable = pwmchip_disable,
};

static int pwmchip_test_probe(struct platform_device *pdev)
{
  int ret;
  struct test_pwmchip *pc;

  printk("pwmchip test probe\n");
  pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
  if (!pc)
    return -ENOMEM;

  pc->chip.dev = &pdev->dev;
  pc->chip.ops = &pwmchip_ops;
  pc->chip.base = -1;
  pc->chip.npwm = 2;

  platform_set_drvdata(pdev, pc);

  ret = pwmchip_add(&pc->chip);
  if (ret < 0) {
    return ret;
  }

  return  0;
}

static int pwmchip_test_remove(struct platform_device *pdev)
{
  struct test_pwmchip *pc;

  printk("pwmchip test remove\n");
  pc = platform_get_drvdata(pdev);

  return pwmchip_remove(&pc->chip);
}

static const struct of_device_id pwmchip_test_of_match[] = {
  { .compatible = "pwmchip-test" },
  {/* Sentinel */}
};

static struct platform_driver pwmchip_test_driver = {
  .probe = pwmchip_test_probe,
  .remove = pwmchip_test_remove,
  .driver = {
    .name = "pwmchip-test",
    .of_match_table = pwmchip_test_of_match,
  },
};

module_platform_driver(pwmchip_test_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("pwmchip test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
