#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>

struct kthread_test {
    struct task_struct *task;
    int private;
} kthread_test;

static int kthread_fn(void *p)
{
    struct kthread_test *kthread_test = p;
    printk("private:%d\n", kthread_test->private);

    while (!kthread_should_stop()) {
        printk("kthread running\n");
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ);
    }

    return 0;
}

static int kthread_test_init(void)
{
    kthread_test.private = 1;
    kthread_test.task =
        kthread_run(kthread_fn, &kthread_test, "%s", "kthread_test");
    if (IS_ERR(kthread_test.task)) {
        return PTR_ERR(kthread_test.task);
    }

    return 0;
}

static void kthread_test_exit(void)
{
    printk("kthread test exit\n");
    kthread_stop(kthread_test.task);
}

module_init(kthread_test_init);
module_exit(kthread_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("kthread test");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");