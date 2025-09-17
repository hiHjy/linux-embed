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
#include <signal.h>
static int fd;
void sig_handler(int sig) {
	unsigned char databuf;
	printf("信号处理函数执行\n");


	int ret = read(fd, &databuf, 1);
	if (ret < 0) {
		printf("read error\n");
		exit(-1);
	} else if (ret > 0) {
		printf("keyvalue=%#x\r\n", databuf);
	} 


}
int main(int argc, char *argv[])
{
	int etvalue;
	char *filename;
	unsigned char databuf;
	int key_value;
	int cmd_num = -1;
	int ret;
	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}
	
	
	filename = argv[1];
	
	
	fd = open(filename, O_RDWR);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	/*设置当前进程接受SIGIO信号*/
	fcntl(fd, F_SETOWN, getpid());

	int flags = fcntl(fd, F_GETFL);

	/*设置异步通知 */
	fcntl(fd, F_SETFL, flags | FASYNC); 

	signal(SIGIO, sig_handler);
	

	
	
	while (1) sleep(1);	
	close(fd); 	

	return 0;
}


