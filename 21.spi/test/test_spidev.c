#include <stdio.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int spi_transfer(int fd, uint8_t const *tx, uint8_t const *rx,
                        size_t len, uint32_t bits, uint32_t speed)
{
    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .delay_usecs = 0,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

    return ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);
}

int main(int argc, char *argv[])
{
    int fd, ret;
    uint32_t mode = 0, speed = 500000, bits_per_word = 8;

    fd = open("/dev/spidev1.0", O_RDWR);
    if (fd < 0) {
        perror("open device failed\n");
        exit(-1);
    }

    //设置spi传输模式
    ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
    if (ret == -1) {
        perror("can't set spi mode");
        exit(-1);
    }
    //设置spi字长
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
    if (ret == -1) {
        perror("can't set bits per word");
        exit(-1);
    }
    //设置最大传输速度
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1) {
        perror("can't set max speed hz");
        exit(-1);
    }

#define DATA_LEN 4
    uint8_t rx[DATA_LEN], tx[DATA_LEN];
    memset(tx, 0x55, sizeof(tx));
    while (1) {
        sleep(1);
        ret = spi_transfer(fd, tx, rx, DATA_LEN, bits_per_word, speed);
        if (ret == -1) {
            perror("can't send spi message");
            exit(-1);
        }
    }

    close(fd);

    return 0;
}