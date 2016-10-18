#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>

#define MESSAGE_LENGTH 256

int arr1[10],arr2[10];
char GatewayPort[7];
char GatewayIP[30],Sensor[50],SensorPort[7],SensorArea[5],SensorIP[16]; 
pthread_t pthread;
int clnt;
int arrcount=0;

void device_register();
void* setInterval();
int MakeConnection();
void *ReadParameters(void *filename);
void ReadConfig(char *filename);

int main(int argc, char *argv[])
{
	int i=0,j=0;
	int k=0;
	ReadConfig(argv[1]);
	clnt=MakeConnection();
	device_register(clnt);
	printf("\n");
	if(pthread_create(&pthread,NULL,ReadParameters,(void*)argv[2])!=0)
	{
		perror("pthread_create");
	}
	if(pthread_create(&pthread,NULL,setInterval,(void*)argv[2])!=0)
	{
		perror("pthread_create");
	}

	pthread_join(pthread,NULL);	
	pthread_detach(pthread);
	return 0;
}

//Function to connect to Gateway
int MakeConnection()
{
	int clnt;
	struct sockaddr_in sock;
	
	clnt=socket(AF_INET,SOCK_STREAM,0);
	if(clnt<0)
	{
		perror("create");
		exit(1);
	}

	sock.sin_addr.s_addr = inet_addr(GatewayIP);
	sock.sin_family = AF_INET;
	sock.sin_port = htons(atoi(GatewayPort));

	if(connect(clnt,(struct sockaddr*)&sock,sizeof(sock))<0)
	{	
		perror("connect");
		close(clnt);
		exit(0);
	}
	return clnt;
}

//Function to read config file
void ReadConfig(char *filename)
{
	FILE *config;
	config=fopen(filename,"r");

	fscanf(config,"%[^:]:%s\nsensor:%[^:]:%[^:]:%s",GatewayIP,GatewayPort,SensorIP,SensorPort,SensorArea);

	close(config);

}

//Function to send temperature to gateway at specific intervals
void *ReadParameters(void *filename)
{
	FILE *param;
	int curr_ts = 0, end_time = 0;
	int start_time, value;	
	char Message[100];
	char temp[10];
	int k;
	int message_size;
	char client_message[256];
	char *file=(char*)filename;
	
	param=fopen(file,"r");			
	
	while(1)
	{	
		if(curr_ts >= end_time)
		{
			fscanf(param,"%d;%d;%d\n",&start_time,&end_time,&value);
			if(feof(param))
			{
				fseek(param, 0, SEEK_SET);
				curr_ts = 0;
				end_time = 0;
			}
		}
		bzero(Message,100);
		sprintf(temp,"%d",value);
		sprintf(Message, "Type:currValue;Action:%s",temp);
		if(send(clnt,Message,strlen(Message),0) < 0)
		{
			perror("send");
			printf("\nUnable to send message");
		}
		printf("Message Sent: %s\n",Message);
		for(k=0;k<arrcount;k++)
			if(arr1[k]==clnt)
				break;
		sleep(arr2[k]);
		curr_ts += arr2[k];
		message_size=0;
	}
}


//Function to register device to the Gateway
void device_register(int clnt)
{

	char Message[MESSAGE_LENGTH];
	arr1[arrcount]=clnt;

	arr2[arrcount]=5;
	sprintf(Message,"Type:register;Action:sensor-%s-%s-%s",SensorIP,SensorPort,SensorArea);

	printf("Register Message to Gateway : %s\n",Message);
	if(send(clnt,Message,strlen(Message),0)<0)
	{
		perror("send");
		printf("\nUnable to send message");
	}
}

//Function to receive interval and set the particular sensor interval
void* setInterval()
{
	int message_size;
	char client_message[256];
	char setValue[10];
	int k=0;
	while(1)
	{	
		if((message_size = recv(clnt,client_message,sizeof(client_message),0))<0)
		{
			perror("read");
			exit(1);
		}
		sscanf(client_message,"Type:setInterval;Action:%s",setValue);
		printf("Message Received: Type:setInterval;Action:%s\n",setValue);
		for(k=0;k<arrcount;k++)
				if(arr1[k]==clnt)
					break;
		arr2[k]=atoi(setValue);
	}	

}