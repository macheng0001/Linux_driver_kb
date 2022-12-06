#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
struct ssd1306fb_par {
    struct fb_info *info;
    struct i2c_client *client;
    unsigned char width;
    unsigned char height;
};

static int ssd1306fb_write_cmd(struct i2c_client *client, unsigned char cmd)
{
    int ret;
    unsigned char array[2];

    array[0] = 0x80;
    array[1] = cmd;

    ret = i2c_master_send(client, array, 2);
    if (ret != 2)
        return ret;

    return 0;
}

static int ssd1306fb_write_data(struct i2c_client *client, unsigned char *data,
                                unsigned int len)
{
    int ret;
    unsigned char array[1 + len];

    array[0] = 0x40;
    memcpy(array + 1, data, len);

    ret = i2c_master_send(client, array, 1 + len);
    if (ret != (1 + len))
        return ret;

    return 0;
}

static void ssd1306fb_display(struct ssd1306fb_par *par)
{
    struct fb_info *info = par->info;
    unsigned char *vmem = info->screen_base;
    printk("ssd1306fb_display\n");
    ssd1306fb_write_data(par->client, vmem, info->fix.smem_len);
}

static void ssd1306fb_deferred_io(struct fb_info *info,
                                  struct list_head *pagelist)
{
    ssd1306fb_display(info->par);
}

static ssize_t ssd1306fb_write(struct fb_info *info, const char __user *buf,
                               size_t count, loff_t *ppos)
{
    struct ssd1306fb_par *par = info->par;
    unsigned char __iomem *dst;
    unsigned long p = *ppos, total_size = info->fix.smem_len;

    if (p > total_size)
        return -EINVAL;

    if (count + p > total_size)
        count = total_size - p;

    if (!count)
        return -EINVAL;
    dst = (void __force *)(info->screen_base + p);
    if (copy_from_user(dst, buf, count))
        return -EFAULT;
    ssd1306fb_display(par);

    *ppos += count;

    return count;
}

static int ssd1306fb_blank(int blank_mode, struct fb_info *info)
{
    struct ssd1306fb_par *par = info->par;
    int ret;

    if (blank_mode != FB_BLANK_UNBLANK)
        ret = ssd1306fb_write_cmd(par->client, 0xae);
    else
        ret = ssd1306fb_write_cmd(par->client, 0xaf);

    return ret;
}

static void ssd1306fb_imageblit(struct fb_info *info,
                                const struct fb_image *image)
{
    struct ssd1306fb_par *par = info->par;
    sys_imageblit(info, image);
    ssd1306fb_display(par);
}

static struct fb_ops ssd1306fb_ops = {
    .owner = THIS_MODULE,
    .fb_write = ssd1306fb_write,
    .fb_blank = ssd1306fb_blank,
    .fb_imageblit = ssd1306fb_imageblit,
};

static const struct fb_fix_screeninfo ssd1306fb_fix = {
    .id = "ssd1306",
    .type = FB_TYPE_PACKED_PIXELS,
    .visual = FB_VISUAL_MONO10,
    .xpanstep = 0,
    .ypanstep = 0,
    .ywrapstep = 0,
    .accel = FB_ACCEL_NONE,
    .line_length = OLED_WIDTH / 8,
};

static const struct fb_var_screeninfo ssd1306fb_var = {
    .bits_per_pixel = 1,
    .xres = OLED_WIDTH,
    .xres_virtual = OLED_WIDTH,
    .yres = OLED_HEIGHT,
    .yres_virtual = OLED_HEIGHT,
    .red.length = 1,
    .red.offset = 0,
};

static struct fb_deferred_io ssd1306fb_defio = {
    .delay = HZ / 10,
    .deferred_io = ssd1306fb_deferred_io,
};

