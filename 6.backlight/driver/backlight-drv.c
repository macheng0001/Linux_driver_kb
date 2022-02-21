#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>

struct oled_bl_data {
    struct i2c_client *client;
    struct backlight_device *bd;
    u32 contrast;
};

#define MAX_CONTRAST 255
#define DEFAULT_CONTRAST 127

static int oled_write_cmd(struct i2c_client *client, u8 cmd)
{
    u8 array[2];

    array[0] = 0x80;
    array[1] = cmd;

    return i2c_master_send(client, array, 2);
}

static int oled_update_bl(struct backlight_device *bdev)
{
    struct oled_bl_data *data = bl_get_data(bdev);
    int ret;
    int brightness = bdev->props.brightness;

    data->contrast = brightness;

    ret = oled_write_cmd(data->client, 0x81);
    if (ret < 0)
        return ret;

    ret = oled_write_cmd(data->client, data->contrast);
    if (ret < 0)
        return ret;

    return 0;
}

static int oled_get_brightness(struct backlight_device *bdev)
{
    struct oled_bl_data *data = bl_get_data(bdev);

    return data->contrast;
}

static const struct backlight_ops oled_bl_ops = {
    .options = BL_CORE_SUSPENDRESUME,
    .update_status = oled_update_bl,
    .get_brightness = oled_get_brightness,
};

static int backlight_test_probe(struct i2c_client *client,
                                const struct i2c_device_id *id)
{
    int ret;
    struct backlight_device *bd;
    struct oled_bl_data *bl_data;

    printk("backlight test probe\n");
    bl_data = devm_kzalloc(&client->dev, sizeof(*bl_data), GFP_KERNEL);
    if (!bl_data)
        return -ENOMEM;

    bd = backlight_device_register("oled_bl", &client->dev, bl_data,
                                   &oled_bl_ops, NULL);
    if (IS_ERR(bd)) {
        ret = PTR_ERR(bd);
        dev_err(&client->dev, "unable to register backlight device: %d\n", ret);
        return ret;
    }

    bd->props.brightness = DEFAULT_CONTRAST;
    bd->props.max_brightness = MAX_CONTRAST;

    bl_data->bd = bd;
    bl_data->client = client;
    i2c_set_clientdata(client, bl_data);

    return 0;
}

static int backlight_test_remove(struct i2c_client *client)
{
    struct oled_bl_data *bl_data;

    printk("backlight test remove\n");
    bl_data = i2c_get_clientdata(client);
    backlight_device_unregister(bl_data->bd);

    return 0;
}

static const struct i2c_device_id oled_i2c_id[] = {{"backlight-test", 0}, {}};
MODULE_DEVICE_TABLE(i2c, oled_i2c_id);

static const struct of_device_id backlight_test_of_match[] = {
    {.compatible = "backlight-test"}, {/* Sentinel */}};

static struct i2c_driver backlight_test_driver = {
    .probe = backlight_test_probe,
    .remove = backlight_test_remove,
    .id_table = oled_i2c_id,
    .driver =
        {
            .name = "backlight-test",
            .of_match_table = backlight_test_of_match,
        },
};

module_i2c_driver(backlight_test_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("backlight test driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
