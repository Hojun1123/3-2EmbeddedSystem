#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define DEVICE_FILE_NAME "/dev/VirtualDisk"
#define MY_IOCTL_NUMBER    100
#define IOCTL_READ_BUFFER      _IOW(MY_IOCTL_NUMBER, 0, int)
#define IOCTL_WRITE_BUFFER     _IOW(MY_IOCTL_NUMBER, 1, int)
#define IOCTL_BUF_INFORMATION   _IOW(MY_IOCTL_NUMBER, 2, int)

int main(int argc, char **argv) {
 	int dev = open(DEVICE_FILE_NAME, O_RDWR);
        int buf_type = atoi(argv[1]);
	int buf_size = 0;
	if(argc == 3)
	        buf_size = atoi(argv[2]);

	if(dev < 0) {
		printf("open error\n");
		exit(EXIT_FAILURE);
	}

        if(buf_type == 1) {             //Read Buffer
                ioctl(dev, IOCTL_READ_BUFFER, &buf_size);
        }
        else if(buf_type == 2) {        //Write Buffer
                ioctl(dev, IOCTL_WRITE_BUFFER, &buf_size);
	}
	else if(buf_type == 3) {   //Buffer Information
		printf("Please, Check Kernel Message for Buffer Information\n");
		ioctl(dev, IOCTL_BUF_INFORMATION, &buf_type);
	}
        else {                           //exception
                printf("Error: please check input Number!\n");
	}
        close(dev);
        return 0;

}