static int ssd1306fb_init(struct ssd1306fb_par *par)
{
    int ret;
    struct i2c_client *client = par->client;

    //设置对比度
    ret = ssd1306fb_write_cmd(client, 0x81);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 127);
    if (ret < 0)
        return ret;

    //设置SEG段重映射
    ret = ssd1306fb_write_cmd(client, 0xa1);
    if (ret < 0)
        return ret;

    //设置列输出扫描方向
    ret = ssd1306fb_write_cmd(client, 0xc8);
    if (ret < 0)
        return ret;

    //设置复用率
    ret = ssd1306fb_write_cmd(client, 0xa8);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, par->height - 1);
    if (ret < 0)
        return ret;

    //设置显示偏移
    ret = ssd1306fb_write_cmd(client, 0xd3);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0);
    if (ret < 0)
        return ret;

    //设置时钟频率
    ret = ssd1306fb_write_cmd(client, 0xd5);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x80);
    if (ret < 0)
        return ret;

    //设置预充电周期
    ret = ssd1306fb_write_cmd(client, 0xd9);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x1f);
    if (ret < 0)
        return ret;

    //设置COM引脚硬件配置
    ret = ssd1306fb_write_cmd(client, 0xda);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x12);
    if (ret < 0)
        return ret;

    //设置VCOMH Deselect
    ret = ssd1306fb_write_cmd(client, 0xdb);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x40);
    if (ret < 0)
        return ret;

    //设置DC-DC电荷泵
    ret = ssd1306fb_write_cmd(client, 0x8d);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x14);
    if (ret < 0)
        return ret;

    //设置地址模式
    ret = ssd1306fb_write_cmd(client, 0x20);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x00);
    if (ret < 0)
        return ret;

    //设置列范围
    ret = ssd1306fb_write_cmd(client, 0x21);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x00);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, par->width - 1);
    if (ret < 0)
        return ret;

    //设置页范围
    ret = ssd1306fb_write_cmd(client, 0x22);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, 0x00);
    if (ret < 0)
        return ret;

    ret = ssd1306fb_write_cmd(client, par->height / 8 - 1);
    if (ret < 0)
        return ret;

    //清除显示
    ssd1306fb_display(par);

    //开启显示
    ret = ssd1306fb_write_cmd(client, 0xaf);
    if (ret < 0)
        return ret;

    return 0;
}

static int ssd1306fb_probe(struct i2c_client *client,
                           const struct i2c_device_id *id)
{
    int ret;
    struct fb_info *info;
    struct ssd1306fb_par *par;
    unsigned char *vmem;
    unsigned int vmem_size;

    printk("ssd1306fb_probe\n");

    info = framebuffer_alloc(sizeof(struct ssd1306fb_par), &client->dev);
    if (!info) {
        dev_err(&client->dev, "Couldn't allocate framebuffer.\n");
        return -ENOMEM;
    }

    par = info->par;
    par->info = info;
    par->client = client;
    par->width = OLED_WIDTH;
    par->height = OLED_HEIGHT;
    i2c_set_clientdata(client, info);

    vmem_size = ssd1306fb_var.xres * ssd1306fb_var.yres / 8;
    vmem =
        (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, get_order(vmem_size));
    if (!vmem) {
        dev_err(&client->dev, "Couldn't allocate graphical memory.\n");
        ret = -ENOMEM;
        goto err_get_free_page;
    }

    info->fbops = &ssd1306fb_ops;
    info->fix = ssd1306fb_fix;
    info->var = ssd1306fb_var;
    info->fbdefio = &ssd1306fb_defio;
    info->screen_base = (unsigned char __force __iomem *)vmem;
    info->fix.smem_start = __pa(vmem);
    info->fix.smem_len = vmem_size;
    fb_deferred_io_init(info);

    ret = ssd1306fb_init(par);
    if (ret < 0) {
        dev_err(&client->dev, "ssd1306 init failed.\n");
        goto err_ssd_init;
    }

    ret = register_framebuffer(info);
    if (ret) {
        dev_err(&client->dev, "Couldn't register the framebuffer\n");
        goto err_reg_fb;
    }

    return 0;
err_ssd_init:
err_reg_fb:
    fb_deferred_io_cleanup(info);
err_get_free_page:
    framebuffer_release(info);
    return ret;
}

static int ssd1306fb_remove(struct i2c_client *client)
{
    struct fb_info *info = i2c_get_clientdata(client);

    printk("ssd1306fb_remove\n");
    unregister_framebuffer(info);
    fb_deferred_io_cleanup(info);
    __free_pages(__va(info->fix.smem_start), get_order(info->fix.smem_len));
    framebuffer_release(info);

    return 0;
}

static const struct i2c_device_id ssd1306fb_i2c_id[] = {
    {"ssd1306_fb", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, ssd1306fb_i2c_id);

static const struct of_device_id ssd1306fb_of_match[] = {
    {.compatible = "xm,ssd1306_fb"},
    {/* Sentinel */},
};

static struct i2c_driver ssd1306fb_driver = {
    .probe = ssd1306fb_probe,
    .remove = ssd1306fb_remove,
    .id_table = ssd1306fb_i2c_id,
    .driver =
        {
            .name = "ssd1306fb",
            .of_match_table = ssd1306fb_of_match,
        },
};

module_i2c_driver(ssd1306fb_driver);

MODULE_AUTHOR("xiaoma <machengyuan@coinv.com>");
MODULE_DESCRIPTION("ssd1306 framebuffer driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");