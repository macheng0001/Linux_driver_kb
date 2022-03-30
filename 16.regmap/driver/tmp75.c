#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/regmap.h>

#define REG_TMP75_TEMP 0x00
#define REG_TMP75_CONF 0x01
#define REG_TMP75_MAX 0x02

#define REG_TMP75_CONF_RESOLUTION(x) ((x & 0x3) << 5)
typedef enum { bit_9, bit_10, bit_11, bit_12 } resolution_cfg;
static unsigned char resolution[4] = {9, 10, 11, 12};

#define TMP75_MAGIC 't'
#define TMP75_SET_RESOLUTION _IOW(TMP75_MAGIC, 1, int)
#define TMP75_GET_TEMPERATURE _IOW(TMP75_MAGIC, 2, int)

struct tmp75_priv {
    struct i2c_client *client;
    struct miscdevice miscdev;
    resolution_cfg resolution_cfg;
    struct regmap *regmap;
};

static int tmp75_set_resolution(struct tmp75_priv *data, resolution_cfg res_cfg)
{
    unsigned int val;

    val = REG_TMP75_CONF_RESOLUTION(res_cfg);

    return regmap_update_bits(data->regmap, REG_TMP75_CONF,
                              REG_TMP75_CONF_RESOLUTION(bit_12), val);
}

static int tmp75_get_temperature(struct tmp75_priv *data,
                                 resolution_cfg res_cfg, int *temperature)
{
    unsigned int val;
    short temp;
    int ret;

    ret = regmap_read(data->regmap, REG_TMP75_TEMP, &val);
    if (ret < 0) {
        return ret;
    }

    temp = val;
    *temperature = ((temp >> (16 - resolution[res_cfg])) * 1000) >>
                   (resolution[res_cfg] - 8);

    return 0;
}

static int tmp75_open(struct inode *inode, struct file *file)
{
    struct miscdevice *miscdev = file->private_data;
    struct tmp75_priv *priv = dev_get_drvdata(miscdev->this_device);

    file->private_data = priv;

    return 0;
}

static long tmp75_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct tmp75_priv *priv = file->private_data;
    resolution_cfg resolution_cfg;
    int temperature, ret;

    switch (cmd) {
    case TMP75_SET_RESOLUTION:
        resolution_cfg = arg;
        if (resolution_cfg > bit_12)
            return -EINVAL;

        ret = tmp75_set_resolution(priv, resolution_cfg);
        if (ret < 0) {
            dev_err(priv->miscdev.this_device, "tmp75 set resolution failed\n");
            return ret;
        }
        priv->resolution_cfg = resolution_cfg;
        break;
    case TMP75_GET_TEMPERATURE:
        ret = tmp75_get_temperature(priv, priv->resolution_cfg, &temperature);
        if (ret < 0) {
            dev_err(priv->miscdev.this_device,
                    "tmp75 get temperature failed\n");
            return ret;
        }
        return put_user(temperature, (int __user *)arg);
    default:
        return -ENOTTY;
    }

    return 0;
}

static int tmp75_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations tmp75_fops = {
    .owner = THIS_MODULE,
    .open = tmp75_open,
    .unlocked_ioctl = tmp75_ioctl,
    .release = tmp75_release,
};

static bool tmp75_is_writeable_reg(struct device *dev, unsigned int reg)
{
    return reg != REG_TMP75_TEMP;
}

static bool tmp75_is_volatile_reg(struct device *dev, unsigned int reg)
{
    return reg == REG_TMP75_TEMP;
}

static const struct regmap_config tmp75_regmap_config = {
    .reg_bits = 8,
    .val_bits = 16,
    .max_register = REG_TMP75_MAX,
    .writeable_reg = tmp75_is_writeable_reg,
    .volatile_reg = tmp75_is_volatile_reg,
    .val_format_endian = REGMAP_ENDIAN_BIG,
    .cache_type = REGCACHE_RBTREE,
    .use_single_rw = true,
};

static int tmp75_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
    int ret;
    struct tmp75_priv *priv;

    priv = devm_kzalloc(&client->dev, sizeof(struct tmp75_priv), GFP_KERNEL);
    if (!priv) {
        dev_err(&client->dev, "Memory alloc failed\n");
        return -ENOMEM;
    }

    priv->regmap = devm_regmap_init_i2c(client, &tmp75_regmap_config);
    if (IS_ERR(priv->regmap))
        return PTR_ERR(priv->regmap);

    priv->miscdev.minor = MISC_DYNAMIC_MINOR;
    priv->miscdev.name = "tmp75";
    priv->miscdev.fops = &tmp75_fops;
    ret = misc_register(&priv->miscdev);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to register miscdev: %d\n", ret);
        return ret;
    }
    priv->resolution_cfg = bit_9; // default
    priv->client = client;
    dev_set_drvdata(priv->miscdev.this_device, priv);
    i2c_set_clientdata(client, priv);

    return 0;
}

static int tmp75_remove(struct i2c_client *client)
{
    struct tmp75_priv *priv;

    priv = i2c_get_clientdata(client);
    misc_deregister(&priv->miscdev);

    return 0;
}

static const struct i2c_device_id tmp75_ids[] = {{"tmp75", 0}, {}};
MODULE_DEVICE_TABLE(i2c, tmp75_ids);

static struct i2c_driver tmp75_driver = {
    .driver =
        {
            .name = "tmp75",
        },
    .probe = tmp75_probe,
    .remove = tmp75_remove,
    .id_table = tmp75_ids,
};

module_i2c_driver(tmp75_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("tmp75 driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
