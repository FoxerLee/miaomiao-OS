#include "stdio.h"
#include "../include/string.h"


int main(int argc, char * argv[])
{
	int fd, n, i, j;
	int is_saved = -1;
	char bufw[2048];
	char bufr[2048];
	char buf[2048];
	char com[2];
	memset(com, 0, 2);
	memset(bufw, 0, 2048);
	memset(bufr, 0, 2048);
	memset(buf, 0, 2048);

	
	if(argc == 1) {
		printf("vi: please input the filename.\n");
		return -1;
	}
	// open
	fd = open(argv[1], O_RDWR);
	if(fd == -1) {
		printf("vi: %s: No such file or directory\n", argv[1]);
		return -1;
	}
	
	printf("==================================================\n");
	printf("                 Stupid  Editer                   \n");
	printf("==================================================\n");
	printf("                                                  \n");
	printf("  Base Command:\n");
	printf("              i: cover and input.\n");
	printf("             i+: input after it.\n");
	printf("              w: save.\n");
	printf("              q: quit.\n");
	printf("             wq: save and quit.\n");
	printf("==================================================\n");

	while (com[0] != 'q' && com[1] != 'q') {
		printf("Please input command: ");
		i = read(0, com, 2);
		if (com[0] == 'i') {
			printf("Input: ");
			int end = read(0, bufw, 2048);
			bufw[end] = 0;
			if (com[1] == '+') {
				
				i = read(fd, bufr, 2048);
				i = strlen(bufr);
				
				for (j = 0; j < i; j++) {
					buf[j] = bufr[j];
					buf[j+1] = '\0';
				}
			}
			for (j = 0; j < strlen(bufw); j++, i++) {
				buf[i] = bufw[j];
				buf[i+1] = '\0';
			}
		}
		else if (com[0] == 'w') {
			if (com[1] == 'q') {
				printf("File %s saved and quit!\n", argv[1]);
				n = write(fd, buf, strlen(buf));	
			}
			else if (com[1] == '\0') {
				printf ("Save successed!\n");
				is_saved = 1;
			}	
		}
		else if (com[0] == 'q') {
			if (is_saved != 1) {
				printf("Didn`t save change, still want to quit? (y/n): ");
				i = read(0, com, 2);
				if (com[0] == 'y') {
					com[0] = com[1] = 'q';
				}
				else {
					memset(com, 0, 2);
				}
			}
			else {
				printf("File %s saved and quit!\n", argv[1]);
				n = write(fd, buf, strlen(buf));
				//close(fd);
			}
		}
	}
	close(fd);
	return 0;
}
