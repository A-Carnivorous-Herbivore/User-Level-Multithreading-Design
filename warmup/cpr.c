#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
/* make sure to use syserror() when a system call fails. see common.h */

void usage(){
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}
void copyFile(char* ori, char* dest){
	int fd1, flag1;
	flag1 = O_RDONLY;
	fd1 = open(ori, flag1);	//Copy file
	if(fd1 < 0)
		syserror(open,ori);
	struct stat status;
	int statIndicator = stat(ori,&status);
	if(statIndicator < 0)
		syserror(stat,ori);

	int fd2;
	//int flag2;
	//flag2 = O_RDWR | O_CREAT;
	//fd2 = open(ori, flag2);	//Destination file
	fd2 = creat(dest,S_IRWXU);
	if(fd2 < 0)
		syserror(creat,dest);
	char buf[4096];
	int ret;
	int result;
	ret = read(fd1,buf,4096);
	if(ret < 0)
		syserror(read,ori);
	result = write(fd2,buf,ret);
	if(result < 0)
		syserror(write,dest);
	while(ret != 0){
		ret = read(fd1,buf,4096);
		if(ret < 0)
			syserror(read,ori);
		result = write(fd2,buf,ret);
		if(result < 0)
			syserror(write,dest);
	
	}
	int chmodIndicator = chmod(dest,status.st_mode);
	 if(chmodIndicator < 0)
			syserror(chmod,dest);
	int first = close(fd1);
	if(first < 0)
		syserror(close,ori);
	int second = close(fd2);
	if(second < 0)
		syserror(close,dest);

}
void copyDirect(char* ori, char* dest){
	struct stat status;
	int statIndicator = stat(ori, &status);
	if(statIndicator < 0)
		syserror(stat,ori);
	if(S_ISREG(status.st_mode)){
		copyFile(ori,dest);
	}else if(S_ISDIR(status.st_mode)){
		DIR* dirIndicator = opendir(ori);		//Open directory
		if(!dirIndicator)
			syserror(opendir,ori);
		struct dirent* read = readdir(dirIndicator);
		int newDir = mkdir(dest,S_IRWXU);
		if(newDir < 0)
			syserror(mkdir,dest);
		//int chmodIndicator = chmod(ori,status.st_mode);	//Change Validity
		//if(chmodIndicator < 0)
	//		syserror(chmod,newDestAddress);
		while(read){
			if(strcmp(read->d_name,".")==0 || strcmp(read->d_name,"..")==0){
			
			}
			
			else{
				char tempOri[100];
				char tempDest[100];
				strcpy(tempOri,ori);
				strcpy(tempDest,dest);
				strcat(tempOri,"/");
				strcat(tempOri,read->d_name);
				strcat(tempDest,"/");
				strcat(tempDest,read->d_name);	
				copyDirect(tempOri,tempDest);
			}
			read = readdir(dirIndicator);
		}
        int chmodIndicator = chmod(dest,status.st_mode);
        if(chmodIndicator < 0)
			syserror(chmod,dest);
		int closeIndicator = closedir(dirIndicator);
		if(closeIndicator < 0)
			syserror(closedir,ori);
	}
}
int main(int argc, char *argv[]){
	if (argc != 3) {
		usage();
	}
	copyDirect(argv[1],argv[2]);
	return 0;
}
