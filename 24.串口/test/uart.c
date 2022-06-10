#include <termios.h>

/**
 * @brief 设置串口参数
 *
 * @param fd 设备句柄，设备一般为ttyXXX
 * @param baud_rate 波特率，支持2400/4800/9600/19200/38400/115200/230400，
 *                  设置错误则默认使用115200
 * @param data_bits 数据位，取值可为7或8,设置错误则默认使用8
 * @param parity 校验位，'n'或'N'-无奇偶校验 'o'或'O'-奇校验
 *               'e'或'E'-偶校验，设置错误默认使用无奇偶校验
 * @param stop_bits 停止位，取值可为1或2,设置错误则默认使用1
 * @return 0-正确 其他-错误
 */
int set_uart_opt(int fd, int baud_rate, int data_bits, char parity,
                 int stop_bits)
{
    struct termios new_cfg, old_cfg;
    int speed, ret;

    ret = tcgetattr(fd, &old_cfg);
    if (ret < 0) {
        return ret;
    }

    new_cfg = old_cfg;
    /* 配置为原始模式 */
    cfmakeraw(&new_cfg);
    new_cfg.c_cflag &= ~CSIZE;

    /* 设置波特率 */
    switch (baud_rate) {
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 230400:
        speed = B230400;
        break;
    default:
    case 115200:
        speed = B115200;
        break;
    }
    cfsetispeed(&new_cfg, speed);
    cfsetospeed(&new_cfg, speed);

    /* 设置数据位 */
    switch (data_bits) {
    case 7:
        new_cfg.c_cflag |= CS7;
        break;
    default:
    case 8:
        new_cfg.c_cflag |= CS8;
        break;
    }

    /* 设置奇偶校验位 */
    switch (parity) {
    default:
    case 'n':
    case 'N':
        new_cfg.c_cflag &= ~PARENB;
        new_cfg.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O':
        new_cfg.c_cflag |= (PARODD | PARENB);
        new_cfg.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E':
        new_cfg.c_cflag |= PARENB;
        new_cfg.c_cflag &= ~PARODD;
        new_cfg.c_iflag |= INPCK;
        break;
    }

    /* 设置停止位 */
    switch (stop_bits) {
    default:
    case 1:
        new_cfg.c_cflag &= ~CSTOPB;
        break;
    case 2:
        new_cfg.c_cflag |= CSTOPB;
    }

    /* 设置等待时间和最小接收字符 */
    new_cfg.c_cc[VTIME] = 0;
    new_cfg.c_cc[VMIN] = 1;

    /* 处理未接收字符 */
    tcflush(fd, TCIFLUSH);

    /* 激活新配置 */
    ret = tcsetattr(fd, TCSANOW, &new_cfg);
    if (ret < 0) {
        return ret;
    }

    return 0;
}