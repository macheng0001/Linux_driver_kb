#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#define ATTR_MAX 128
typedef enum { bit_9, bit_10, bit_11, bit_12 } resolution_cfg;

static int sysfs_read_attr(const char *device, const char *attr, char *buf)
{
    char path[NAME_MAX];
    char *p;
    FILE *f;

    snprintf(path, NAME_MAX, "%s/%s", device, attr);

    if (!(f = fopen(path, "r")))
        return -1;
    p = fgets(buf, ATTR_MAX, f);
    fclose(f);
    if (!p)
        return -1;

    /* Last byte is a '\n'; chop that off */
    p[strlen(buf) - 1] = '\0';

    return 0;
}

static int sysfs_write_attr(const char *device, const char *attr,
                            const char *buf)
{
    char path[NAME_MAX];
    int ret;
    FILE *f;

    snprintf(path, NAME_MAX, "%s/%s", device, attr);

    if (!(f = fopen(path, "w")))
        return -1;
    ret = fputs(buf, f);
    if (fclose(f) < 0)
        return -1;
    if (ret < 0)
        return ret;

    return 0;
}

static int tmp75_get_temperature(float *value)
{
    char buff[ATTR_MAX];
    int ret;

    ret = sysfs_read_attr("/sys/class/hwmon/hwmon1", "temp1_input", buff);
    if (ret < 0) {
        return -1;
    } else {
        *value = atoi(buff) / 1000.0;
    }

    return 0;
}

static int tmp75_set_resolution(resolution_cfg res_cfg)
{
    char buf[10];
    int ret;

    snprintf(buf, sizeof(buf), "%d", res_cfg);
    ret = sysfs_write_attr("/sys/class/hwmon/hwmon1", "resolution", buf);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

int main()
{
    int ret;
    float temperature;

    ret = tmp75_set_resolution(bit_12);
    if (ret < 0) {
        perror("tmp75 set resolution failed");
        exit(-1);
    }
    while (1) {
        ret = tmp75_get_temperature(&temperature);
        if (ret < 0) {
            perror("tmp75 get temperature failed");
            exit(-1);
        } else {
            printf("current temperature: %.4f ℃\n", temperature);
        }
        sleep(1);
    }

    return 0;
}