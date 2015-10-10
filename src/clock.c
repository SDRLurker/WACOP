#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

unsigned long buff = 0L, clockfnd; // + clockfnd
int fnd,key,lcd;
char digit[] = {'3','6','9','#','2','5','8','0','1','4','7','*'};
char ch=8;	// initialization to '1'
int load_Bitmap24(const char* bmps);
void getsubstr(char *,int,int,char *);
void get_negative(int,int,int,int *,int *,int *,int *);

struct tm *now;
time_t t;
static int mode=0;

void call_watch(int signum) // Changed
{
	switch(digit[ch])
	{
		case '1':
			mode = 0;
			buff = 0;
			break;
		case '2':
			mode = 2;
			buff = 0;
			break;
		case '4':
			if(mode!=0)	mode=1; 	break;
		case '5':
			if(mode!=0)	mode=2;		break;
		default:
			break;
	}

	switch(mode){
		case 0:	time(&t); now = gmtime(&t); break;
		case 1: buff++; if(buff>0 && (buff%10000)%6000==0) buff+=4000;
			break;
		case 2:	break;
	}
}

void* func_fnd(void* param)
{
	while(1){
		read(key,&ch,1);
		//pushed=1;
		if(digit[ch]=='#') break;
	}
}

int main(int argc, char *argv[])
{
	int LY, LM, LD, LP;
	struct sigaction act;
	struct itimerval itv;
	char today[20];
	char ltoday[20];
	unsigned long command;
	pthread_t fnd_thread;
	int oldday;

	memset(&act,0,sizeof(act));
	act.sa_handler = call_watch;
	sigaction(SIGALRM, &act, NULL);
	memset(&itv,0,sizeof(itv));
	itv.it_interval.tv_usec = 10000;
	itv.it_value.tv_usec = 10000;
	setitimer(ITIMER_REAL, &itv, NULL);

	fnd = open("/dev/FNDS",O_WRONLY);
	if(fnd==-1) return 2;
	ioctl(fnd,81);
	
	key = open("/dev/KEY",O_RDONLY);
	lcd = open("/dev/TXTLCD",O_WRONLY|O_NDELAY);
	if(key==-1 || lcd==-1) return 2;
	pthread_create(&fnd_thread,NULL,func_fnd,NULL);	

nextday:
	time(&t); now = gmtime(&t);
	sprintf(today,"%4d-%02d-%02d/",1900+(now->tm_year),1+(now->tm_mon),now->tm_mday);
	oldday = now->tm_mday;

	switch(now->tm_wday) {
		case 0 : strcat(today,"SUN"); break;
		case 1 : strcat(today,"MON"); break;
		case 2 : strcat(today,"WED"); break;
		case 3 : strcat(today,"THU"); break;
		case 4 : strcat(today,"FRI"); break;
		case 5 : strcat(today,"SAT"); break;
	}

	command = 0x1; ioctl(lcd,1,&command,4);
	command = 0x0C; ioctl(lcd,1,&command,4);
	//pthread_create(&fnd_thread,NULL,func_fnd,NULL);
	write(lcd,today,strlen(today));
	get_negative(1900+(now->tm_year),1+(now->tm_mon),now->tm_mday,&LY,&LM,&LD,&LP);
	LD--;
	sprintf(ltoday,"Lunar:%4d-%02d-%02d",LY,LM,LD);
	command = 0x80 | 0x40; ioctl(lcd,1,&command,4);
	write(lcd,ltoday,strlen(ltoday));

	if(LD==30 || LD==0) loadBitmap24("0.bmp");
	else if(LD>=2 && LD<=5) loadBitmap24("1.bmp");
	else if(LD>=6 && LD<=8) loadBitmap24("2.bmp");
	else if(LD>=9 && LD<=13) loadBitmap24("3.bmp");
	else if(LD>=14 && LD<=16) loadBitmap24("4.bmp");
	else if(LD>=17 && LD<=20) loadBitmap24("5.bmp");
	else if(LD>=21 && LD<=23) loadBitmap24("6.bmp");
	else if(LD>=24 && LD<=29) loadBitmap24("7.bmp");
	
	while(1){
		if(digit[ch]=='#') break;
		if(mode==1 || mode==2) write(fnd,&buff,4);
		if(mode==0){
			clockfnd = now->tm_sec;
			clockfnd = clockfnd + (now->tm_min*100);
			clockfnd = clockfnd + (now->tm_hour*10000);
			write(fnd,&clockfnd,4);
		}
		if(now->tm_mday!=oldday) goto nextday;
	}
	pthread_join(fnd_thread,NULL);
	loadBitmap24("8.bmp");
	close(key);
	command = 0x1; ioctl(lcd,1,&command,4);
	close(lcd);
	close(fnd);
	return 0;
}
