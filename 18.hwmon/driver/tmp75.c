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
#include <linux/regmap.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define REG_TMP75_TEMP 0x00
#define REG_TMP75_CONF 0x01
#define REG_TMP75_MAX 0x02

#define REG_TMP75_CONF_RESOLUTION(x) ((x & 0x3) << 5)
typedef enum { bit_9, bit_10, bit_11, bit_12 } resolution_cfg;
static unsigned char resolution[4] = {9, 10, 11, 12};

struct tmp75_priv {
    struct i2c_client *client;
    resolution_cfg resolution_cfg;
    struct regmap *regmap;
    struct device *hwmon_dev;
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

static umode_t tmp75_is_visible(const void *data, enum hwmon_sensor_types type,
                                u32 attr, int channel)
{
    switch (type) {
    case hwmon_temp:
        switch (attr) {
        case hwmon_temp_input:
            return S_IRUGO;
        };
    default:
        break;
    }
    return 0;
}

static int tmp75_read(struct device *dev, enum hwmon_sensor_types type,
                      u32 attr, int channel, long *val)
{
    struct tmp75_priv *priv = dev_get_drvdata(dev);
    int ret, temp;

    switch (type) {
    case hwmon_temp:
        switch (attr) {
        case hwmon_temp_input:
            ret = tmp75_get_temperature(priv, priv->resolution_cfg, &temp);
            if (ret < 0) {
                return ret;
            }
            *val = temp;
            break;
        default:
            return -EINVAL;
        };
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static struct hwmon_ops tmp75_ops = {
    .is_visible = tmp75_is_visible,
    .read = tmp75_read,
};

static const u32 tmp75_temp_config[] = {HWMON_T_INPUT, 0};

static struct hwmon_channel_info tmp75_temp = {
    .type = hwmon_temp,
    .config = tmp75_temp_config,
};

static const struct hwmon_channel_info *tmp75_info[] = {
    &tmp75_temp,
    NULL,
};

static struct hwmon_chip_info tmp75_chip_info = {
    .ops = &tmp75_ops,
    .info = tmp75_info,
};

static ssize_t tmp75_resolution_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    struct tmp75_priv *priv = dev_get_drvdata(dev);

    return sprintf(buf, "%u\n", priv->resolution_cfg);
}

static ssize_t tmp75_resolution_store(struct device *dev,
                                      struct device_attribute *attr,
                                      const char *buf, size_t count)
{
    struct tmp75_priv *priv = dev_get_drvdata(dev);
    resolution_cfg cfg;
    int ret;

    sscanf(buf, "%du", &cfg);
    if (cfg < bit_9 || cfg > bit_12)
        return -EINVAL;

    ret = tmp75_set_resolution(priv, cfg);
    if (ret < 0)
        return ret;

    priv->resolution_cfg = cfg;

    return count;
}

static SENSOR_DEVICE_ATTR(resolution, S_IWUSR | S_IRUGO, tmp75_resolution_show,
                          tmp75_resolution_store, 0);

static struct attribute *tmp75_attrs[] = {
    &sensor_dev_attr_resolution.dev_attr.attr,
    NULL,
};

ATTRIBUTE_GROUPS(tmp75);

static int tmp75_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
    struct tmp75_priv *priv;

    priv = devm_kzalloc(&client->dev, sizeof(struct tmp75_priv), GFP_KERNEL);
    if (!priv) {
        dev_err(&client->dev, "Memory alloc failed\n");
        return -ENOMEM;
    }

    priv->regmap = devm_regmap_init_i2c(client, &tmp75_regmap_config);
    if (IS_ERR(priv->regmap))
        return PTR_ERR(priv->regmap);

    priv->resolution_cfg = bit_9; // default
    priv->client = client;
    i2c_set_clientdata(client, priv);

    priv->hwmon_dev = devm_hwmon_device_register_with_info(
        &client->dev, client->name, priv, &tmp75_chip_info, tmp75_groups);
    if (IS_ERR(priv->hwmon_dev))
        return PTR_ERR(priv->hwmon_dev);

    return 0;
}

static int tmp75_remove(struct i2c_client *client)
{
    struct tmp75_priv *priv;

    priv = i2c_get_clientdata(client);

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
