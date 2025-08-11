#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> 
int main()
{
    int fd = open("/sys/class/gpio/gpio0/value", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open error");
    }

    int epfd = epoll_create(5);
    struct epoll_event event,events[5];
    event.events = EPOLLPRI;
    event.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event);
    pid_t pid = fork();
    if (pid > 0) {

        while (1) {
            int len = epoll_wait(epfd, events, 5, -1);
            for (int i = 0; i < len; ++i) {
                char buf[5];
                lseek(fd, 0, SEEK_SET);
                int n = read(fd, buf, 5);
                printf("interupt ----\n");
            }
            

        }

    } else {
        
        while(1) {
            
            if (write(fd, "1", 1) < 0) {
                perror("write error");
            }
            lseek(fd, 0, SEEK_SET);
            sleep(3);
            if (write(fd, "0", 1) < 0) {
                perror("write error");
            }
            lseek(fd, 0, SEEK_SET);
            sleep(3);
        }
        printf("------\n");   
    }
   
    return 0;
}