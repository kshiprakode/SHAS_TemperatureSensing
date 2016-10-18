#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_CONNECTION 20
#define MESSAGE_LENGTH 256

struct sensor_device{
	int id;
	char IP[16];
	char Port[7];
	char Area[3];
	bool isSensor;
	int sockid;
	int lastValue;
};

typedef struct {
	struct sockaddr address;
	int addr_len;
	int sockid;
}connection_ds;

bool sig = false;
struct sensor_device connectionList[MAX_CONNECTION];
connection_ds *ActiveClient = NULL;
int connectionCount = 0; 
char GatewayPort[6];
char GatewayIP[20];
FILE *output;
pthread_t thread, thread_setValue;
int sockfd;
FILE *devStatus;

void *connection(void *);
void *setValue();
void  INThandler(int sig);
void ReadConfig(char *filename);
void MakeConnection();

int main(int argc, char *argv[])
{

	int clnt;
	signal(SIGINT, INThandler); 
	signal(SIGTSTP, INThandler); 
	ReadConfig(argv[1]);
	MakeConnection();
	getchar();
	pthread_detach(thread);
	pthread_detach(thread_setValue);
	return 0;
}

void * connection(void *clnt)
{
	int client = *(int*)clnt;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	int message_size;
	char message[MESSAGE_LENGTH];
	char status[4];
	char message_type[10];
	char action[100];
	char type_temp[10];
	int value,i,j;
	char fmessage[100];
	char temp_area[3];
	
	if(output==NULL)
	{
		printf("Error opening file\n");
	}

	pthread_mutex_lock(&mutex);
	fprintf(output,"-------------------------------------------------------\n");
	fprintf(output,"-------------------------------------------------------\n");
	pthread_mutex_unlock(&mutex);

	while(1)
	{
		
		if(sig == true)
			break;
		//Read incoming messages to Gateway
		bzero(message,MESSAGE_LENGTH);
		if((message_size = read(client,message,256))<0)
		{
			perror("recv");
			return 0;
		}

		if(message_size != 0)
		{
			memset(action,0,100);
			sscanf(message, "Type:%[^;];Action:%s", message_type,action);
			//If the incoming message is the current value from Sensor
			if(strcmp("currValue", message_type) == 0)	//to check type of a msg 
			{
			
				value = atoi(action);
				if(value < 32 || value > 34)
				{
					//turn on heater
					for(i=0;i<connectionCount;i++)
					{
						if(connectionList[i].sockid == client)
						{
							break;
						}
					}
					connectionList[i].lastValue=value;
					strcpy(temp_area,connectionList[i].Area);
					
					for(i=0; i < connectionCount;i++)
					{
						if(strcmp(connectionList[i].Area, temp_area) == 0)
						{
							if(connectionList[i].isSensor == false)
							{
								break;
							}
						}	
					}
					if(i != connectionCount)
					{
						if(value < 32) //1st sensor's value is less than 32 so the heater should be on
						{
							write(connectionList[i].sockid, "Type:Switch;Action:on", sizeof("Type:Switch;Action:on")); 
							strcpy(status,"ON");
						}
						else //sinc 1st sensor's value is less than 32, we are checking the value of the next sensor
						{
							for(j=0;j<connectionCount;j++)
							{
								if(((strcmp(connectionList[j].Area,connectionList[i].Area))==0) && connectionList[j].isSensor==true && connectionList[j].sockid!=client)
								{										
									if(connectionList[j].lastValue<32)
									{
										write(connectionList[i].sockid, "Type:Switch;Action:on", sizeof("Type:Switch;Action:on")); 
										strcpy(status,"ON");	
										break;	
									}	
								}
								else
								{
									strcpy(status,"OFF");
								 	write(connectionList[i].sockid, "Type:Switch;Action:off", sizeof("Type:Switch;Action:off"));
								}
							}															
						}
						
						for(j=0;j<connectionCount;j++){
							if(((strcmp(connectionList[j].Area,connectionList[i].Area))==0) && connectionList[j].isSensor==true && connectionList[j].sockid==client)
								break;
						}
						pthread_mutex_lock(&mutex);
						fprintf(output,"  %d ---- %s:%s ---- device ---- %s ---- %s\n",connectionList[i].id,connectionList[i].IP,connectionList[i].Port,connectionList[i].Area,status);
						pthread_mutex_unlock(&mutex);
						
						pthread_mutex_lock(&mutex);
						fprintf(output,"  %d ---- %s:%s ---- sensor ---- %s ---- %d\n",connectionList[j].id,connectionList[j].IP,connectionList[j].Port,connectionList[j].Area,value);
						pthread_mutex_unlock(&mutex);

						for(j=0;j<connectionCount;j++)
						{
							if(((strcmp(connectionList[j].Area,connectionList[i].Area))==0) && connectionList[j].isSensor==true && connectionList[j].sockid!=client)
							{										
								pthread_mutex_lock(&mutex);
								fprintf(output,"  %d ---- %s:%s ---- sensor ---- %s ---- %d\n",connectionList[j].id,connectionList[j].IP,connectionList[j].Port,connectionList[j].Area,connectionList[j].lastValue);
								pthread_mutex_unlock(&mutex);
							}	
						}
						pthread_mutex_lock(&mutex);
						fprintf(output,"-------------------------------------------------------\n");
						fprintf(output,"-------------------------------------------------------\n");
						pthread_mutex_unlock(&mutex);

						bzero(message,MESSAGE_LENGTH);				
					}
					else
					{
					}
				}

			}
			//If the message received is CurrState from the smart device
			else if(strcmp("currState", message_type) == 0)
			{
				char msg_content[20];
				int msg_size;
				printf("Currstate Gateway received message!");
			}
			//If the message received is register from the smart device and sensor
			else if(strcmp("register", message_type) == 0)
			{
				sscanf(action, "%[^-]-%[^-]-%[^-]-%s", type_temp, connectionList[connectionCount].IP, connectionList[connectionCount].Port, connectionList[connectionCount].Area);
				connectionList[connectionCount].sockid = client;
				connectionList[connectionCount].id = connectionCount+1;;
				if(strcmp(type_temp, "sensor") == 0)
				{
					connectionList[connectionCount].isSensor = true;
					connectionCount++;
				}
				else
				{
					connectionList[connectionCount].isSensor = false; 
					connectionCount++;
					break;
				}
			}
		}
		else
		{
			break;
		}
	}
	
	return 0;
}

