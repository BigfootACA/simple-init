#include<fcntl.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

void vibrate(char*dev,int time){
	if(time<0||time>0xFFFF)return;
	int wr;
	char buff[8]={0};
	snprintf(buff,7,"%d\n",time);
	if((wr=open(dev,O_WRONLY))<0)return;
	write(wr,buff,strlen(buff));
	close(wr);
}
