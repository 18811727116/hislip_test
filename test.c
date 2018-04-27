#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define MESSAGE_PROLOGUE 0x4853 //"HS"
#define MESSAGE_HEADER_SIZE 16
#define SERVER_PROTOCOL_VERSION 0x1000
#define SERVER_VENDOR_ID 0x575A //WZ(WelZek)

#define SEND_BUF_SIZE 1024
#define DATA_BUF_SIZE 0x100000//1Mbytes
#define HiSLIP_ PORT 4880
#define MAX_CLIENT 256

int fdbuf[MAX_CLIENT] = {0};

int main()
{
	int listenfd,connecfd;
	struct sockaddr_in serveraddr,clientaddr;
	int sin_size = sizeof(struct sockaddr_in);
	int number = 0;
	if((listenfd = socket(AF_INET,SOCK_STREAN,0)) == -1)
	{
		perror("create socket failed");
	        close(listenfd);
		exit(1);
	}

	int opt = SO_REUSEADDR;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));//reuse port

	bzero(&severaddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sinport = htons(HiSLIP_PORT);
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
}

void *pthread_service(void* sfd)
{
	int connectfd = *(int *)sfd;
}



int sync_initialize_response(hislip_message *send_message,uint16_t server_protocol_version,int sessionID)
{
	memset(send_message,0,sizeof(hislip_message));
	strncpy(send_message->prologue,"HS",2);
	send_message->message_type = InitializeResponse;
	send_message->control_code = 0x01;//overlap mode,0:synchonized mode
	send_message->message_parameter.s.UpperWord = htons(server_protocol_version);
	send_message->message_parameter.s.LowerWord = htons(sessionID);
	send_message->data_length = 0;
	//return sizeof(hislip_message);

}


int async_initialize_response(hislip_message *send_message,uint32_t server_vendorID)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = MESSAGE_PROLOGUE;
	send_message->message_type = AsyncInitializeResponse;
	send_message->control_code = 0x01;//overlap mode,0:synchonized mode
	send_message->message_parameter.s.value = htonl(server_vendorID);
	send_messgae->data_length = 0;
	//return sizeof(hislip_message);
}


int async_maximum_message_size_response(hislip_message *send_message,uint64_t max_size)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = MESSAGE_PROLOGUE;
	send_message->message_type = AsyncMaximumMessageResponse;
	send_message->control_code = 0;
	send_messge->message_parameter = 0;
	send_message->data_length = 0x08;
	uint64_t *p = ((char *)&send_message->prologue) + 16;//
	*p = max_size;

	
}


int async_lock_info_response(hislip_message *send_message,int value)//valuo=0:no exclusive lock granted;value=1:exclusive lock granted
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = MESSAGE_PROLOGUE;
	send_message->message_type = AsyncLockInfoResponse;
	send_message->control_code = value;
	send_messge->message_parameter = 0;
	send_message->data_length = 0;
}


int async_lock_response(hislip_message *send_message,uint8_t result)//result=0:failure;result=1:success;result=3:error
{                                                                   //request a lock or release a lock
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = MESSAGE_PROLOGUE;
	send_message->message_type = AsyncLockResponse;
	send_message->control_code = result;
	send_messge->message_parameter = 0;
	send_message->data_length = 0;
	//return sizeof(hislip_message);
}


int fatal_error(hislip_message *send_message,uint8_t error_code,char *message)
{
	memset(send_message,0,sizeof(hislip_message));
	//	strncpy(send_message->prologue,"HS",2);
	send_message->prologue = MESSAGE_PROLOGUE;
	send_message->message_type = FatalError;
	send_message->control_code = error_code;
	send_messge->message_parameter = 0;
	int len = 0;
	if(message)
	{
		len = strlen(message);
		memcpy(send_message,message,len);
		send_message->data_length = htobe64(len);
	}
	return sizeof(hislip_message)+len;
}


