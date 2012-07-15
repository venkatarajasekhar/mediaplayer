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
    message type : SYS2AP_MSG_NETPLUG_STATUS
    [Device Name] : wlan0
    [Message 1] : UP/DOWN
conclusion message content: "wlan0 UP"

example 2:
    message type : SYS2AP_MSG_NETLINK_STATUS
    [Device Name] : eth0
    [Message 1] : UP/DOWN/PBC
conclusion message content: "eth0 UP"
    message type : SYS2AP_MSG_NETPLUG_STATUS
    [Device Name] : wlan0
    [Message 1] : PBC 
conclusion message content: "wlan0 PBC"  --> for WPS PBC ability

example 3:
    message type : SYS2AP_MSG_USBCDROM
    [Device Name] : sr0
    [Message 1] : UP
    [Message 2] :  /dev/scsi/host4/bus0/target0/lun0/cd
conclusion message content: "sr0 UP /dev/scsi/host4/bus0/target0/lun0/cd"
    message type : SYS2AP_MSG_USBCDROM
    [Device Name] : sr0
    [Message 1] : DOWN
conclusion message content: "sr0 DOWN"

example 4:
    message type : SYS2AP_MSG_BLOCK
    [Device Name] : sda1
    [Message 1] : UP			: the device is mounted
    [Message 2] : /mnt/usbmounts/sda1	: mount point
    [Message 3] : /tmp/ramfs/volumes/C: : link point
conclusion message content: "sda1 UP /mnt/usbmounts/sda1 /tmp/ramfs/volumes/C:"
    message type : SYS2AP_MSG_BLOCK
    [Device Name] : sda1
    [Message 1] : DOWN		: the device is umounted
conclusion message content: "sda1 DOWN"
    message type : SYS2AP_MSG_BLOCK
    [Device Name] : sda1
    [Message 1] : READY		: the device is all mounted
    [Message 2] : host0		: host number ( "ptp" if it is ptp device)
    [Message 3] : SATA/USB	: bus type
    [Message 4] : 1		: port number(start from 1, none if it is a ptp device)
conclusion message content: "sda1 READY host0 SATA 1"
conclusion message content: "1-1:1.0 READY ptp USB"
    message type : SYS2AP_MSG_BLOCK
    [Device Name] : sda1
    [Message 1] : CLEAR		: the device is all umounted
    [Message 2] : host0		: host number ( "ptp" if it is ptp device)
    [Message 3] : SATA/USB	: bus type
    [Message 4] : 1		: port number(start from 1, none if it is a ptp device)
conclusion message content: "sda1 CLEAR host2 USB 2.1"
conclusion message content: "1-1:1.0 CLEAR ptp USB"

example 5:
    message type : SYS2AP_MSG_SATAPLUG_STATUS
    [Device Name] : host0
    [Message 1] : UP			: plug in to system
    [Message 2] : 1			: port number(start from 1)
conclusion message content: "host0 UP 1"
    message type : SYS2AP_MSG_SATAPLUG_STATUS
    [Device Name] : host0
    [Message 1] : DOWN		: plug out from system
conclusion message content: "host0 DOWN"

example 6:
    message type : SYS2AP_MSG_USBPLUG_STATUS
    [locate] : 2.1			: port number(start from 1)
    [Message 1] : UP			: plug in to system
conclusion message content: "2.1 UP"
    message type : SYS2AP_MSG_USBPLUG_STATUS
    [locate] : 2.1			: port number(start from 1)
    [Message 1] : DOWN		: plug out from system
conclusion message content: "2.1 DOWN"

example 7:
    message type : SYS2AP_MSG_BLOCK_SATA_NO_MOUNT
    [Device Name] : sda1
    [Message 1] : /dev/scsi/host0/bus0/target0/lun0/part1	: raw path to mount
conclusion message content: "sda1 /dev/scsi/host0/bus0/target0/lun0/part1"

example 8:
    message type : SYS2AP_MSG_BLOCK_SATA_SIGNATURE
    [Device Name] : sda
    [Message 1] : /dev/scsi/host0/bus0/target0/lun0/disc	: raw path to mount
conclusion message content: "sda /dev/scsi/host0/bus0/target0/lun0/disc"

example 9:
    message type : SYS2AP_MSG_AUDIOPLUG_STATUS
    [locate] : 2.1			: port number(start from 1)
    [Message 1] : UP			: plug in to system
    [Message 2] : IN			: audio direction
conclusion message content: "2.1 UP IN"
    message type : SYS2AP_MSG_AUDIOPLUG_STATUS
    [locate] : 2.1			: port number(start from 1)
    [Message 1] : DOWN			: plug out from system
    [Message 2] : OUT			: audio direction
conclusion message content: "2.1 DOWN OUT"

example 9:
    message type : SYS2AP_MSG_VIDEOPLUG_STATUS
    [locate] : 2.1			: port number(start from 1)
    [Message 1] : UP			: plug in to system
    [Message 2] : IN			: video direction
conclusion message content: "2.1 UP IN"
    message type : SYS2AP_MSG_VIDEOPLUG_STATUS
    [locate] : 2.1			: port number(start from 1)
    [Message 1] : DOWN			: plug out from system
    [Message 2] : IN			: vidoe direction
conclusion message content: "2.1 DOWN OUT"

*/
// Message type
typedef enum sys2ap_message_type {
    SYS2AP_MSG_NULL=0, 
    SYS2AP_MSG_NETPLUG_STATUS,
    SYS2AP_MSG_NETLINK_STATUS,
    SYS2AP_MSG_USBCDROM,
    SYS2AP_MSG_BLOCK,
    SYS2AP_MSG_SATAPLUG_STATUS,
    SYS2AP_MSG_USBPLUG_STATUS,
    SYS2AP_MSG_BLOCK_SATA_NO_MOUNT,
    SYS2AP_MSG_BLOCK_SATA_SIGNATURE,
    SYS2AP_MSG_AUDIOPLUG_STATUS,
    SYS2AP_MSG_VIDEOPLUG_STATUS,
    SYS2AP_MSG_NUM,
} ENUM_SYS2AP_MESSAGE_TYPE;

// Message event
#define SYS2AP_MSG_UP      "UP"
#define SYS2AP_MSG_DOWN    "DOWN"
#define SYS2AP_MSG_READY   "READY"
#define SYS2AP_MSG_CLEAR   "CLEAR"
#define SYS2AP_MSG_SATA    "SATA"
#define SYS2AP_MSG_USB     "USB"
#define SYS2AP_MSG_1       "1"
#define SYS2AP_MSG_2       "2"
#define SYS2AP_MSG_PBC    "PBC"   // for user push WPS button on wifi dongle

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



