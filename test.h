#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>




typedef struct hislip_message
{
        //uint16_t prologue[2];//'H' and 'S',2 bytes
        uint16_t prologue;
        uint8_t message_type;//1 byte
        uint8_t control_code;//1 byte
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


typedef struct
{
        int port;
        int connections_max;
        int work_thread_max;
        int work_wueue_depth_max;
        uint64_t payload_size_max;// = 0x400000;//4Mbytes
        int message_timeout;
}server_config;


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

void *pthread_service(void* sfd);
int tcp_server_read(int fd,void *buffer,int length,int timeout);
int tcp_server_write(int fd,void *buffer,int length,int timeout);
int sync_initialize_response(hislip_message *send_message,uint16_t server_protocol_version,int sessionID);
int async_initialize_response(hislip_message *send_message,uint32_t server_vendorID);
int async_maximum_message_size_response(hislip_message *send_message,uint64_t max_size);
int async_lock_info_response(hislip_message *send_message,int value);
int async_lock_response(hislip_message *send_message,uint8_t result);
int fatal_error(hislip_message *send_message,uint8_t error_code,char *message);
int info_communication(hislip_message *send_message,uint32_t messageID,char data[],uint64_t len);

#endif

