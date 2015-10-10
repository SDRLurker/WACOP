#include<stdio.h>
#include<time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/un.h>

int main(int argc, char* argv[])
{
	struct sockaddr_in sa;
	int fd_skt;
	int dt;
	time_t t;
	struct tm now;
	char order[100];
	FILE *fp;
	char ip[30];

	if(argc>=2){
		scanf("%s",ip);
		fp = fopen("/mnt/mtd/project/ip.txt","wt");
		if(!fp) return 2;
		fprintf(fp,"%s",ip);
		fclose(fp);
		return 0;
	}
	
	fp = fopen("/mnt/mtd/project/ip.txt","rt");
	if(!fp) return 2;
	fscanf(fp,"%s",ip);
	fclose(fp);
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(5000);
	sa.sin_addr.s_addr = inet_addr(ip);

	fd_skt = socket(AF_INET, SOCK_STREAM, 0);
	connect(fd_skt, (struct sockaddr *)&sa, sizeof(sa));
	read(fd_skt,&now.tm_sec,sizeof(int));
	read(fd_skt,&now.tm_min,sizeof(int));
	read(fd_skt,&now.tm_hour,sizeof(int));
	read(fd_skt,&now.tm_mday,sizeof(int));
	read(fd_skt,&now.tm_mon,sizeof(int));
	read(fd_skt,&now.tm_year,sizeof(int));
printf("sec : %d, min : %d, hour : %d, day : %d, month : %d, year : %d\n",
	now.tm_sec, now.tm_min, now.tm_hour, now.tm_mday, now.tm_mon+1, 
	now.tm_year+1900);
	
	sprintf(order,"date %02d%02d%02d%02d%4d",now.tm_mon+1,
	now.tm_mday,now.tm_hour,now.tm_min,now.tm_year+1900);
	//printf("%ld, %s\n",t, order);
	system(order);
	close(fd_skt);
}
