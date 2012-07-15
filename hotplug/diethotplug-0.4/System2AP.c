
#include "System2AP.h"
//#include <Application/AppClass/System2AP.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define IPC_KEYPATH "/"
#define IPC_ID 59

#define SAFE_STRLEN(x) ((x == NULL)?0:strlen(x))

int RTK_SYS2AP_Get_Msgid(int *msgid)
{
    key_t ipckey;
    ipckey=ftok(IPC_KEYPATH,IPC_ID);
    if( (*msgid=msgget(ipckey,0))==-1)
        if( (*msgid=msgget(ipckey,IPC_CREAT|0660))==-1)
        {
            printf("msgget fail errno = %d , errstr=%s\n",errno,strerror(errno));
            return -1;
        }
    return 0;
}

int RTK_SYS2AP_SendMsg(SYS2AP_MESSAGE *Msg)
{
    int msgid;
    
    if(RTK_SYS2AP_Get_Msgid(&msgid)==-1)
        return -1;
    
    if (msgsnd(msgid,Msg,(SAFE_STRLEN(Msg->m_msg)+1),0)==-1)
    {
        printf("msgsnd fail errno = %d , errstr=%s\n",errno,strerror(errno));
        return -1;
    }
    return 0;
}

int RTK_SYS2AP_SendBinaryMsg(SYS2AP_MESSAGE *Msg,int m_msg_len)
{
    int msgid;
    
    if(RTK_SYS2AP_Get_Msgid(&msgid)==-1)
        return -1;
    
    if (msgsnd(msgid,Msg,m_msg_len,0)==-1)
    {
        printf("msgsnd fail errno = %d , errstr=%s\n",errno,strerror(errno));
        return -1;
    }
    return 0;
}

int RTK_SYS2AP_block_GetAllMsg(SYS2AP_MESSAGE *Msg,int *m_msg_len)
{
    int msgid;
    
    if(RTK_SYS2AP_Get_Msgid(&msgid)==-1)
        return -1;
        
    if((*m_msg_len=msgrcv(msgid,(void *)Msg,SYS2AP_MSG_SIZE,0,0))==-1)
    {
        printf("msgrcv fail errno = %d , errstr=%s\n",errno,strerror(errno));
        return -1;
    }
//    *m_msg_len -= sizeof(long int);
    return 0;
}
int RTK_SYS2AP_nonblock_GetMsg(SYS2AP_MESSAGE *Msg,long int m_type,int *m_msg_len)
{
    int msgid;
    
    if(RTK_SYS2AP_Get_Msgid(&msgid)==-1)
        return -1;
    if((*m_msg_len=msgrcv(msgid,(void *)Msg,SYS2AP_MSG_SIZE,m_type,IPC_NOWAIT))==-1)
    {
        return -1;
    }
//    *m_msg_len -= sizeof(long int);
    return 0;
}
int RTK_SYS2AP_PurgeMsg(void)
{
    SYS2AP_MESSAGE msg;
    int msgid;
    
    if(RTK_SYS2AP_Get_Msgid(&msgid)==-1)
        return -1;
        
    while(msgrcv(msgid,&msg,SYS2AP_MSG_SIZE,0,IPC_NOWAIT)!=-1);
    return 0;
}
