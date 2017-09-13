#include "stdio.h"
#include "../include/sys/fs.h"

int main(int argc, char * argv[])
{
	int i = -1, j;
	char bufr[1024];
	char filename[256];
	memset(filename, 0, 256);
	memset(bufr, 0, 1024);
	i = list("\\", bufr);

	if(i != 0) {
		printf("Failed to load the list!");
		return -1;
	}
	j = 0;
	for(i = 0; bufr[i] != 0; i++) {
		if (bufr[i] != '|') {
			filename[j] = bufr[i];
			j++;
		}
		else {
			if (strcmp(filename, "dev_tty0") == 0 ||
				strcmp(filename, "dev_tty1") == 0 ||
				strcmp(filename, "dev_tty2") == 0 ||
				strcmp(filename, "cmd.tar") == 0 ||
				strcmp(filename, "kernel.bin") == 0 ||
				strcmp(filename, "echo") == 0 ||
				strcmp(filename, "pwd") == 0 ||
				strcmp(filename, "touch") == 0 ||
				strcmp(filename, "cat") == 0 ||
				strcmp(filename, "ls") == 0 ||
				strcmp(filename, "vi") == 0 ||
				strcmp(filename, "cpuinfo") == 0 ||
				strcmp(filename, "rm") == 0 ||
				strcmp(filename, "mv") == 0 ) {
				
				memset(filename, 0, 256);
				j = 0;
			}
			else {
				printf("%s", filename);
				printf(" ");
				memset(filename, 0, 256);
				j = 0;
			}
		}
	}


	printf("\n");
	return 0;
}
