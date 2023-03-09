#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
#include <getopt.h>

#define BUFSIZE 10 * 1024
int main(int argc, char *argv[])
{
    const char *filename = NULL, *device = NULL;
    int dev_fd = -1, fil_fd = -1, ret, result;
    struct mtd_info_user mtd;
    struct stat filestat;
    struct erase_info_user erase;
    unsigned char src[BUFSIZE], dest[BUFSIZE];
    size_t size;

    if (argc != 3) {
        printf("usage: mtdchar_test <filename> <device>\n");
        return -1;
    }

    filename = argv[1];
    device = argv[2];

    dev_fd = open(device, O_SYNC | O_RDWR);
    if (dev_fd < 0) {
        printf("open device failed\n");
        return -1;
    }
    if (ioctl(dev_fd, MEMGETINFO, &mtd) < 0) {
        printf("This doesn't seem to be a valid MTD flash device!\n");
        return -1;
    }

    fil_fd = open(filename, O_RDONLY);
    if (dev_fd < 0) {
        printf("open file failed\n");
        return -1;
    }
    if (fstat(fil_fd, &filestat) < 0) {
        printf("get file failed\n");
        return -1;
    }

    //检查文件是否大于分区大小
    if (filestat.st_size > mtd.size) {
        printf("file is too large\n");
        return -1;
    }

    //擦除
    erase.start = 0;
    erase.length = filestat.st_size % mtd.erasesize == 0
                       ? (filestat.st_size)
                       : (filestat.st_size / mtd.erasesize + 1) * mtd.erasesize;
    if (ioctl(dev_fd, MEMERASE, &erase) < 0) {
        printf("erase device failed\n");
        return -1;
    }

    //写入
    size = filestat.st_size;
    while (size) {
        ret = read(fil_fd, src, BUFSIZE);
        if (ret < 0) {
            printf("read file failed\n");
            return -1;
        }

        result = write(dev_fd, src, ret);
        if (result != ret) {
            printf("write device failed\n");
            return -1;
        }
        size -= ret;
    }

    //校验
    lseek(fil_fd, 0, SEEK_SET);
    lseek(dev_fd, 0, SEEK_SET);
    size = filestat.st_size;
    while (size) {
        ret = read(fil_fd, src, BUFSIZE);
        if (ret < 0) {
            printf("read file failed\n");
            return -1;
        }

        result = read(dev_fd, dest, ret);
        if (result != ret) {
            printf("read device failed\n");
            return -1;
        }

        if (memcmp(src, dest, ret)) {
            printf("Check failed");
            return -1;
        }
        size -= ret;
    }

    return 0;
}