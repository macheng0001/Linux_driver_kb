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
#include <linux/miscdevice.h>

struct fasync_io_data {
    struct gpio_desc *gpio;
    struct timer_list timer;
    wait_queue_head_t wq;
    atomic_t key_value;
    struct mutex mutex;
    struct fasync_struct *fasync;
    struct miscdevice miscdev;
};

static int char_dev_open(struct inode *inode, struct file *file)
{
    struct miscdevice *miscdev = file->private_data;
    struct fasync_io_data *p = dev_get_drvdata(miscdev->this_device);

    file->private_data = p;

    return 0;
}

static ssize_t char_dev_read(struct file *file, char __user *buf, size_t len,
                             loff_t *loff_t)
{
    struct fasync_io_data *p = file->private_data;
    int value;

    value = atomic_read(&p->key_value);
    if (!value) {
        if (file->f_flags & O_NONBLOCK) { /*非阻塞*/
            return -EAGAIN;
        }
#if 0
        DECLARE_WAITQUEUE(wait, current);
        add_wait_queue(&p->wq, &wait);
        __set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        set_current_state(TASK_RUNNING);
        remove_wait_queue(&p->wq, &wait);
        if (signal_pending(current)) {
            printk("signal_pending\n");
            return -ERESTARTSYS;
        }
#else
        if (wait_event_interruptible(p->wq, atomic_read(&p->key_value))) {
            printk("signal_pending\n");
            return -ERESTARTSYS;
        }
#endif
    }

    value = atomic_read(&p->key_value);
    atomic_set(&p->key_value, 0);

    return copy_to_user(buf, &value, sizeof(value));
}

unsigned int char_dev_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct fasync_io_data *p = file->private_data;

    poll_wait(file, &p->wq, wait);

    if (atomic_read(&p->key_value)) {
        mask |= POLLIN;
    } else {
    }

    return mask;
}

static int char_dev_fasync(int fd, struct file *file, int mode)
{
    struct fasync_io_data *p = file->private_data;

    return fasync_helper(fd, file, mode, &p->fasync);
}

static int char_dev_release(struct inode *inode, struct file *file)
{
    printk("char dev release\n");

    return char_dev_fasync(-1, file, 0);
}

static const struct file_operations char_dev_fops = {
    .owner = THIS_MODULE,
    .open = char_dev_open,
    .read = char_dev_read,
    .poll = char_dev_poll,
    .fasync = char_dev_fasync,
    .release = char_dev_release,
};

static void debounce_timer_handler(unsigned long data)
{
    struct fasync_io_data *p = (void *)data;
    int value;

    value = gpiod_get_value(p->gpio);
    if (value == 0) {
        atomic_set(&p->key_value, 1);
        if (p->fasync) {
            kill_fasync(&p->fasync, SIGIO, POLL_IN);
        } else {
            wake_up_interruptible(&p->wq);
        }
    }
}

static irqreturn_t gpio_interrupt_handler(int irq, void *dev)
{
    struct fasync_io_data *p = dev;

    printk("gpio interrupt handler\n");
    mod_timer(&p->timer, jiffies + msecs_to_jiffies(50));

    return IRQ_HANDLED;
}

static int fasync_io_probe(struct platform_device *pdev)
{
    struct fasync_io_data *p;
    int ret;

    printk("fasync io probe\n");
    p = devm_kzalloc(&pdev->dev, sizeof(struct fasync_io_data), GFP_KERNEL);
    if (!p) {
        dev_err(&pdev->dev, "Memory alloc failed\n");
        return -ENOMEM;
    }

    p->gpio = devm_gpiod_get_index_optional(&pdev->dev, NULL, 0, GPIOD_IN);
    if (IS_ERR(p->gpio))
        return PTR_ERR(p->gpio);

    ret = devm_request_irq(&pdev->dev, gpiod_to_irq(p->gpio),
                           gpio_interrupt_handler, IRQF_TRIGGER_RISING,
                           "fasync_io", p);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to request IRQ: %d\n", ret);
        return ret;
    }

    setup_timer(&p->timer, debounce_timer_handler, (unsigned long)p);
    init_waitqueue_head(&p->wq);
    mutex_init(&p->mutex);

    p->miscdev.minor = MISC_DYNAMIC_MINOR;
    p->miscdev.name = "fasync_io";
    p->miscdev.fops = &char_dev_fops;
    ret = misc_register(&p->miscdev);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to register miscdev: %d\n", ret);
        return ret;
    }
    dev_set_drvdata(p->miscdev.this_device, p);

    platform_set_drvdata(pdev, p);

    return 0;
}

static int fasync_io_remove(struct platform_device *pdev)
{
    struct fasync_io_data *p;

    printk("fasync io remove\n");
    p = platform_get_drvdata(pdev);
    misc_deregister(&p->miscdev);

    return 0;
}

static const struct of_device_id fasync_io_of_match[] = {
    {.compatible = "fasync-io"}, {/* Sentinel */}};

static struct platform_driver fasync_io_driver = {
    .probe = fasync_io_probe,
    .remove = fasync_io_remove,
    .driver =
        {
            .name = "fasync-io",
            .of_match_table = fasync_io_of_match,
        },
};

module_platform_driver(fasync_io_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("miscdevice test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
