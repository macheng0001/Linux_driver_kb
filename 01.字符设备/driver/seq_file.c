#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ide.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "seqfile_test"
#define DEVID_COUNT 1
#define DRIVE_COUNT 1
#define DEV_MINOR 0

struct seqfile_test {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

struct seqfile_entry {
    char *str;
    int num;
};

static struct seqfile_test seqfile_test = {
    .cdev =
        {
            .owner = THIS_MODULE,
        },
};

#define SEQ_FILE_ENTRY(s, n)                                                   \
    {                                                                          \
        .str = #s, .num = n,                                                   \
    }

struct seqfile_entry seqfile_entrys[] = {
    SEQ_FILE_ENTRY(first, 1),
    SEQ_FILE_ENTRY(second, 2),
    SEQ_FILE_ENTRY(third, 3),
};

static int seq_show(struct seq_file *seq, void *v)
{
    int i;
    printk("seq_show\n");
    i = (int)*(loff_t *)v;
    seq_setwidth(seq, 63); //设置填充宽度
    seq_printf(seq, "%s: %d", seqfile_entrys[i].str, seqfile_entrys[i].num);
    seq_pad(seq, '\n'); //填充空格
    return 0;
}

static void *seq_start(struct seq_file *seq, loff_t *pos)
{
    return (*pos >= ARRAY_SIZE(seqfile_entrys)) ? NULL : (void *)pos;
}

static void seq_stop(struct seq_file *seq, void *v)
{
}

static void *seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
    ++*pos;
    return (*pos >= ARRAY_SIZE(seqfile_entrys)) ? NULL : (void *)pos;
}

static const struct seq_operations seq_ops = {
    .start = seq_start,
    .next = seq_next,
    .stop = seq_stop,
    .show = seq_show,
};

static int seqfile_test_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &seq_ops);
}

static struct file_operations seqfile_test_fops = {
    .owner = THIS_MODULE,
    .open = seqfile_test_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

static int __init seqfile_test_init(void)
{
    int ret;

    if (alloc_chrdev_region(&seqfile_test.devid, DEV_MINOR, DEVID_COUNT,
                            DEVICE_NAME)) {
        printk("unable to allocate chrdev region\n");
        return -EFAULT;
    }

    cdev_init(&seqfile_test.cdev, &seqfile_test_fops);

    if (cdev_add(&seqfile_test.cdev, seqfile_test.devid, DRIVE_COUNT)) {
        ret = -EFAULT;
        goto err_chrdev_unreg;
    }

    seqfile_test.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(seqfile_test.class)) {
        ret = PTR_ERR(seqfile_test.class);
        goto err_cdev_del;
    }

    seqfile_test.device =
        device_create(seqfile_test.class, NULL, seqfile_test.devid,
                      &seqfile_test, DEVICE_NAME);
    if (IS_ERR(seqfile_test.device)) {
        ret = PTR_ERR(seqfile_test.device);
        goto err_class_destr;
    }

    return 0;
err_class_destr:
    class_destroy(seqfile_test.class);
err_cdev_del:
    cdev_del(&seqfile_test.cdev);
err_chrdev_unreg:
    unregister_chrdev_region(seqfile_test.devid, DEVID_COUNT);
    return ret;
}

static void __exit seqfile_test_exit(void)
{
    cdev_del(&seqfile_test.cdev);
    unregister_chrdev_region(seqfile_test.devid, DEVID_COUNT);
    device_destroy(seqfile_test.class, seqfile_test.devid);
    class_destroy(seqfile_test.class);
}

module_init(seqfile_test_init);
module_exit(seqfile_test_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("seq file test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
