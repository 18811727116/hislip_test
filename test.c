#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include "test.h"

#define MESSAGE_PROLOGUE 0x4853 //"HS"
#define MESSAGE_HEADER_SIZE 16
#define SERVER_PROTOCOL_VERSION 0x1000
#define SERVER_VENDOR_ID 0x575A //WZ(WelZek)

#define MAX_SESSIONS 256

#define DATA_BUF_SIZE 0x100000//1Mbytes
#define HiSLIP_PORT 4880
#define MAX_CLIENT 256

uint16_t session_id = 100;
int fdbuf[MAX_CLIENT] = {0};



void *pthread_service(void* sfd)
{
	int connectfd = *(int *)sfd;
	int receive_bytes,send_bytes;

	hislip_message header;
	//char *subaddress;
	void *payload = NULL;

	while(1)
	{
		
		if((receive_bytes = tcp_server_read(connectfd,&header,MESSAGE_HEADER_SIZE,0)) == 0)
		{

			printf("receive 0 byte\n");
		//	close(connectfd);

		}

		if(receive_bytes < MESSAGE_HEADER_SIZE)
		
			continue;


		if(header.prologue != htons(MESSAGE_PROLOGUE))//verify the message header
		{
		
			printf("invalid header\n");
			fatal_error(&header,PoorlyFormedMessageHeader,NULL);
			continue;
		}

		/*if(header.data_length > 0)
		{
			if(header.data_length > server_config->payload_size_max)
			{
				printf("maximum payload size exceeded\n");
				continue;
			}

			payload = malloc(header.data_length);

			if(payload == NULL)
			{
				printf("malloc failed\n");
				continue;
			}

			if((receive_bytes = tcp_read(connectfd,payload,header.data_length,0)) == 0)
			{
				printf("client closed connection\n");
				close(connectfd);
			}
		}*/
		switch(header.message_type)
		{
			case Initialize:
				{
					//header.message_parameter.s.LowerWord = session_id;
			                sync_initialize_response(&header,SERVER_PROTOCOL_VERSION,session_id);
		  			tcp_server_write(connectfd,&header,MESSAGE_HEADER_SIZE,0);
					break;
				}
			case AsyncInitialize:
				{
				       //	uint16_t sessionID = ntohs(header.parameter.a.LowerWord);
					async_initialize_response(&header,SERVER_VENDOR_ID);
					tcp_server_write(connectfd,&header,MESSAGE_HEADER_SIZE,0);
				        break;
				}
			case AsyncMaximumMessageSize:
				{
					hislip_message *p = (hislip_message *)malloc(MESSAGE_HEADER_SIZE+8);
					async_maximum_message_size_response(p,0x100000);//1Mbytes
					tcp_server_write(connectfd,p,MESSAGE_HEADER_SIZE+8,0);
					free(p);
					break;
				}
			case AsyncLock:
				{
					async_lock_response(&header,1);//result = 1:success
					tcp_server_write(connectfd,&header,MESSAGE_HEADER_SIZE,0);
					break;	
				}
			case DataEnd:	
				{
					uint32_t MessageID = 0xffffff00;
					char buf[1000];
					strcpy(buf,"LitePoint,IQxstream-M,IQ020FA0098,1.7.0\n");
					int len = sizeof(buf);
					info_communication(&header,MessageID,buf,len);
					tcp_server_write(connectfd,&header,MESSAGE_HEADER_SIZE+len,0);
				}
			default:	break;
		}
	}
}



int tcp_server_read(int fd,void *buffer,int length,int timeout)
{
	int res;
	struct timeval time;
	time.tv_sec = 0;
	time.tv_usec = timeout*1000;

	fd_set rdfs;
	FD_ZERO(&rdfs);
	FD_SET(fd,&rdfs);

	if(timeout)
	
		res = select(fd+1,&rdfs,NULL,NULL,&time);
	
	else
		res = select(fd+1,&rdfs,NULL,NULL,NULL);

	if(res == -1)

		return -1;

	else if(res == 0)

		printf("timeout\n");

	else

		res = read(fd,buffer,length);//return bytes
		return res;
}


int tcp_server_write(int fd,void *buffer,int length,int timeout)
{
	int res;
	struct timeval time;
	time.tv_sec = 0;
	time.tv_usec = timeout*1000;

	fd_set wdfs;
	FD_ZERO(&wdfs);
	FD_SET(fd,&wdfs);

	if(timeout)
	
		res = select(fd+1,NULL,&wdfs,NULL,&time);

	else

		res = select(fd+1,NULL,&wdfs,NULL,NULL);

	if(res == -1)	
		
		return -1;

	else if(res == 0)

		printf("timeout\n");

	else

		return write(fd,buffer,length);
	
}




