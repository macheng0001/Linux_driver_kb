#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include "uart.h"

int main()
{
    int fd, ret, i, w = 0;
    fd_set rfds;
    struct timeval timeout;
    char r_buf[100];

    fd = open("/dev/vttyS1", O_RDWR);
    if (fd <= 0) {
        printf("open device failed\n");
        return -1;
    }

    ret = set_uart_opt(fd, 115200, 8, 'n', 1);
    if (ret < 0) {
        printf("set uart option failed\n");
        return ret;
    }

    while (1) {
        usleep(500000);

        write(fd, &w, 1);
        w++;

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        ret = select(fd + 1, &rfds, NULL, NULL, &timeout);
        if (ret < 0) {

        } else if (ret == 0) {
            printf("timeout\n");
        } else {
            ret = read(fd, r_buf, sizeof(r_buf));
            for (i = 0; i < ret; i++) {
                printf("r_buf[%d] = %x\n", i, r_buf[i]);
            }
        }
    }

    close(fd);

    return 0;
}