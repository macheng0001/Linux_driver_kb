#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>
#include <fcntl.h>

#define TMP75_MAGIC 't'
#define TMP75_SET_RESOLUTION _IOW(TMP75_MAGIC, 1, int)
#define TMP75_GET_TEMPERATURE _IOW(TMP75_MAGIC, 2, int)

typedef enum { bit_9, bit_10, bit_11, bit_12 } resolution_cfg;

int main(int argc, char *argv[])
{
    int fd, temperature, ret;

    fd = open("/dev/tmp75", O_RDWR);
    if (fd <= 0) {
        printf("open device failed\n");
        exit(-1);
    }

    resolution_cfg res_cfg = bit_12;
    ret = ioctl(fd, TMP75_SET_RESOLUTION, res_cfg);
    if (ret < 0) {
        perror("tmp75 set resolution failed");
        exit(-1);
    }
    while (1) {
        ret = ioctl(fd, TMP75_GET_TEMPERATURE, &temperature);
        if (ret < 0) {
            perror("tmp75 get temperature failed");
            exit(-1);
        }
        printf("current temperature: %.4f ℃\n", temperature * 1.0 / 1000);
        sleep(1);
    }
    close(fd);
}