#include<time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/un.h>

int main(void)
{
	struct sockaddr_in sa;
	int fd_skt, fd_client;
	time_t t;
	struct tm *now;
	char order[100];
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(5000);
	sa.sin_addr.s_addr = inet_addr("192.168.48.197");

	fd_skt = socket(AF_INET, SOCK_STREAM, 0);
	bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa));
	listen(fd_skt, SOMAXCONN);
	fd_client = accept(fd_skt, NULL, 0);
	time(&t);
	now = localtime(&t);
	write(fd_client, &now->tm_sec, sizeof(int));
	write(fd_client, &now->tm_min, sizeof(int));
	write(fd_client, &now->tm_hour, sizeof(int));
	write(fd_client, &now->tm_mday, sizeof(int));
	write(fd_client, &now->tm_mon, sizeof(int));
	write(fd_client, &now->tm_year, sizeof(int));
	printf("sec : %d, min : %d, hour : %d, day : %d, mon : %d, year : %d",
	now->tm_sec, now->tm_min, now->tm_hour, now->tm_mday,
	now->tm_mon+1, now->tm_year+1900);
	close(fd_skt);
	close(fd_client);
}
