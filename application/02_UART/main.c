#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/input.h>
#include <termios.h>
#include <assert.h>
// struct termios
// {
// tcflag_t c_iflag; /* input mode flags */
// tcflag_t c_oflag; /* output mode flags */
// tcflag_t c_cflag; /* control mode flags */
// tcflag_t c_lflag; /* local mode flags */
// cc_t c_line; /* line discipline */
// cc_t c_cc[NCCS]; /* control characters */
// speed_t c_ispeed; /* input speed */
// speed_t c_ospeed; /* output speed */
// };

/**
 * 
 * 
 * ×
 * 
 * 
 */

int main(int argc, char *argv[])
{
    struct termios new_cfg = {0};
    // if (argc < 2) {
    //     fprintf(stderr, "Usage: %s <path>\n", argv[0]);
    //     exit(1);
    // }
    
    int fd = open("/dev/ttymxc2", O_RDONLY);
    assert(fd > 0 && "open");
    /*设置终端为原始模式*/
    cfmakeraw(&new_cfg);

    // int tcgetattr(int fd, struct termios *termios_p); 获取终端当前配置参数
    /*获取当前串口终端的配置参数*/
    struct termios old_cfg;
    int ret = tcgetattr(fd, &old_cfg);
    assert(ret == 0 && "tcgetattr");

    /*设置接受使能*/
    
    new_cfg.c_cflag |= CREAD;
    
    /*设置波特率*/
    cfsetispeed(&new_cfg, B115200);    
    cfsetospeed(&new_cfg, B115200);


    /*设置数据位大小*/
    new_cfg.c_cflag |= CS8;

    /*设置停止位*/
    new_cfg.c_cflag |= CSTOPB;

    /*设置非阻塞*/
    new_cfg.c_cc[VTIME] = 0;
    new_cfg.c_cc[VMIN] = 0;
    
    /*清空缓冲区*/
    tcdrain(fd);

    /*设置串口终端新属性*/
    ret = tcsetattr(fd, TCSANOW, &new_cfg);
    assert(ret != -1 && "tcsetattr");
    close(fd);
    return 0;
}