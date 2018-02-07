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
#include "loop_queue.h"
#include "uart.h"

#define LOCK(x)                        						do{		\
    int lockRet; \
    if ((lockRet = pthread_mutex_lock( &(x) )) != 0)	\
    printf("%s, --%d, lockRet = %d\n", __FUNCTION__, __LINE__, lockRet);\
    }while(0)

#define UNLOCK(x)											do{		\
    int lockRet; \
    if ((lockRet = pthread_mutex_unlock( &(x) )) != 0) \
    printf("%s, --%d, lockRet = %d\n", __FUNCTION__, __LINE__, lockRet);\
    }while(0)
//==============================================================================

typedef struct _tty_opt
{
    char *ttyPath;
    int ttyFp;
    int baud;

    char *savePath;
    FILE *saveFp;

    char *name;
    LOOP_QUEUE ttyQueue;
    pthread_mutex_t queueLock;

    int timeOut;
    int perLen;
} TTYOPT;

int IsRun = 1;
TTYOPT setTty;

int isShiBiaoOut = 0;
int isDaoHangOut = 0;
int isGPSOut = 0;

static int sendSetCommond(void)
{
    int ret = 0;
    char ptr[1];
    int cun=0;
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

            ret = write(setTty.ttyFp, (void *)ptr, 1);
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

static void *tty_receive_pthread(void *arg)
{
    int i = 0;
    int selectRet, UartReadLen=0;
    int ttyFp =0;
    fd_set _readFdsr;
    struct timeval _tv;
    _tv.tv_sec = 0;
    _tv.tv_usec = 1000;


    TTYOPT *ttyOpt = (TTYOPT*)arg;
    ttyFp = ttyOpt->ttyFp;
    LOOP_QUEUE *CommandQueue = &(ttyOpt->ttyQueue);
    char UartReadContent[12500];
    memset(UartReadContent, 0, sizeof(UartReadContent));
    printf("goto tty_receive_pthread :%s \n",ttyOpt->name);

    while(IsRun)
    {
        FD_ZERO(&_readFdsr);
        FD_SET(ttyFp, &_readFdsr);

        selectRet = select(ttyFp + 1, &_readFdsr, NULL, NULL, &_tv);
        if(selectRet > 0)
        {
            if(FD_ISSET(ttyFp, &_readFdsr))
            {
                UartReadLen = read(ttyFp, UartReadContent, sizeof(UartReadContent));
                //printf("\n tty:%s recive len:: %d\n",ttyOpt->ttyPath, UartReadLen);
#if 0
                for(i = 0; i < UartReadLen; i++)
                {
                    printf("0x%x ", UartReadContent[i]);
                    if(i % 9 == 8)
                        printf("\n");
                }
                printf("\n");
#endif

                if((UartReadLen > 0) && !loop_queue_is_full(CommandQueue)) //this fd has data ,so can recv it
                {
                    LOCK(ttyOpt->queueLock);
                    loop_queue_in(CommandQueue, UartReadContent, UartReadLen);
                    UNLOCK(ttyOpt->queueLock);

                } else
                {
                    printf("tty:%s receive fail:%d %d\n",ttyOpt->ttyPath, UartReadLen,errno);
                }
            }
        } else
        {
        }
        usleep(10);		//pthread delay, reduce CPU occupancy rate
    }

    return (void *)0;
}
#define PROCESS_SIZE 100
static void *tty_process_pthread(void *arg)
{
    TTYOPT *ttyOpt = (TTYOPT*)arg;
    FILE *saveFp = ttyOpt->saveFp;
    LOOP_QUEUE *CommandQueue = &(ttyOpt->ttyQueue);
    char buf[PROCESS_SIZE];

    while(IsRun)
    {
        LOCK(ttyOpt->queueLock);

        if(loop_queue_avaliable_items_count(CommandQueue) >= PROCESS_SIZE)
        {
            loop_queue_out(CommandQueue, buf, sizeof(buf));
            UNLOCK(ttyOpt->queueLock);
            //===
            if(saveFp !=NULL)
            {
                fwrite(buf, sizeof(buf), 1, saveFp);
                fflush(saveFp);
                // printf("save tty data to file:%s\n", ttyOpt->savePath);
            }
            if(strcmp(ttyOpt->name,"shiBiao") == 0)
            {
                isShiBiaoOut = 1;
            }
            if(strcmp(ttyOpt->name,"daoHang") == 0)
            {
                isDaoHangOut = 1;
            }
 			if(strcmp(ttyOpt->name,"GPS1") == 0)
            {
                isGPSOut = 1;
            }
            ttyOpt->timeOut =0 ;
        } else
        {
            ttyOpt->timeOut ++;

            if(strcmp(ttyOpt->name,"shiBiao") == 0)
            {
                if(ttyOpt->timeOut >80)
                {
                    isShiBiaoOut = 0;
                }
            }
            if(strcmp(ttyOpt->name,"daoHang") == 0)
            {
                if(ttyOpt->timeOut >320)
                {
                    isDaoHangOut = 0;
                }
            }
            if(strcmp(ttyOpt->name,"GPS1") == 0)
            {
                if(ttyOpt->timeOut >1000)
                {
                    isGPSOut = 0;
                }
            }

        }
        UNLOCK(ttyOpt->queueLock);
        usleep(1000);		//pthread delay, reduce CPU occupancy rate
    }

    return (void *)0;
}

int init_tty(TTYOPT *ttyOpt)
{
    int ttyFp = 0;
    FILE *saveFp = NULL;

    pthread_t tty_receive_tid;
    pthread_t tty_process_tid;

	ttyFp = uart_init(ttyOpt->ttyPath, ttyOpt->baud, 8, 0, 1);

    if(ttyFp <=0)
    {
        printf("open tty:%s fail\n",ttyOpt->ttyPath);
        return -1;
    }
    ttyOpt->ttyFp = ttyFp;

    saveFp = fopen(ttyOpt->savePath, "wb");
    if(saveFp == NULL)
    {
        printf("create save file:%s fail!\n", ttyOpt->savePath);
        return -1;
    }
    ttyOpt->saveFp = saveFp;
#if 1
	if(strcmp(ttyOpt->name,"daoHang") == 0)
	{
		fclose(ttyOpt->saveFp);
		ttyOpt->saveFp = NULL;
	}
#endif
    initialize_loop_queue(&(ttyOpt->ttyQueue));
    if(pthread_mutex_init(&(ttyOpt->queueLock), NULL) != 0)
    {
        printf("%s--%d, pthread mutex init failed!\n", __FILE__, __LINE__);
        return -1;
    }

    if(pthread_create(&tty_receive_tid, NULL, tty_receive_pthread, (void*)ttyOpt))
    {
        printf("%s--%d, tty_receive_pthread create error!\n", __FILE__, __LINE__);
        return -1;
    }
    if(pthread_create(&tty_process_tid, NULL, tty_process_pthread, (void*)ttyOpt))
    {
        printf("%s--%d, tty_receive_pthread create error!\n", __FILE__, __LINE__);
        return -1;
    }

    return 0;
}

static void stopSystem(int sig)
{
    printf("\nstop system...\n");
    IsRun = 0;
    exit(0);
}

#include <time.h>

//get system local time
static void getTime(char*str, int len)
{
	time_t timep;
	struct tm *p_lt;
	time(&timep);
	p_lt = localtime(&timep);

	memset(str,0,len);

	sprintf(str, "%d-%02d-%02d-%02d-%02d-%02d",
			(1900+p_lt->tm_year), p_lt->tm_mon, p_lt->tm_mday,
			p_lt->tm_hour, p_lt->tm_min, p_lt->tm_sec);
	return;
}

int main(int argc, char *argv[])
{
    IsRun = 1;

	char timeSp[60];
	getTime(timeSp, sizeof(timeSp));

    TTYOPT gps1;
	char gps1SavePath[60];
	memset(gps1SavePath, 0, sizeof(gps1SavePath));
	sprintf(gps1SavePath, "/opt/sdcard/posData/%s.gps1", timeSp);

    gps1.ttyPath = "/dev/ttyO1";
    gps1.baud = 38400;
    gps1.savePath = gps1SavePath;
    gps1.name = "GPS1";

    TTYOPT daoHang;
	char daoHangSavePath[60];
	memset(daoHangSavePath, 0, sizeof(daoHangSavePath));
	sprintf(daoHangSavePath, "/opt/sdcard/posData/%s.daoHang", timeSp);

    daoHang.ttyPath = "/dev/ttyO2";
    daoHang.baud = 115200;
    daoHang.savePath = daoHangSavePath;
    daoHang.name = "daoHang";
    daoHang.timeOut = 0;

    TTYOPT shiBiao;
	char shiBiaoSavePath[60];
	memset(shiBiaoSavePath, 0, sizeof(shiBiaoSavePath));
	sprintf(shiBiaoSavePath, "/opt/sdcard/posData/%s.shiBiao", timeSp);

    shiBiao.ttyPath = "/dev/ttyO3";
    shiBiao.baud = 115200;
    shiBiao.savePath = shiBiaoSavePath;
    shiBiao.name = "shiBiao";
    daoHang.timeOut = 0;
//==================
	printf("gps::  %s\n",gps1.savePath);
	printf("daoHang::  %s\n",daoHang.savePath);
	printf("shiBiao::  %s\n",shiBiao.savePath);
//==================

    setTty.ttyPath = "/dev/ttyO4";
    setTty.baud = 115200;

	setTty.ttyFp = uart_init(setTty.ttyPath, setTty.baud, 8, 0, 1);
    if(setTty.ttyFp <=0)
    {
        printf("open tty:%s fail\n",setTty.ttyPath);
        return -1;
    }


//==================
#if 1
    if(init_tty(&gps1) !=0)
    {
        printf("init tty:%s fail \n",gps1.name);
        return  1;
    }

    if(init_tty(&daoHang) !=0)
    {
        printf("init tty:%s fail \n",daoHang.name);
        return  1;
    }
#endif
    if(init_tty(&shiBiao) !=0)
    {
        printf("init tty:%s fail \n",shiBiao.name);
        return  1;
    }


    signal(SIGINT, stopSystem);

	int sysTime = 0;
    while(1)
    {
		printf("isShiBiaoOut: %d -- isDaoHangOut: %d isGPSOut: %d\n",isShiBiaoOut,isDaoHangOut,isGPSOut);
        if((isShiBiaoOut == 1)&&(isDaoHangOut == 0))
        {
            sendSetCommond();
        }
        if((isShiBiaoOut == 1)&&(isDaoHangOut == 1))
        {
			close(setTty.ttyFp);
            break;
        }
        sleep(10);
		sysTime ++;
		printf("system is in noDaohang %d s\n",sysTime*10);
        
    }
	sysTime = 0;
    while(1)
    {
        printf("start save daohang's data\n");
		while(1)
		{
			sysTime ++;
        	sleep(30);
			printf("system has saving %d min\n",sysTime/2);
		}
        printf("\n 8h time is done!\n");
        return 0;
    }
    return 0;
}
