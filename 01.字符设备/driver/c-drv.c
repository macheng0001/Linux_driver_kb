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

/* 设备节点名称 */
#define DEVICE_NAME "char_dev"
/* 设备号个数 */
#define DEVID_COUNT 1
/* 驱动个数 */
#define DRIVE_COUNT 1
/* 主设备号 */
#define DEV_MAJOR
/* 次设备号 */
#define DEV_MINOR 0

#define MEM_SIZE 1024

struct char_dev {
    dev_t devid;           //设备号
    struct cdev cdev;      //字符设备
    struct class *class;   //类
    struct device *device; //设备节点
    unsigned char mem[MEM_SIZE];
};

static struct char_dev char_dev = {
    .cdev =
        {
            .owner = THIS_MODULE,
        },
};

static int char_dev_open(struct inode *inode, struct file *file)
{
    struct char_dev *dev = container_of(inode->i_cdev, struct char_dev, cdev);
    struct char_dev *drvdata = dev_get_drvdata(dev->device);

    printk("device name:%s\n", dev_name(dev->device));
    printk("driver data device name:%s\n", dev_name(drvdata->device));
    printk("%s\n", __func__);

    file->private_data = dev;
    memset(dev->mem, 0, sizeof(dev->mem));
    return 0;
}

static ssize_t char_dev_write(struct file *file, const char __user *buf,
                              size_t len, loff_t *loff_t)
{
    int ret;
    unsigned long p = *loff_t;
    struct char_dev *dev = file->private_data;

    printk("%s\n", __func__);
    printk("file private data---device name:%s\n", dev_name(dev->device));

    if (p >= MEM_SIZE)
        return 0;
    if (len > MEM_SIZE - p)
        len = MEM_SIZE - p;

    ret = copy_from_user(dev->mem + p, buf, len);
    if (ret == 0) {
        *loff_t += len;
        ret = len;
        printk("witten %u bytes from %lu\n", len, p);
    } else {
        ret = -EFAULT;
    }

    return ret;
}

static ssize_t char_dev_read(struct file *file, char __user *buf, size_t len,
                             loff_t *loff_t)
{
    int ret;
    unsigned long p = *loff_t;
    struct char_dev *dev = file->private_data;

    printk("%s\n", __func__);
    printk("file private data---device name:%s\n", dev_name(dev->device));

    if (p >= MEM_SIZE)
        return 0;
    if (len > MEM_SIZE - p)
        len = MEM_SIZE - p;

    ret = copy_to_user(buf, dev->mem + p, len);
    if (ret == 0) {
        *loff_t += len;
        ret = len;
        printk("read %u bytes from %lu\n", len, p);
    } else {
        ret = -EFAULT;
    }

    return ret;
}

static loff_t char_dev_llseek(struct file *file, loff_t offset, int orig)
{
    loff_t ret = -EINVAL;

    switch (orig) {
    case SEEK_SET:
        if (offset < 0 || offset >= MEM_SIZE)
            break;
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (file->f_pos + offset >= MEM_SIZE || file->f_pos + offset < 0)
            break;
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        break;
    }

    return ret;
}

static int char_dev_release(struct inode *inode, struct file *file)
{
    printk("%s\n", __func__);
    return 0;
}

static struct file_operations char_dev_fops = {
    .owner = THIS_MODULE,
    .open = char_dev_open,
    .write = char_dev_write,
    .read = char_dev_read,
    .llseek = char_dev_llseek,
    .release = char_dev_release,
};

/* 模块加载时会调用的函数 */
static int __init char_dev_init(void)
{
    int ret;

    /* 注册设备号 */
    if (alloc_chrdev_region(&char_dev.devid, DEV_MINOR, DEVID_COUNT,
                            DEVICE_NAME)) {
        printk("unable to allocate chrdev region\n");
        return -EFAULT;
    }

    /* 初始化字符设备结构体 */
    cdev_init(&char_dev.cdev, &char_dev_fops);

    /* 注册字符设备 */
    if (cdev_add(&char_dev.cdev, char_dev.devid, DRIVE_COUNT)) {
        ret = -EFAULT;
        goto err_chrdev_unreg;
    }

    /* 创建类 */
    char_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(char_dev.class)) {
        ret = PTR_ERR(char_dev.class);
        goto err_cdev_del;
    }

    /* 创建设备节点 */
    char_dev.device = device_create(char_dev.class, NULL, char_dev.devid,
                                    &char_dev, DEVICE_NAME);
    if (IS_ERR(char_dev.device)) {
        ret = PTR_ERR(char_dev.device);
        goto err_class_destr;
    }

    printk("char dev init ok\n");

    return 0;
err_class_destr:
    class_destroy(char_dev.class);
err_cdev_del:
    cdev_del(&char_dev.cdev);
err_chrdev_unreg:
    unregister_chrdev_region(char_dev.devid, DEVID_COUNT);
    return ret;
}

/* 卸载模块 */
static void __exit char_dev_exit(void)
{
    /* 注销字符设备 */
    cdev_del(&char_dev.cdev);

    /* 注销设备号 */
    unregister_chrdev_region(char_dev.devid, DEVID_COUNT);

    /* 删除设备节点 */
    device_destroy(char_dev.class, char_dev.devid);

    /* 删除类 */
    class_destroy(char_dev.class);

    printk("char dev exit ok\n");
}

/* 标记加载、卸载函数 */
module_init(char_dev_init);
module_exit(char_dev_exit);

/* 驱动描述信息 */
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("char device driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
