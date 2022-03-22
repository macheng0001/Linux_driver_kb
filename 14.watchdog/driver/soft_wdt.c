#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/types.h>
#include <linux/slab.h>

static unsigned int softdog_num = 2;
module_param(softdog_num, uint, 0);
MODULE_PARM_DESC(softdog_num, "software watchdog number. (default=2)");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(
    nowayout,
    "Watchdog cannot be stopped once started (default=" __MODULE_STRING(
        WATCHDOG_NOWAYOUT) ")");

static unsigned int soft_margin = 10;
module_param(soft_margin, uint, 0);
MODULE_PARM_DESC(soft_margin,
                 "Watchdog soft_margin in seconds. (0 < soft_margin < 65536, "
                 "default=10)");

struct softdog_data {
    struct watchdog_device wdd;
    struct timer_list fire_timer;
    struct timer_list pre_timer;
};

static void softdog_fire(unsigned long data)
{
    module_put(THIS_MODULE);
    pr_crit("Initiating system reboot\n");
    emergency_restart();
    pr_crit("Reboot didn't ?????\n");
}

static void softdog_pretimeout(unsigned long data)
{
    struct softdog_data *softdog = (struct softdog_data *)data;

    watchdog_notify_pretimeout(&softdog->wdd);
}

static int softdog_ping(struct watchdog_device *w)
{
    struct softdog_data *softdog = watchdog_get_drvdata(w);

    if (!mod_timer(&softdog->fire_timer, jiffies + (w->timeout * HZ)))
        __module_get(THIS_MODULE);

    if (w->pretimeout)
        mod_timer(&softdog->pre_timer,
                  jiffies + (w->timeout - w->pretimeout) * HZ);
    else
        del_timer(&softdog->pre_timer);

    return 0;
}

static int softdog_stop(struct watchdog_device *w)
{
    struct softdog_data *softdog = watchdog_get_drvdata(w);

    if (del_timer(&softdog->fire_timer))
        module_put(THIS_MODULE);

    del_timer(&softdog->pre_timer);

    return 0;
}

static const struct watchdog_ops softdog_ops = {
    .owner = THIS_MODULE,
    .start = softdog_ping,
    .stop = softdog_stop,
};

static struct watchdog_info softdog_info = {
    .identity = "Software Watchdog",
    .options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE |
               WDIOF_PRETIMEOUT,
};

static struct softdog_data *softdog;
static int softdog_init(void)
{
    unsigned int i, j;
    int ret;
    struct watchdog_device *wdd;

    softdog = kcalloc(softdog_num, sizeof(struct softdog_data), GFP_KERNEL);
    if (!softdog) {
        pr_err("Memory alloc failed\n");
        return -ENOMEM;
    }

    for (i = 0; i < softdog_num; i++) {
        wdd = &softdog[i].wdd;
        wdd->info = &softdog_info;
        wdd->ops = &softdog_ops;
        wdd->min_timeout = 1;
        wdd->max_timeout = 65535;

        watchdog_init_timeout(wdd, soft_margin, NULL);
        watchdog_set_nowayout(wdd, nowayout);
        watchdog_stop_on_reboot(wdd);

        setup_timer(&softdog[i].fire_timer, softdog_fire,
                    (unsigned long)&softdog[i]);
        setup_timer(&softdog[i].pre_timer, softdog_pretimeout,
                    (unsigned long)&softdog[i]);

        watchdog_set_drvdata(wdd, &softdog[i]);

        ret = watchdog_register_device(wdd);
        if (ret)
            goto err;

        pr_info("initialized. soft_margin=%d sec"
                "(nowayout=%d)\n",
                wdd->timeout, nowayout);
    }

    printk("soft wdt probe\n");

    return 0;

err:
    for (j = 0; j < i; j++) {
        wdd = &softdog[j].wdd;
        watchdog_unregister_device(wdd);
    }
    kfree(softdog);
    return ret;
}

module_init(softdog_init);

static void __exit softdog_exit(void)
{
    unsigned int i;
    struct watchdog_device *wdd;

    for (i = 0; i < softdog_num; i++) {
        wdd = &softdog[i].wdd;
        watchdog_unregister_device(wdd);
    }
    kfree(softdog);
}

module_exit(softdog_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("software watchdog driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
