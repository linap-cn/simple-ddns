#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h> 
#include "base64.h"

void usage()
{
	fprintf(stderr,"Usage: oray [username] [passwd]\n");
}

void mylog(char* fmt,...)
{
	time_t now;
	struct tm * timeinfo;
	char timestr [128];
	time (&now);
	timeinfo = localtime (&now);
	strftime(timestr,sizeof(timestr),"%Y/%m/%d %H:%M:%S",timeinfo);

	va_list arglist;
	va_start(arglist, fmt);
	char buf[2048];
	vsnprintf(buf, sizeof(buf), fmt, arglist);
	va_end(arglist);

	printf("%s\t%s\n",timestr,buf);
}

//构建Authorization
void makeAuth(char* username,char* passwd,char* inOut)
{
	//构建Authorization
	char buf[64];
	snprintf(buf,240,"%s:%s",username,passwd);
	strcpy(inOut,"Authorization: Basic ");
	char* result=b64_encode(buf,strlen(buf));
	strcat(inOut,result);
	free(result);
}

int main(int argc,char* argv[])
{
	char request[512];
	if(argc<3) {
		usage();
		return 1;
	}else{
		//构建http请求
		char auth[128];
		makeAuth(argv[1],argv[2],auth);

		const char *req="GET /ph/update HTTP/1.0";
		const char *agent="User-Agent: Oray";
		const char *host="Host: ddns.oray.com";

		snprintf(request,512,"%s\r\n%s\r\n%s\r\n%s\r\n\r\n",
				 req,
				 host,
				 auth,
				 agent);
	}
	int requestlen=strlen(request);

	int needsleep=0;
	for(;;) {
		if(needsleep) {
			sleep(needsleep);
			needsleep=0;
		}
		//使用socket进行http连接
		struct addrinfo hints,*ainfo;
		memset(&hints,0,sizeof(struct addrinfo));
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_STREAM;
		const char *hostname="ddns.oray.com";
		if(getaddrinfo(hostname,"80",&hints,&ainfo)) {
			mylog("getaddrinfo fail:%s",strerror(errno));
			needsleep=5;
			continue;
		}
		if(!ainfo||!ainfo->ai_addr) {
			mylog("getaddrinfo got nothing:%s",strerror(errno));
			needsleep=5;
			freeaddrinfo(ainfo);
			continue;
		}
		int sockfd = socket(AF_INET,SOCK_STREAM,0);
		if( sockfd < 0) {
			mylog("create socket fail:%s",strerror(errno));
			needsleep=5;
			freeaddrinfo(ainfo);
			continue;
		}
		if(connect(sockfd,ainfo->ai_addr, ainfo->ai_addrlen) < 0) {
			mylog("connect fail:%s",strerror(errno));
			needsleep=5;
			freeaddrinfo(ainfo);
			close(sockfd);
			continue;
		}
		send(sockfd,request,requestlen,0);
		char buffer[1024];
		memset(buffer,0,sizeof(buffer));
		int length = recv(sockfd,buffer,sizeof(buffer),0);
		if(length < 0) {
			mylog("recv fail:%s",strerror(errno));
			needsleep=5;
		} else if(length==0) {
			mylog("recv empty:%s",strerror(errno));
			needsleep=5;
		} else {
			char* respon=strstr(buffer,"\r\n\r\n");
			if(respon) {
				mylog(respon+4);
				needsleep=600;
			} else {
				mylog(buffer);
				needsleep=300;
			}
		}
		freeaddrinfo(ainfo);
		close(sockfd);
	}
	return 0;
}