//Function to send message to Sensor to set the interval
void *setValue()
{
	char Message_SetValue[100];
	int setVal = 0,j=0;
	int i,sen_num;
	int c;
	while(1)
	{

	printf("Menu\n1. Set Interval Sensor\n2. Exit\n");
		scanf("%d",&c);
		if(c==1)
		{	j=0;
			for(i=0;i<connectionCount;i++)
			{	
				if(connectionList[i].isSensor==true)
				{
					j++;
					printf("Sensor ID - %d.) %s : %s : %s \n",connectionList[i].id,connectionList[i].IP,connectionList[i].Port,connectionList[i].Area);
				}
			}
			printf("Enter the Sensor ID in the list to change interval : ");
			scanf("%d",&sen_num);
			while(sen_num>connectionCount)
			{
				printf("Enter correct Sensor ID : ");
				scanf("%d",&sen_num);
			}
				
			printf("Enter the interval to set : ");
			scanf("%d",&setVal);
	
			sprintf(Message_SetValue,"Type:setInterval;Action:%d",setVal);
			for(j=0;j<connectionCount;j++)
			{	
				if(sen_num==connectionList[j].id)
					break;
			}
			write(connectionList[j].sockid, Message_SetValue, sizeof(Message_SetValue)); 				
			printf("Interval has been changed for sensor with IP %s!\n",connectionList[j].IP);
		
		}
		else if (c  == 2)
		{
			pthread_detach(thread);
			pthread_detach(thread_setValue);
			kill(getpid(),SIGKILL);
		}
	}	

}

//Function to handle Cntrl + C and Cntrl + z
void  INThandler(int sig)
{
	sig = true;
    signal(sig, SIG_IGN);
	pthread_detach(thread);
	pthread_detach(thread_setValue);
	kill(getpid(),SIGKILL);

}

//Function to read Gateway Config File
void ReadConfig(char *filename)
{
	FILE *config;
	config=fopen(filename,"r");
	fscanf(config,"%[^:]:%s\n",GatewayIP,GatewayPort);
	fclose(config);					
}


//Function to accept incoming messages and spawn a thread
void MakeConnection()
{
	struct sockaddr_in server,client;
	//initialize Socket
	int userThread = 0;
	int yes=1,c,clnt;
	int temp_sockfd;
	int GPort;

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	//Socket Creation

	if(sockfd < 0)
	{
		perror("socket");
		exit(1);
	}	

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(GatewayPort));	

	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))==-1)
	{
		perror("setsockopt");
		close(sockfd);
		exit(1);
	}

	//Bind the name to the socket
	if(bind(sockfd,(struct sockaddr*)&server,sizeof(server))<0)
	{
		perror("bind");
		close(sockfd);
		exit(1);
	}
	
	//Listen at the particular socket for connection requests
	listen(sockfd,3);
	output=fopen("output.txt","w+");

	ActiveClient = (connection_ds*) malloc (sizeof(connection));
	
	//Accept incoming messages and spawn a thread for each incoming connection at the socket

	while(ActiveClient->sockid = accept(sockfd, &ActiveClient->address, &ActiveClient->addr_len))
	{
		temp_sockfd = ActiveClient->sockid;
		if((pthread_create(&thread,NULL,connection,(void*)&temp_sockfd))!=0)
		{
			perror("pthread_create");
		}

		if(userThread==0)
		{

			if((pthread_create(&thread_setValue,NULL,setValue,NULL)!=0))
			{
				perror("pthread_create");
			}

			userThread=1;	
		}
	}

	if(clnt<0)
	{
		perror("accept");
		close(sockfd);
		exit(1);
	}

	fclose(output);
	close(sockfd);
				
}