int info_communication(hislip_message *send_message,int RMT,uint32_t messageID,char *data,uint64_t len)//RMT=1:delivered;RMT=0:not delivered
{
	memset(send_message,0,sizeof(hislip_message));
	send_message->prologue = MESSAGE_PROLOGUE;
	send_message->message_type = DataEnd;
	send_message->control_code = RMT;
	send_message->message_parameter.messageID = messageID;
	memcpy(((char *)&send_message->prologue + 16),data,len);
	send_message->data_length = len;
}












typedef struct hislip_message
{
	//uint16_t prologue[2];//'H' and 'S',2 bytes
	uint16_t prologue;
	uint8_t message_type;//1 byte
	uint8_t control_code;//1 byte
	uint32_t message_paremeter;//4 bytes
	union
	{
		struct parameter
		{
			uint16_t UpperWord;//2 bytes
			uint16_t LowerWord;
		}s;
	uint32_t value;//4 bytes
	uint32_t timeout;
	uint32_t messageID;
	}message_parameter;
	uint64_t data_length;//8 bytes
	//uint64_t data[DATA_BUF_SIZE];
	

}hislip_message;

typedef enum //meiju,message type
{
    Initialize,//0
    InitializeResponse,//1
    FatalError,//2
    Error,//3
    AsyncLock,//4
    AsyncLockResponse,//5
    Data,//6
    DataEnd,//7
    DeviceClearComplete,//8
    DeviceClearAcknowledge,//9
    AsyncRemoteLocalControl,//10
    AsyncRemoteLocalResponse,//11
    Trigger,//12
    Interrupted,//13
    AsyncInterrupted,//14
    AsyncMaximumMessageSize,//15
    AsyncMaximumMessageSizeResponse,//16
    AsyncInitialize,//17
    AsyncInitializeResponse,//18
    AsyncDeviceClear,//19
    AsyncServiceRequest,//20
    AsyncStatusQuery,//21
    AsyncStatusResponse,//22
    AsyncDeviceClearAcknowledge,//23
    AsyncLockInfo,//24
    AsyncLockInfoResponse//25
} message_type;

typedef enum //meiju,fatal error code
{
    F_UnidentifiedError,//0
    PoorlyFormedMessageHeader,//1
    AttemptToUseConnectionWithoutBothChannelsEstablished,//2
    InvalidInitializationSequence,//3
    ServerRefusedConnectionDueToMaximumNumberOfClinetsExceeded//4
}fatal_error_code;

typedef enum //meiju, non-fatal error codes
{
    UnidentifiedError,//0
    UnrecognizedMessageType,//1
    UnrecognizedControlCode,//2
    UnreconnizedVendorDefinedMessage,//3
    MessageTooLarge//4
}error_code;

/*
//client program
int create_and_connect_server(struct sockaddr_in server_addr)
{

}


void sync_message_init(hislip_message *send_message,char *send_buffer,int *send_length)
{
	strncpy(send_message->prologue,"HS",2);
	send_message->message_type = Initialize;
	send_message->control_code = 0x00;
	send_message->data_length = 0x00;
	memset(send_message->data,0,DATA_BUF_SIZE);
	*send_length = sizeof(hislip_message);
	bzero(send_buffer,SEND_BUF_SIZE);
	memcpy(send_buffer,send_message,*send_length);
}


void async_message_init(hislip_message *send_message,char *send_buffer,int *send_length,unsigned char *sessionID)
{
	strncpy(send_message->prologue,"HS",2);
	send_message->message_type = AsyncInitialize;
	send_message->control_code = 0x00;
	send_message->message_paremeter = sessionID;
	send_message->data_length = 0x00;
	memset(send_message->data,0,DATA_BUF_SIZE);
	*send_length = sizeof(hislip_message);
	bzero(send_buffer,SEND_BUF_SIZE);
	memcpy(send_buffer,send_message,*send_length);

}


void async_message_maxsize_init(hislip_message *send_message,char *send_buffer,int *send_length)
{

}


void data_end_send(hislip_message *send_message,char *send_buffer,int *send_length,char *SCPI_commmand)
{


}*/
