/*
	Main program.

	Michał Obrembski (byku@byku.com.pl)
*/
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>
#include "http.h"
#include "SpeedtestConfig.h"
#include "SpeedtestServers.h"
#include "Speedtest.h"

#define __NUM__

#ifdef __NUM__
#define NUM 5
#else
#define NUM 10
#endif

int sortServers(SPEEDTESTSERVER_T **srv1, SPEEDTESTSERVER_T **srv2)
{
	return((*srv1)->distance - (*srv2)->distance);
}

float getElapsedTime(struct timeval tval_start) {
    struct timeval tval_end, tval_diff;
    gettimeofday(&tval_end, NULL);
    tval_diff.tv_sec = tval_end.tv_sec - tval_start.tv_sec;
    tval_diff.tv_usec = tval_end.tv_usec - tval_start.tv_usec;
    if(tval_diff.tv_usec <0) {
        --tval_diff.tv_sec;
        tval_diff.tv_usec += 1000000;
    }
    return (float)tval_diff.tv_sec + (float)tval_diff.tv_usec/1000000;
}

void parseCmdLine(int argc, char **argv) {
    int i;
    for(i=1;i<argc;i++) {
        if(strcmp("--help", argv[i])==0 || strcmp("-h", argv[i])==0) {
            printf("Usage (options are case sensitive):\n\
            \t--help - Show this help.\n\
            \t--server URL - use server URL, don'read config.\n\
            \t--upsize SIZE - use upload size of SIZE bytes.\n"
            "\nDefault action: Get server from Speedtest.NET infrastructure\n"
            "and test download with 15MB download size and 1MB upload size.\n");
            exit(1);
        }
        if(strcmp("--server", argv[i])==0) {
            downloadUrl = malloc(sizeof(char) * strlen(argv[i+1]) + 1);
            strcpy(downloadUrl,argv[i + 1]);
        }
        if(strcmp("--upsize", argv[i])==0) {
            totalToBeTransfered = strtoul(argv[i + 1], NULL, 10);
        }
    }
}

