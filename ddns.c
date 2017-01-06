#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(a) closesocket(a)
#else
#include <sys/socket.h>
#include <netdb.h>
#include <resolv.h>
#endif
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include "base64.h"

typedef struct {
	char user[64];
	char pass[64];
	char host[128];
	char url[128];
	char agent[128];
	long time;
} DDNS_OPTION;
DDNS_OPTION g_option;

const char* g_programname;
const char* VERSION="0.1";

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
	fflush(stdout);
}

void print_usage()
{
	fprintf(stderr, "Usage: %s [options]\n",
			g_programname);
	fprintf(stderr,
			"  -f  --config file\tRead config from file.\n"
			"  -h  --help\t\tDisplay this usage information.\n"
			"  -u  --user username\tUser Name to login.\n"
			"  -p  --pass password\tPassword to login.\n"
			"  -H  --host hostname\tHostname of the ddns server.\n"
			"  -U  --url url\t\tUrl without hostname to send get request.\n"
			"  -t  --time [s]\tOptional. Time between two request. Default:900.\n"
			"  -A  --agent [s]\tOptional. User-Agent. Default:ddnsv%s.\n",VERSION
		   );
}

void rtrim(char *str)
{
	char *c = str + strlen(str) - 1;
	while (c>str && (*c=='\n' || *c==' ' || *c=='\r')) {
		*c = '\0';
		--c;
	}
	if(c == str)	 //此处
		*c = '\0';
}
int parse_configfile(const char* configfile)
{
	FILE* fp=fopen(configfile,"r");
	if(!fp) {
		return 2;
	}
	char buf[1024], *c;
	while (fgets(buf, 1024, fp) != NULL) {
		if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n' || buf[0] == ';') {
			continue;
		}
		rtrim(buf);
		c = buf;
		while (*c!=' '&&*c!='='&&*c!='\0')
			++c;
		if (*c != '\0') {
			*c=0;
			if(strcmp(buf,"user")==0){
				strcpy(g_option.user,c+1);
			}else if(strcmp(buf,"pass")==0){
				strcpy(g_option.pass,c+1);
			}else if(strcmp(buf,"host")==0){
				strcpy(g_option.host,c+1);
			}else if(strcmp(buf,"url")==0){
				strcpy(g_option.url,c+1);
			}else if(strcmp(buf,"agent")==0){
				strcpy(g_option.agent,c+1);
			}else if(strcmp(buf,"time")==0){
				g_option.time=atol(c+1);
			}
		}
	}
	fclose(fp);
	return 0;
}
void dumpconfig(){
	printf("user=%s\n",g_option.user);
	printf("pass=%s\n",g_option.pass);
	printf("host=%s\n",g_option.host);
	printf("url=%s\n",g_option.url);
	printf("agent=%s\n",g_option.agent);
	printf("time=%ld\n",g_option.time);
}
int readargs(int argc,char* argv[])
{
	int next_option;
	const char* const short_option = "hf:u:p:H:U:t:A:";
	const struct option long_option[]= {
		{"help",0,NULL,'h'},
		{"user",1,NULL,'u'},
		{"pass",1,NULL,'p'},
		{"host",1,NULL,'H'},
		{"url",1,NULL,'U'},
		{"time",1,NULL,'t'},
		{"agent",1,NULL,'A'},
		{NULL,0,NULL,0}
	};
	g_option.time=900;
	snprintf(g_option.agent,128,"ddnsv%s",VERSION);
	do {
		next_option = getopt_long (argc, argv, short_option,
								   long_option, NULL);
		switch (next_option) {
		case 'h':
			print_usage();
			exit(0);
		case 'f':
		{
			int ret=parse_configfile(optarg);
			if(ret) {
				fprintf(stderr,"cannot open %s\n",optarg);
				return ret;
			}
		}
			break;
		case 'u':
			strcpy(g_option.user,optarg);
			break;
		case 'p':
			strcpy(g_option.pass,optarg);
			break;
		case 'H':
			strcpy(g_option.host,optarg);
			break;
		case 'U':
			strcpy(g_option.url,optarg);
			break;
		case 'A':
			strcpy(g_option.agent,optarg);
			break;
		case 't':
			g_option.time=atol(optarg);
			break;
		case '?':
			print_usage();
			return(22);
		case -1:
			break;
		default:
			abort();
		}
	} while (next_option != -1);
	//dumpconfig();
	if(!g_option.host[0]){
		print_usage();
		return 22;
	}
	if(!g_option.url[0]){
		g_option.url[0]='/';
		g_option.url[1]=0;
	}
	return 0;
}

//构建Authorization
void makeAuth(char* username,char* passwd,char* inOut)
{
	char buf[128];
	snprintf(buf,128,"%s:%s",username,passwd);
	strcpy(inOut,"Authorization: Basic ");
	char* result=b64_encode(buf,strlen(buf));
	strcat(inOut,result);
	free(result);
}

int main(int argc,char* argv[])
{
	memset(&g_option,0,sizeof(DDNS_OPTION));
	g_programname = argv[0];
	int ret=readargs(argc,argv);
	if(ret) {
		exit(ret);
	}
#ifdef WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		wVersionRequested = MAKEWORD( 2, 0 );
		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) {
			exit(err);
		}
	}
#endif

	char request[512];
	if(g_option.user[0])//if need auth
	{
		//构建http请求
		char auth[128];
		makeAuth(g_option.user,g_option.pass,auth);

		const char *req="GET %s HTTP/1.0";
		const char *agent="User-Agent: %s";
		const char *host="Host: %s";
		char temp[512];
		snprintf(temp,512,"%s\r\n%s\r\n%s\r\n%s\r\n\r\n",
				 req,
				 host,
				 auth,
				 agent);
		snprintf(request,512,temp,g_option.url,g_option.host,g_option.agent);
	}else{
		const char *req="GET %s HTTP/1.0";
		const char *agent="User-Agent: %s";
		const char *host="Host: %s";
		char temp[512];
		snprintf(temp,512,"%s\r\n%s\r\n%s\r\n\r\n",
				 req,
				 host,
				 agent);
		snprintf(request,512,temp,g_option.url,g_option.host,g_option.agent);

	}
	int requestlen=strlen(request);
	//printf("%s\n",request);
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
		const char *hostname=g_option.host;
		int ret=getaddrinfo(hostname,"80",&hints,&ainfo);
		if(ret) {
			mylog("getaddrinfo fail:%s",gai_strerror(ret));
#ifndef WIN32
			res_init();//重新读取resolv.conf
#endif
			needsleep=5;
			continue;
		}
		if(!ainfo||!ainfo->ai_addr) {
			mylog("getaddrinfo got nothing:%s",strerror(errno));
			needsleep=5;
			if(ainfo) {
				freeaddrinfo(ainfo);
			}
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
				rtrim(respon+4);
				mylog(respon+4);
				needsleep=g_option.time;
			} else {
				mylog(buffer);
				needsleep=g_option.time/5;
			}
		}
		freeaddrinfo(ainfo);
		close(sockfd);
	}
	return 0;
}
