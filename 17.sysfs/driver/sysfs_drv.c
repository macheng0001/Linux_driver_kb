#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

struct sysfs_drv_priv {
    struct kobject *kobject;
    struct kobj_attribute kobj_attr;
    int attr_value;
    struct attribute_group attr_group;
    struct attribute *attr_array[2];
} sysfs_priv;

static ssize_t sysfs_file0_show(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf)
{
    struct sysfs_drv_priv *priv =
        container_of(attr, struct sysfs_drv_priv, kobj_attr);

    return sprintf(buf, "%d\n", priv->attr_value);
}

static ssize_t sysfs_file0_store(struct kobject *kobj,
                                 struct kobj_attribute *attr, const char *buf,
                                 size_t count)
{
    struct sysfs_drv_priv *priv =
        container_of(attr, struct sysfs_drv_priv, kobj_attr);

    sscanf(buf, "%du", &priv->attr_value);

    return count;
}

static int sysfs_file1_value = 64;
static ssize_t sysfs_file1_show(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", sysfs_file1_value);
}

static ssize_t sysfs_file1_store(struct kobject *kobj,
                                 struct kobj_attribute *attr, const char *buf,
                                 size_t count)
{

    sscanf(buf, "%du", &sysfs_file1_value);

    return count;
}

//利用宏初始化属性
static struct kobj_attribute kobj_attr =
    __ATTR(sysfs_file1, 0644, sysfs_file1_show, sysfs_file1_store);

//初始化属性列表
static struct attribute *test_attrs[] = {
    &kobj_attr.attr,
    NULL,
};
//利用宏初始化属性组
ATTRIBUTE_GROUPS(test);

static int sysfs_drv_init(void)
{
    int ret;

    printk("sysfs drv init\n");

    //不利用宏初始化属性
    sysfs_priv.kobj_attr.attr.name = "sysfs_file0";
    sysfs_priv.kobj_attr.attr.mode = 0644;
    sysfs_priv.kobj_attr.show = sysfs_file0_show;
    sysfs_priv.kobj_attr.store = sysfs_file0_store;
    sysfs_priv.attr_value = 128;

    sysfs_priv.attr_group.name = "sysfs_group";
    sysfs_priv.attr_array[0] = &sysfs_priv.kobj_attr.attr;
    sysfs_priv.attr_array[1] = NULL;
    //不利用宏初始化属性组
    sysfs_priv.attr_group.attrs = sysfs_priv.attr_array;

    //创建/sys/sysfs_path/目录
    sysfs_priv.kobject = kobject_create_and_add("sysfs_path", NULL);
    if (!sysfs_priv.kobject) {
        ret = PTR_ERR(sysfs_priv.kobject);
        goto err_kobject_create;
    }

    //在/sys/sysfs_path/目录下创建sysfs_file0文件
    ret = sysfs_create_file(sysfs_priv.kobject, &sysfs_priv.kobj_attr.attr);
    if (ret < 0) {
        goto err_create_file0;
    }

    //创建/sys/sysfs_path/sysfs_group目录后,在/sys/sysfs_path/sysfs_group目录后目录下创建sysfs_file0文件
    ret = sysfs_create_group(sysfs_priv.kobject, &sysfs_priv.attr_group);
    if (ret < 0) {
        goto err_create_group;
    }

    //在/sys/sysfs_path/目录下创建sysfs_file1文件
    ret = sysfs_create_groups(sysfs_priv.kobject, test_groups);
    if (ret < 0) {
        goto err_create_groups;
    }

    return 0;
err_create_groups:
    sysfs_remove_group(sysfs_priv.kobject, &sysfs_priv.attr_group);
err_create_group:
    sysfs_remove_file(sysfs_priv.kobject, &sysfs_priv.kobj_attr.attr);
err_create_file0:
    kobject_del(sysfs_priv.kobject);
err_kobject_create:
    return ret;
}

static void sysfs_drv_exit(void)
{
    printk("sysfs drv exit\n");
    sysfs_remove_groups(sysfs_priv.kobject, test_groups);
    sysfs_remove_group(sysfs_priv.kobject, &sysfs_priv.attr_group);
    sysfs_remove_file(sysfs_priv.kobject, &sysfs_priv.kobj_attr.attr);
    kobject_put(sysfs_priv.kobject);
}

module_init(sysfs_drv_init);
module_exit(sysfs_drv_exit);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("sysfs test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