int main(int argc, char **argv)
{
    time_t start_time = time(NULL);
#define DOWNSTREAMPATH "/usr/idianjia/downstream"
#define UPSTREAMPATH "/usr/idianjia/upstream"
    FILE *fp_down = NULL;
    FILE *fp_up = NULL;
    int fd = 0;
    int ch = 0;
    int m = 0;
    int n = 0;
    char downstream_speed[16] = {0};
    char upstream_speed[16] = {0};
    char cmd1[64] = {0};
    char cmd2[64] = {0};
    sprintf(cmd1,"sed -i '1d' %s &",DOWNSTREAMPATH);
    sprintf(cmd2,"sed -i '1d' %s &",UPSTREAMPATH);
    parseCmdLine(argc,argv);
    if(downloadUrl == NULL) {
        serverList = getServers(&serverCount);
        speedTestConfig = getConfig();
        printf("Grabbed %d servers\n",serverCount);
        printf("Your IP: %s And ISP: %s\n",
            speedTestConfig->ip, speedTestConfig->isp);
        printf("Lat: %f Lon: %f\n", speedTestConfig->lat, speedTestConfig->lon);
        for(i=0;i<serverCount;i++)
            serverList[i]->distance = haversineDistance(speedTestConfig->lat,
                speedTestConfig->lon,
                serverList[i]->lat,
                serverList[i]->lon);

        qsort(serverList,serverCount,sizeof(SPEEDTESTSERVER_T *),
                    (int (*)(const void *,const void *)) sortServers);

        printf("Best Server URL: %s\n\t Name:%s Country: %s Sponsor: %s Dist: %ld\n",
            serverList[0]->url,serverList[0]->name,serverList[0]->country,
            serverList[0]->sponsor,serverList[0]->distance);
        printf("Best Server URL: %s\n\t Name:%s Country: %s Sponsor: %s Dist: %ld\n",
            serverList[1]->url,serverList[1]->name,serverList[1]->country,
            serverList[1]->sponsor,serverList[1]->distance);
        downloadUrl = getServerDownloadUrl(serverList[0]->url);
        uploadUrl = serverList[0]->url;
    }
    else {
        uploadUrl = downloadUrl;
        downloadUrl = getServerDownloadUrl(downloadUrl);
    }

    /* Testing download... */
    sockId = httpGetRequestSocket(downloadUrl);
    if(sockId == 0)
        return 1;
    size = -1;
    totalTransfered = 0;
    gettimeofday(&tval_start, NULL);
    while(size!=0)
    {
    	size = httpRecv(sockId,buffer,1500);
		totalTransfered += size;
    }
    elapsedSecs = getElapsedTime(tval_start);
    speed = (totalTransfered*8/elapsedSecs)/1024;
    httpClose(sockId);
    printf("Bytes %lu downloaded in %.2f seconds %.2f kbit/s\n",
        totalTransfered,elapsedSecs,speed);

    fp_down = fopen(DOWNSTREAMPATH,"a+");
    if(fp_down != NULL)
    {
        fd = fileno(fp_down);
        if(flock(fd,LOCK_EX) == 0)
        {
            while((ch = fgetc(fp_down)) != EOF)
            {
                if(ch == '\n')
                {
                    m++;
                }
            }
            printf("m=%d\n",m);
            if(m >= NUM)
                system(cmd1);
            sprintf(downstream_speed,"%.2f\n",speed);
            fputs(downstream_speed,fp_down);
        }
        else
            printf("file lock unsuccessfully\n");

        if(flock(fd,LOCK_UN) == 0)
            printf("file unlock successfully\n");
        else
            printf("file unlock unsuccessfully\n");
    	fclose(fp_down);
    }
    else
    {
        printf("file open unsuccessfully\n");
        return 1;
    }
    /* Testing upload... */
    totalTransfered = totalToBeTransfered;
    sockId = httpPutRequestSocket(uploadUrl,totalToBeTransfered);
    if(sockId == 0)
        return 1;
    gettimeofday(&tval_start, NULL);
    while(totalTransfered != 0)
    {
        for(i=0;i<1500;i++)
            buffer[i] = (char)i;
        size = httpSend(sockId,buffer,1500);
        /* To check for unsigned overflow */
        if(totalTransfered < size)
            break;
        totalTransfered -= size;
    }
    elapsedSecs = getElapsedTime(tval_start);
    speed = (totalToBeTransfered*8/elapsedSecs)/1024;
    httpClose(sockId);
    printf("Bytes %lu uploaded in %.2f seconds %.2f kbit/s\n",
        totalToBeTransfered,elapsedSecs,speed);

    fp_up = fopen(UPSTREAMPATH,"a+");
    if(fp_up != NULL)
    {
        fd = fileno(fp_up);
        if(flock(fd,LOCK_EX) == 0)
        {
            while((ch = fgetc(fp_up)) != EOF)
            {
                if(ch == '\n')
                {
                    n++;
                }
            }
            printf("n=%d\n",n);
            if(n >= NUM)
                system(cmd2);
            sprintf(upstream_speed,"%.2f\n",speed);
            fputs(upstream_speed,fp_up);
        }
        else
            printf("file lock unsuccessfully\n");

        if(flock(fd,LOCK_UN) == 0)
            printf("file unlock successfully\n");
        else
            printf("file unlock unsuccessfully\n");
    	fclose(fp_up);
    }
    else
    {
        printf("file open unsuccessfully\n");
        return 1;
    }

    for(i=0;i<serverCount;i++){
    	free(serverList[i]->url);
    	free(serverList[i]->name);
    	free(serverList[i]->sponsor);
    	free(serverList[i]->country);
    	free(serverList[i]);
    }
    free(downloadUrl);
    free(uploadUrl);
    //free(serverList);
    free(speedTestConfig);
    time_t end_time = time(NULL);
    printf("test use time:%ld\n",(end_time-start_time));
	return 0;
}

