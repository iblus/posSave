#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <dirent.h>
#include <time.h>
#include <signal.h>

#include <sys/ioctl.h>


static int setup_port(char* com_path, int baud, int databits, int parity, int stopbits)
{
    char _cmd[255];
    sprintf(_cmd,"stty -F %s speed %d", com_path, baud);
    printf("shell cmd is :%s\n",_cmd);
    return system(_cmd);
}
static int sendSetCommond(void)
{
    int ret = 0;
    char ptr[1];
    int cun=0;
    int ttyFp = 0;

    ttyFp = open("/dev/ttyO4", O_RDWR | O_NOCTTY | O_NDELAY);
    if(ttyFp <=0)
    {
        printf("open tty:%s fail\n","/dev/ttyO4");
        return -1;
    }

    while(cun++<8)
    {
        for(int i=0; i<88; i++)
        {
            ptr[0] = 0;
            if(i==0)
            {
                ptr[0] = 0x55;
            }
            if(i==1)
            {
                ptr[0] = 0xAA;
            }
            if(i==86)
            {
                ptr[0] = 0x11;
            }
            if(i==87)
            {
                ptr[0] = 0x10;
            }

            ret = write(ttyFp, (void *)ptr, 1);
            if(ret < 0)
            {
                printf("%s--%d, send to set tty fail cun:%d\n", __FILE__, __LINE__,cun);
                return -3;
            }
            usleep(100);
        }
        printf("send to set tty success cun:%d\n",cun);
        usleep(1000);
    }
    printf("send all  set tty success\n");
    return 0;
}

int main(void)
{
    sendSetCommond();
    return 1;
}