#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
	int fd;
	fd = open("txt/out.txt", O_RDONLY);

	char curChar;
	int count = 0;
	while(read(fd, &curChar, 1) > 0){
		if(curChar == '+'){
			for(int i = 0; i < 5; i++){
				read(fd, &curChar, 1);
				if(curChar != '+')
					i = 6;
				if(i == 4){
					count++;
					printf("%d\n", count);
				}
			}
		}
		//printf("%c\n", curChar);
	}
	printf("%d articles\n", count);
}
