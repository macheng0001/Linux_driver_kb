#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <sys/epoll.h>

#define NONBLOCK_IO_SELECT
//#define NONBLOCK_IO_POLL
//#define NONBLOCK_IO_EPOLL

static pthread_t pthread[2];
static void *pthead_fn1(void *arg)
{
    int fd, key_value, ret;
    fd_set readfds;
    struct timeval timeout;
    struct pollfd readpollfd;
    int epollfd;
    struct epoll_event epoll_event_set, epoll_event_get;

    fd = open("/dev/nonblock_io", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        printf("Can't open device\n");
        return NULL;
    }

    readpollfd.fd = fd;
    readpollfd.events = POLL_IN;

    epollfd = epoll_create(1);
    if (epollfd < 0) {
        perror("epoll create");
        return NULL;
    }
    bzero(&epoll_event_set, sizeof(struct epoll_event));
    epoll_event_set.events = EPOLLIN;
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &epoll_event_set);
    if (ret < 0) {
        perror("epoll ctl");
        return NULL;
    }

    while (1) {
#if defined(NONBLOCK_IO_SELECT)
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 500000;
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret) {
        case 0:
            printf("read key value timeout\n");
            break;
        case -1:
            perror("read failed");
            break;
        default:
            if (FD_ISSET(fd, &readfds)) {
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
#elif defined(NONBLOCK_IO_POLL)
        ret = poll(&readpollfd, 1, 1500);
        switch (ret) {
        case 0:
            printf("read key value timeout\n");
            break;
        case -1:
            perror("read failed");
            break;
        default:
            if (readpollfd.revents | POLL_IN) {
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
#elif defined(NONBLOCK_IO_EPOLL)
        ret = epoll_wait(epollfd, &epoll_event_get, 1, 1500);
        switch (ret) {
        case 0:
            printf("read key value timeout\n");
            break;
        case -1:
            perror("read failed");
            break;
        default:
            if (epoll_event_get.events | EPOLLIN) {
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
#endif
    }

    close(fd);
}

static void *pthead_fn2(void *arg)
{
    while (1) {
        sleep(5);
        pthread_kill(pthread[0], SIGCHLD);
    }
}

void sig_handler(int sigo) { printf("sig_handler\n"); }

int main(int argc, char **argv)
{
    int ret;
    struct sigaction act;

    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    ret = pthread_create(&pthread[0], NULL, pthead_fn1, NULL);
    if (ret != 0)
        pthread_exit(NULL);

    ret = pthread_create(&pthread[1], NULL, pthead_fn2, NULL);
    if (ret != 0)
        pthread_exit(NULL);

    pthread_join(pthread[0], NULL);
    pthread_join(pthread[1], NULL);

    return 0;
}
