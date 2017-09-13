#include "stdio.h"

int main(int argc, char * argv[]) {
	int fd;
	char buf[2048];
	memset(buf, 0, 2048);
	if(argc == 1) {
		printf("touch: please input the filename.\n");
		return 0;
	}

	
	fd = open(argv[1], O_CREAT | O_RDWR);
	if(fd == -1) {
		printf("touch: %s is already existed.\n", argv[1]);
		return 0;
	}
	printf("File created: %s (fd %d)\n", argv[1], fd);
	int n = write(fd, buf, strlen(buf));
	close(fd);

	return 0;
}
