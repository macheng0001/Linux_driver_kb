#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/slab.h>

static int type = 0;
module_param(type, int, S_IRUGO);

enum concurrency_control_type {
    CONCURRENCY_CONTROL_NULL,
    CONCURRENCY_CONTROL_SPINLOCK,
    CONCURRENCY_CONTROL_NUTEX,
    CONCURRENCY_CONTROL_MAX,
};

#define THREAD_CNT 2
#define TEST_CNT 10

struct concurrency_control_pdata {
    struct completion completion[THREAD_CNT];
    struct task_struct *thread[THREAD_CNT];
    unsigned int result;
    spinlock_t lock;
    struct mutex mutex;
} pdata;

static int thread_func(void *arg)
{
    int i;
    struct completion *c = (struct completion *)arg;

    for (i = 0; i < 2000000; i++) {
        udelay(1);
        if (type == CONCURRENCY_CONTROL_SPINLOCK)
            spin_lock(&pdata.lock);
        else if (type == CONCURRENCY_CONTROL_NUTEX)
            mutex_lock(&pdata.mutex);

        pdata.result++;

        if (type == CONCURRENCY_CONTROL_SPINLOCK)
            spin_unlock(&pdata.lock);
        else if (type == CONCURRENCY_CONTROL_NUTEX)
            mutex_unlock(&pdata.mutex);
    }
    complete(c);

    return 0;
}

static int __init concurrency_control_init(void)
{
    int i, j;

    for (i = 0; i < THREAD_CNT; i++) {
        init_completion(&pdata.completion[i]);
    }

    mutex_init(&pdata.mutex);
    spin_lock_init(&pdata.lock);

    for (j = 0; j < TEST_CNT; j++) {
        pdata.result = 0;
        for (i = 0; i < THREAD_CNT; i++) {
            pdata.thread[i] =
                kthread_run(thread_func, &pdata.completion[i], "test");
            if (IS_ERR(pdata.thread[i])) {
                return PTR_ERR(pdata.thread[i]);
            }
        }
        for (i = 0; i < THREAD_CNT; i++) {
            wait_for_completion(&pdata.completion[i]);
        }
        printk("result[%d]:%d\n", j, pdata.result);
    }

    return 0;
}

static void __exit concurrency_control_exit(void)
{
}

module_init(concurrency_control_init);
module_exit(concurrency_control_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("concurrency control test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");