int sync_initialize_response(hislip_message *send_message,uint16_t server_protocol_version,int sessionID)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = InitializeResponse;
	send_message->control_code = 0x01;//overlap mode,0:synchonized mode
	send_message->message_parameter.s.UpperWord = htons(server_protocol_version);
	send_message->message_parameter.s.LowerWord = htons(sessionID);
	send_message->data_length = 0;
	return;

}


int async_initialize_response(hislip_message *send_message,uint32_t server_vendorID)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = AsyncInitializeResponse;
	send_message->control_code = 0x01;//overlap mode,0:synchonized mode
	send_message->message_parameter.value = htons(server_vendorID);
	send_message->data_length = 0;
	return;
}


int async_maximum_message_size_response(hislip_message *send_message,uint64_t max_size)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = AsyncMaximumMessageSizeResponse;
	send_message->control_code = 0;
	send_message->message_parameter = 0;
	send_message->data_length = 0x08;
	uint64_t *p = ((char *)&send_message->prologue) + 16;//
	*p = max_size;
	return;
	
}


int async_lock_info_response(hislip_message *send_message,int value)//valuo=0:no exclusive lock granted;value=1:exclusive lock granted
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = AsyncLockInfoResponse;
	send_message->control_code = value;
	send_message->message_parameter = 0;
	send_message->data_length = 0;	
	return;
}


int async_lock_response(hislip_message *send_message,uint8_t result)//result=0:failure;result=1:success;result=3:error
{                                                                   //request a lock or release a lock
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = AsyncLockResponse;
	send_message->control_code = result;
	send_message->message_parameter = 0;
	send_message->data_length = 0;
	return;
}


int fatal_error(hislip_message *send_message,uint8_t error_code,char *message)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = FatalError;
	send_message->control_code = error_code;
	send_message->message_parameter = 0;
	int len = 0;
	if(message)
	{
		len = strlen(message);
		memcpy(send_message,message,len);
		send_message->data_length = htobe64(len);
	}
	return;
}


int info_communication(hislip_message *send_message,uint32_t messageID,char data[],uint64_t len)//RMT=1:delivered;RMT=0:not delivered
{
	memset(send_message,0,sizeof(hislip_message));
	send_message->prologue = htons(MESSAGE_PROLOGUE);
	send_message->message_type = Data;
	send_message->control_code = 0;
	send_message->message_parameter.messageID = messageID;
	messageID = messageID + 2;
	memcpy(((char *)&send_message->prologue + 16),data,len);
	send_message->data_length = len;

	send_message->prologue = htons(MESSAGE_PROLOGUE);
        send_message->message_type = DataEnd;
        send_message->control_code = 0;
	send_message->message_parameter.messageID = messageID;
	messageID = messageID + 2;
	memcpy(((char *)&send_message->prologue + 16),data,len);
        send_message->data_length = len;
	return;

}



int main()
{
	int listenfd,connectfd;
	struct sockaddr_in serveraddr,clientaddr;
	int sin_size = sizeof(struct sockaddr_in);
	int number = 0;
	if((listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("create socket failed");
	        close(listenfd);
		exit(1);
	}

	int opt = SO_REUSEADDR;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));//reuse port

	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(HiSLIP_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) == -1)
	{
		perror("bind failed");
	        close(listenfd);
		exit(1);
	}

	if((listen(listenfd,256)) == -1)
	{
		perror("listen failed");
		close(listenfd);
		exit(1);
	}

	printf("waitting for client connecting\n");

	while(1)
	{
	
		connectfd = accept(listenfd,(struct sockaddr *)&clientaddr,&sin_size);
		if(connectfd == -1)
		{
			perror("accept failed");
			close(listenfd);
			exit(1);
		}
		printf("client connect on server");

		if(number >= MAX_CLIENT)
		{
			printf("no more client is allowed\n");
			close(connectfd);
		}
		int i;
		for(i=0;i<MAX_CLIENT;i++)
		{
			if(fdbuf[i] == 0)
			{
				fdbuf[i] = connectfd;
				break;
			}
		}
		pthread_t thread;
		pthread_create(&thread,NULL,(void*)pthread_service,&connectfd);
		number++;
		pthread_detach(thread);
	}
	close(listenfd);
	return;
}










