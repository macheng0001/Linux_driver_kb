#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <math.h>

#define REG_TMP75_TEMP 0x00
#define REG_TMP75_CONF 0x01

#define REG_TMP75_CONF_RESOLUTION(x) ((x & 0x3) << 5)
typedef enum { bit_9, bit_10, bit_11, bit_12 } resolution_cfg;
static unsigned char resolution[4] = {9, 10, 11, 12};

#define TMP75_ADDRESS 0x4f

static int i2c_read(int fd, unsigned short addr, unsigned char reg,
                    unsigned char *buf, unsigned short len)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msg[2];

    data.nmsgs = 2;
    data.msgs = msg;

    msg[0].addr = addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &reg;

    msg[1].addr = addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = len;
    msg[1].buf = buf;

    return ioctl(fd, I2C_RDWR, &data);
}

static int i2c_write(int fd, unsigned short addr, unsigned char reg,
                     unsigned char *buf, unsigned short len)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msg[1];
    unsigned char temp[len + 1];

    data.nmsgs = 1;
    data.msgs = msg;

    msg[0].addr = addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = temp;
    msg[0].buf[0] = reg;
    memcpy(&msg[0].buf[1], buf, len);

    return ioctl(fd, I2C_RDWR, &data);
}

static int tmp75_set_resolution(int fd, resolution_cfg res_cfg)
{
    unsigned char buf[1];
    int ret;

    buf[0] = REG_TMP75_CONF_RESOLUTION(res_cfg);
    ret = i2c_write(fd, TMP75_ADDRESS, REG_TMP75_CONF, buf, 1);
    if (ret < 0) {
        perror("config tmp75 resolution failed");
        return ret;
    }

    return 0;
}

static int tmp75_get_temperature(int fd, resolution_cfg res_cfg,
                                 float *temperature)
{
    unsigned char buf[2];
    short temp;
    int ret;

    ret = i2c_read(fd, TMP75_ADDRESS, REG_TMP75_TEMP, buf, 2);
    if (ret < 0) {
        perror("read tmp75 failed");
        return ret;
    }
    temp = buf[0] << 8 | buf[1];
    *temperature = temp * 1.0 / pow(2, 16 - resolution[res_cfg]) /
                   pow(2, resolution[res_cfg] - 8);

    return 0;
}

int main(int argc, char *argv[])
{
    int fd, ret;
    float temperature;
    resolution_cfg res_cfg = bit_12;

    fd = open("/dev/i2c-2", O_RDWR);
    if (fd <= 0) {
        printf("open device failed\n");
        exit(-1);
    }

    ret = tmp75_set_resolution(fd, res_cfg);
    if (ret < 0) {
        exit(-1);
    }

    while (1) {
        ret = tmp75_get_temperature(fd, res_cfg, &temperature);
        if (ret < 0) {
            exit(-1);
        }
        printf("current temperature: %.4f ℃\n", temperature);
        sleep(1);
    }

    close(fd);

    return 0;
}