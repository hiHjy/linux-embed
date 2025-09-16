#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"



int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
	unsigned char databuf;
	int key_value;
	int cmd_num = -1;
	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}
	
	
	filename = argv[1];

	
	fd = open(filename, O_RDWR | O_NONBLOCK);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	while (1) {
		int ret = read(fd, &databuf, sizeof(databuf));
		if (ret > 0) {
			printf("key value = %#x\n", databuf);
		} 
		
	}

	close(fd); 
	
		
		

	return 0;
}


