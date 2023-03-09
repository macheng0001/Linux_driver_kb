#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#define VIRT_MTD_SIZE 32 * 1024
struct virt_mtd_priv {
    struct mtd_info mtd;
    char buffer[VIRT_MTD_SIZE];
};

static struct mtd_partition virt_mtd_partitions[] = {
    {
        .name = "virt_mtd_part1",
        .size = 16 * 1024,
        .offset = 0,
    },
    {
        .name = "virt_mtd_part2",
        .size = MTDPART_SIZ_FULL,
        .offset = MTDPART_OFS_APPEND,
    },
};

static int virt_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct virt_mtd_priv *priv = mtd->priv;

    printk("%s,%lld,%lld\n", __func__, instr->addr, instr->len);
    /* Start address must align on block boundary */
    if (mtd_mod_by_eb(instr->addr, mtd)) {
        printk("%s: unaligned address\n", __func__);
        return -EINVAL;
    }
    /* Length must align on block boundary */
    if (mtd_mod_by_eb(instr->len, mtd)) {
        pr_debug("%s: length not block aligned\n", __func__);
        return -EINVAL;
    }

    memset(priv->buffer + instr->addr, 0xff, instr->len);
    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return 0;
}

static int virt_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
                         size_t *retlen, u_char *buf)
{
    struct virt_mtd_priv *priv = mtd->priv;
    printk("%s,%lld,%x\n", __func__, from, len);
    memcpy(buf, priv->buffer + from, len);
    *retlen = len;
    return 0;
}

static int virt_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
                          size_t *retlen, const u_char *buf)
{
    struct virt_mtd_priv *priv = mtd->priv;
    printk("%s,%lld,%x\n", __func__, to, len);
    memcpy(priv->buffer + +to, buf, len);
    *retlen = len;
    return 0;
}

static int virt_mtd_probe(struct platform_device *pdev)
{
    struct virt_mtd_priv *priv;
    struct mtd_info *mtd;

    printk("%s\n", __func__);

    priv = devm_kzalloc(&pdev->dev, sizeof(struct virt_mtd_priv), GFP_KERNEL);
    if (!priv) {
        printk("can not alloc memory\n");
        return -ENOMEM;
    }
    mtd = &priv->mtd;

    mtd->priv = priv;
    mtd->name = "virt_mtd";
    mtd->type = MTD_RAM;
    mtd->flags = MTD_CAP_RAM;
    mtd->size = VIRT_MTD_SIZE;
    mtd->writesize = 1;
    mtd->erasesize = 512;
    mtd->_erase = virt_mtd_erase;
    mtd->_read = virt_mtd_read;
    mtd->_write = virt_mtd_write;
    memset(priv->buffer, 0xff, sizeof(priv->buffer));

    platform_set_drvdata(pdev, priv);
    return mtd_device_register(mtd, virt_mtd_partitions,
                               ARRAY_SIZE(virt_mtd_partitions));
}

static int virt_mtd_remove(struct platform_device *pdev)
{
    struct virt_mtd_priv *priv = platform_get_drvdata(pdev);
    printk("%s\n", __func__);
    return mtd_device_unregister(&priv->mtd);
}

static void virt_mtd_release(struct device *dev)
{
}

static struct platform_device virt_mtd_device = {
    .name = "virt-mtd-dev",
    .id = -1,
    .dev =
        {
            .release = virt_mtd_release,
        },
};

static struct platform_driver virt_mtd_driver = {
    .probe = virt_mtd_probe,
    .remove = virt_mtd_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "virt-mtd-dev",
        },
};

static int virt_mtd_init(void)
{
    printk("virt_mtd_init\n");
    platform_device_register(&virt_mtd_device);
    platform_driver_register(&virt_mtd_driver);

    return 0;
}

static void virt_mtd_exit(void)
{
    printk("virt_mtd_exit\n");
    platform_driver_unregister(&virt_mtd_driver);
    platform_device_unregister(&virt_mtd_device);
}

module_init(virt_mtd_init);
module_exit(virt_mtd_exit);
MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("virt mtd driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");