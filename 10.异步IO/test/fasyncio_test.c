#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <sys/epoll.h>

int fd = 0;

void sig_action(int sigo, siginfo_t *info, void *context)
{
    int key_value, ret;

    printf("sig_action\n");
    if (info->si_code & POLL_IN) {
        key_value = 0;
        ret = read(fd, &key_value, sizeof(key_value));
        if (ret < 0) {
            perror("read failed");
        }

        if (key_value) {
            printf("key press\n");
        }
    }
}

int main(int argc, char **argv)
{
    int ret;
    int flags = 0;
    struct sigaction act;

    fd = open("/dev/fasync_io", O_RDWR);
    if (fd < 0) {
        printf("Can't open device\n");
        return -1;
    }

    act.sa_sigaction = sig_action;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGIO, &act, NULL);

    fcntl(fd, F_SETSIG, SIGIO);
    fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFL, flags | FASYNC);

    while (1) {
        sleep(1);
    }

    close(fd);

    return 0;
}
