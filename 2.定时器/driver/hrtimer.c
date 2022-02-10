#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>

static struct hrtimer hrtimer;
static ktime_t kt;

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
  printk("hrtimer handler\n");
  hrtimer_forward(timer, timer->base->get_time(), kt);

  //return HRTIMER_NORESTART;//不重启定时器
  return HRTIMER_RESTART;//重启定时器
}

static int hrtimer_drv_init(void)
{
  printk("hrtimer drv init\n");

  kt = ktime_set(1, 10); /* 1 sec, 10 nsec */
  hrtimer_init(&hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  hrtimer.function = hrtimer_handler;
  hrtimer_start(&hrtimer, kt, HRTIMER_MODE_REL);

  return  0;
}

static void hrtimer_drv_exit(void)
{
  printk( "hrtimer drv exit\n");

  hrtimer_cancel(&hrtimer);
}

module_init(hrtimer_drv_init);
module_exit(hrtimer_drv_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("hrtimer driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
