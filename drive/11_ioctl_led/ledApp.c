#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <sys/ioctl.h>
#define CLOSE_CMD			_IO(0XEF, 1)
#define OPEN_CMD			_IO(0XEF, 2)
#define SETPERIOD_CMD		_IOW(0XEF, 3, int)


#define LEDOFF 	0
#define LEDON 	1


int main(int argc, char *argv[])
{
	int fd;
	char *filename;
	

	int cmd_num = -1;
	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}
	
	
	filename = argv[1];

	/* 打开led驱动 */
	fd = open(filename, O_RDWR);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}
	char buf[32];
	char str;
	while (1) {
		printf("请输入命令：1，打开定时器 2，关闭定时器， 3设置定时器周期\n");

		int ret = scanf("%d", &cmd_num);
		if (ret != 1) {
			gets(&str);
		}

		int arg;
		switch (cmd_num) {
			case 1:
				printf("已选择1：打开定时器\n"); 
				ioctl(fd, OPEN_CMD, &arg);
			break;

			case 2:
				printf("已选择2：关闭定时器\n"); 
				ioctl(fd, CLOSE_CMD, &arg);
			break;

			case 3:
				
				memset(buf, 0, sizeof(buf));
				printf("已选择3：设置定时器：支持500 1000（默认）2000 （单位毫秒）\n");
				printf("请输入：定时器周期： ");
				printf("\n");
				int ret = read(STDIN_FILENO, buf, sizeof(buf) - 1);
				if (ret < 0) {
					fprintf(stderr, "read error\n");
				}

				buf[ret] = '\0';
				int period = atoi(buf);
				printf("已选择%d ms\n", period);
				ioctl(fd, SETPERIOD_CMD, &period);

			break;

		}
		
		
	}

	close(fd); 
	
		
		

	return 0;
}


