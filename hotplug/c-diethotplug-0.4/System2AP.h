#ifndef __USR_LIB_SYS2AP_H__
#define __USR_LIB_SYS2AP_H__

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SYS2AP_MSG_SIZE 1023
typedef struct 
{
    long int m_type;
    char m_msg[SYS2AP_MSG_SIZE+1];
} SYS2AP_MESSAGE;
/*
Define System 2 AP protocol

1. message type : see enum ENUM_SYS2AP_MESSAGE_TYPE
2. message content: seperated by space
            [Device Name] [Message 1] [Message 2] [Message 3] ...
            
[Device Name] : in message type the sub item 
    example : type is WLAN and sub item may : wlan0,wlan1,relian0....
[Message N] : message define by message type self

example 1:
    message type : SYS2AP_MSG_WLAN
    [Device Name] : wlan0
    [Message 1] : UP
conclusion message content: "wlan0 UP"
    message type : SYS2AP_MSG_ETH
    [Device Name] : eth0
    [Message 1] : UP
conclusion message content: "eth0 UP"

example 2:
    message type : SYS2AP_MSG_USB
    [Device Name] : flash_disk
    [Message 1] : 3                 : following message number (variable message number)
    [Message 2] : /dev/sda1         : raw disk position
    [Message 3] : ADD               : plug in to system
conclusion message content: "flash_disk 3 /dev/sda1 ADD"

example 3:
    message type : SYS2AP_MSG_USB
    [Device Name] : flash_disk
    [Message 1] : 4                 : following message number (variable message number)
    [Message 2] : /dev/sdb1         : raw disk position
    [Message 3] : /dev/sdb2         : raw disk position
    [Message 4] : ADD               : plug in to system
conclusion message content: "flash_disk 4 /dev/sdb1 /dev/sdb2 ADD"

example 4:
    message type : SYS2AP_MSG_USBCDROM
    [Device Name] : sr0
    [Message 1] : UP
    [Message 2] :  /dev/scsi/host4/bus0/target0/lun0/cd
conclusion message content: "sr0 UP /dev/scsi/host4/bus0/target0/lun0/cd"
    message type : SYS2AP_MSG_USBCDROM
    [Device Name] : sr0
    [Message 1] : DOWN
conclusion message content: "sr0 DOWN"
    
*/
// Message type
typedef enum sys2ap_message_type {
    SYS2AP_MSG_NULL=0, 
    SYS2AP_MSG_WLAN=1,
    SYS2AP_MSG_ETH=2,
    SYS2AP_MSG_USBCDROM=3,
    SYS2AP_MSG_NUM=4,
} ENUM_SYS2AP_MESSAGE_TYPE;

// Message event
#define SYS2AP_MSG_UP      "UP"
#define SYS2AP_MSG_DOWN    "DOWN"

#ifdef __cplusplus    /*"__cplusplus" is #defined if/only-if compiler is C++*/
extern "C" {
#endif

    // for kernel use
    int RTK_SYS2AP_SendMsg(SYS2AP_MESSAGE *Msg);
    int RTK_SYS2AP_SendBinaryMsg(SYS2AP_MESSAGE *Msg,int m_msg_len);
    // for AP use
    int RTK_SYS2AP_block_GetAllMsg(SYS2AP_MESSAGE *Msg,int *m_msg_len);
    int RTK_SYS2AP_nonblock_GetMsg(SYS2AP_MESSAGE *Msg,long int m_type,int *m_msg_len);
    int RTK_SYS2AP_PurgeMsg(void);
#ifdef __cplusplus    /*"__cplusplus" is #defined if/only-if compiler is C++*/
}
#endif
#endif



