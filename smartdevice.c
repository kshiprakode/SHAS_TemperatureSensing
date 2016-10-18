#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include<stdbool.h>

#define MESSAGE_LENGTH 256

char GatewayPort[7];
char GatewayIP[30],SmartDevice[50],SmartDevicePort[7],SmartDeviceArea[5],SmartDeviceIP[16]; 
char action[4]="off";
char status[4]="off";

void device_register();
void* switch_action(void*);
void* sendCurrState(void*);
int MakeConnection();
void ReadConfig(char*);

int main(int argc, char *argv[])
{
	int i=0,j=0;
	int clnt;
	pthread_t pthread_send_state,pthread_switch_action;
	
	printf("\n");

	//Read Configuration file to get sensor and gateway details
	ReadConfig(argv[1]);

	//Make socket connections to gateway
	clnt=MakeConnection();

	//Register message with gateway using the message type register
	device_register(clnt);
	
	//create thread for receiving switch messages from gateway
	if(pthread_create(&pthread_switch_action,NULL,switch_action,(void*)&clnt)!=0)
	{
		perror("pthread_create");
	}

	//create thread for sending current state to gateway
	if(pthread_create(&pthread_send_state,NULL,sendCurrState,(void*)&clnt)!=0)
	{
		perror("pthread_create");
	}
	
	pthread_join(pthread_send_state,NULL);
	pthread_join(pthread_switch_action,NULL);

	pthread_detach(pthread_switch_action);
	pthread_detach(pthread_send_state);
	return 0;
}


//Function to read the configuration file of the smart device
void ReadConfig(char *filename)
{
	FILE *config;
	config=fopen(filename,"r");

	fscanf(config,"%[^:]:%s\ndevice:%[^:]:%[^:]:%s",GatewayIP,GatewayPort,SmartDeviceIP,SmartDevicePort,SmartDeviceArea);

	close(config);
}

//Function to connect to the socket server
int MakeConnection()
{
	int clnt;
	struct sockaddr_in sock;
	
	//create socket
	clnt = socket(AF_INET,SOCK_STREAM,0);
	if(clnt < 0)
	{
		perror("create");
		exit(1);
	}

	sock.sin_addr.s_addr = inet_addr(GatewayIP);
	sock.sin_family = AF_INET;
	sock.sin_port = htons(atoi(GatewayPort));

	//connect to the socket address
	if(connect(clnt,(struct sockaddr*)&sock,sizeof(sock))<0)
	{	
		perror("connect");
		close(clnt);
		exit(0);
	}
	return clnt;
}


//Function to switch the smart device status
void* switch_action(void* cl)
{
	int clnt;
	char message[MESSAGE_LENGTH];
	char message_type[10];
	int message_size;
	int send_msg;
	char msg[20];
	char parameter[20];

	clnt=*(int*)cl;
	while(1)
	{	
		bzero(message,MESSAGE_LENGTH);
		if((message_size=read(clnt,message,MESSAGE_LENGTH)) < 0)
		{
			perror("recv");
			exit(1);
		}
		if(message_size != 0)
		{
			sscanf(message, "Type:%[^;];Action:%s", message_type,action);	
		}
		printf("Message Received : %s\n",message);	
	}
}

//Function to send register message
void device_register(int clnt)
{

	char Message[MESSAGE_LENGTH];

	sprintf(Message,"Type:register;Action:device-%s-%s-%s",SmartDeviceIP,SmartDevicePort,SmartDeviceArea);

	printf("Register Message to Gateway : %s\n",Message);
	if(send(clnt,Message,strlen(Message),0)<0)
	{
		perror("send");
		printf("Unable to send message\n");
	}
	strcpy(status,"on");
}

//Function to send current state to socket server, Gateway
void *sendCurrState(void* cl)
{
	int message_size;
	int clnt=*(int*)cl;
	char client_message[256];
	while(1)
	{
		strcpy(status,action);
		sprintf(client_message,"Type:currState;Action:%s",status);
		if(write(clnt,client_message,strlen(client_message)) < 0 )
		{
			perror("send");
			printf("\nUnable to send message");
		}
		printf("Message Sent : %s\n", client_message);
		sleep(20);
	}
}
