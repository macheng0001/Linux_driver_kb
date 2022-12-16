#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>

#define ATTR_MAX 128

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

/**
 * @brief 获取uio映射内存大小
 *
 * @param index uio设备编号，如uio0则为0
 * @param n map编号
 * @param size 映射内存大小
 * @return int 0-成功 其他-失败
 */
static int uio_get_map_size(int index, int n, int *size)
{
    char path[NAME_MAX];
    int ret;
    char buf[16];

    snprintf(path, NAME_MAX, "/sys/class/uio/uio%d/maps/map%d", index, n);
    ret = sysfs_read_attr(path, "size", buf);
    if (ret < 0) {
        return ret;
    }

    sscanf(buf, "0x%x", size);

    return 0;
}

int main()
{
    int map_size, fd, event_count;
    int ret;
    int *mmap_addr;
    struct timeval timeout;
    fd_set readfds;

    ret = uio_get_map_size(0, 0, &map_size);
    if (ret < 0) {
        printf("uio get map size failed\n", ret);
        return ret;
    }
    printf("map_size:0x%x\n", map_size);

    fd = open("/dev/uio0", O_RDWR);
    if (fd <= 0) {
        return -1;
    }
    // mmap的offset设置需要映射的内存区域，0即为映射第一个，注意1×4096为映射第二个(mmap的最小单位为页)，以此类推
    mmap_addr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_addr == MAP_FAILED) {
        printf("mmap failed\n");
        return -1;
    }
    printf("mmap_addr:0x%x\n", mmap_addr);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret) {
        case 0:
            printf("timeout\n");
            break;
        case -1:
            perror("failed");
            break;
        default:
            if (FD_ISSET(fd, &readfds)) {
                ret = read(fd, &event_count, sizeof(event_count));
                if (ret < 0) {
                    perror("read failed");
                }
                printf("event_count:%d\n", event_count);
            }
        }
    }

    munmap(mmap_addr, map_size);
    close(fd);
    return 0;
}