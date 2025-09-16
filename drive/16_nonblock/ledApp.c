#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> 
#include <poll.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
	unsigned char databuf;
	int key_value;
	int cmd_num = -1;
	int ret;
	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}
	 struct pollfd fds;
	
	filename = argv[1];

	
	fd = open(filename, O_RDWR | O_NONBLOCK);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	fd_set readfds; 
	struct timeval timeout;
	while (1) {
#if 0	
		FD_ZERO(&readfds);
		FD_SET( fd,  &readfds);//监视的文件描述符集合
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;//500ms

		ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
		switch (ret) {
			case 0: /*超时*/

				break;
			case -1: /*错误*/
				break;
			default:

				if (FD_ISSET(fd, &readfds )) {
					int ret = read(fd, &databuf, sizeof(databuf));
						if (ret > 0) {
							printf("key value = %#x\n", databuf);
						} 
						
				 }


		}
#endif
		fds.fd = fd;
		fds.events = POLLIN;
		int count = poll(&fds, 1, 500);
		if (count > 0) {

			int ret = read(fd, &databuf, sizeof(databuf));
			if (ret > 0) {
				if (fds.revents & POLLIN) {
					printf("key value = %#x\n", databuf);
				}
				
			} else if (ret == 0){
				
			} else {
				printf("read 错误\n");
				break;
			}
			


		} else if (count == 0) {

			//printf("检测超时一次\n");
		}
		
		
		
	}

	close(fd); 
	
		
		

	return 0;
}


