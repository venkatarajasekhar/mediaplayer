//#include "efence.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Application/AppClass/rtk_ini.h>
#include <Utility/samba/samba.h> // samba class

#define STR_NULL ""
#define FREE(p) {if(p){free((void *)p); p=NULL;}}
//#define jsprintf(format,arg...)  printf( format,## arg)
#define jsprintf(format,arg...)  

int main(int argc,char *argv[])
{
    bool bCreateOutput=true;
    int command;
    Rtk_Ini *infile=new Rtk_Ini();
    Rtk_Ini *outfile=new Rtk_Ini();
    if(argc != 3)
    {
        printf("Usage : execute infile outfile\n");
        return -1;
    }
    infile->loadFile((char *)argv[1]);
    infile->getIntValue(MAIN,COMMAND,&command);
    switch(command)
    {
    case 1: // Query Name
        {
            char *IniValue;
            struct in_addr ip;
            IniValue=infile->getStrValue(MAIN,P1);
            if(IniValue)
            {
                char *IniRealValue=NULL;
                char *NetBiosName=NULL;
                if(!infile->removeStrMark_needfree(IniValue,&IniRealValue))
                    NetBiosName=IniRealValue;
                else
                    NetBiosName=IniValue;
                jsprintf(" NetBiosName = %s , %s , %s\n",IniValue,IniRealValue,NetBiosName);
                if(!Samba_NetBiosName2Ip(NetBiosName,&ip))
                {
                    outfile->setIntValue(MAIN, NUMBER,1);
                    outfile->setU64IntValue(MAIN,"1",ip.s_addr);
                }
                else
                    outfile->setIntValue(MAIN, NUMBER,-1);
                if(IniRealValue)
                    FREE(IniRealValue);
            }
            else
                outfile->setIntValue(MAIN, NUMBER,0);
        }
        break;
    case 2: // Query Domain
        {
            long Handle;
            int num=0;
            int i;
            if(Samba_QueryDomainList(&Handle,&num))
                outfile->setIntValue(MAIN, NUMBER,-1);
            else
            {
                char StrNum[32];
                outfile->setIntValue(MAIN, NUMBER,num);
                for (i=0;i<num;i++)
                {
                    char *cTemp=NULL;
                    char *cOutput=NULL;
                    sprintf(StrNum,"%d",i);
                    if(!Samba_GetDomainList(Handle,i,&cTemp))
                    {
                        if(!outfile->addStrMark_needfree(cTemp,&cOutput))
                        {
                            outfile->setStrValue(MAIN,StrNum,cOutput);
                            FREE(cOutput);
                        }
                        else
                            outfile->setStrValue(MAIN,StrNum,STR_NULL);
                    }
                    else
                        outfile->setStrValue(MAIN,StrNum,STR_NULL);
                }
                Samba_freeHandle(Handle);
            }
        }
        break;
    case 3: // Query Server
        {
            char *IniValue;
            IniValue=infile->getStrValue(MAIN,P1);
            if(IniValue)
            {
                long Handle;
                int num;
                
                char *IniRealValue=NULL;
                char *DomainName=NULL;
                if(!infile->removeStrMark_needfree(IniValue,&IniRealValue))
                    DomainName=IniRealValue;
                else
                    DomainName=IniValue;
                jsprintf(" DomainName = %s , %s , %s\n",IniValue,IniRealValue,DomainName);
                if(Samba_QueryServerList(&Handle,&num,DomainName)<0)
                    outfile->setIntValue(MAIN, NUMBER,-1);
                else
                {
                    char StrNum[32];
                    outfile->setIntValue(MAIN, NUMBER,num);
                    
                    for(int i=0;i<num;i++)
                    {
                        char *cTemp=NULL;
                        char *cOutput=NULL;
                        sprintf(StrNum,"%d",i);
                        
                        if(!Samba_GetServerList(Handle,i,&cTemp))
                        {
                            if(!outfile->addStrMark_needfree(cTemp,&cOutput))
                            {
                                outfile->setStrValue(MAIN,StrNum,cOutput);
                                FREE(cOutput);
                            }
                            else
                                outfile->setStrValue(MAIN,StrNum,STR_NULL);
                        }
                        else
                            outfile->setStrValue(MAIN,StrNum,STR_NULL);
                    }
                    Samba_freeHandle(Handle);
                }
                if(IniRealValue)
                    FREE(IniRealValue);
            }
            else
                outfile->setIntValue(MAIN, NUMBER,0);
        }
        break;
    case 4: // Query Sharelist
        {
            long Handle;
            int num;
            
            char *IPAddr=NULL;
            char *UserName=NULL;
            char *Password=NULL;
            char *DomainName=NULL;
            char *IniValue;
// get IP
            IniValue=infile->getStrValue(MAIN,P1);
            if(IniValue)
            {
                char *IniRealValue=NULL;
                if(!infile->removeStrMark_needfree(IniValue,&IniRealValue))
                    IPAddr=IniRealValue;
                else
                    IPAddr=strdup(IniValue);
                jsprintf(" IPAddr = %s , %s , %s\n",IniValue,IniRealValue,IPAddr);
            }
// get UserName
            IniValue=infile->getStrValue(MAIN,P2);
            if(IniValue)
            {
                char *IniRealValue=NULL;
                if(!infile->removeStrMark_needfree(IniValue,&IniRealValue))
                    UserName=IniRealValue;
                else
                    UserName=strdup(IniValue);
                jsprintf(" UserName = %s , %s , %s\n",IniValue,IniRealValue,UserName);
            }
// get Password
            IniValue=infile->getStrValue(MAIN,P3);
            if(IniValue)
            {
                char *IniRealValue=NULL;
                if(!infile->removeStrMark_needfree(IniValue,&IniRealValue))
                    Password=IniRealValue;
                else
                    Password=strdup(IniValue);
                jsprintf(" Password = %s , %s , %s\n",IniValue,IniRealValue,Password);
            }
// get DomainName
            IniValue=infile->getStrValue(MAIN,P4);
            if(IniValue)
            {
                char *IniRealValue=NULL;
                if(!infile->removeStrMark_needfree(IniValue,&IniRealValue))
                    DomainName=IniRealValue;
                else
                    DomainName=strdup(IniValue);
                jsprintf(" DomainName = %s , %s , %s\n",IniValue,IniRealValue,DomainName);
            }
            if(Samba_QueryServerShareList(&Handle,&num, IPAddr, UserName, Password,DomainName))
                outfile->setIntValue(MAIN, NUMBER,-1);
            else
            {
                char StrNum[32];
                outfile->setIntValue(MAIN, NUMBER,num);
                for (int i=0;i<num;i++)
                {
                    char *cTemp=NULL;
                    char *cOutput=NULL;
                    int type;
                    sprintf(StrNum,"%d",i);
                    if(!Samba_GetServerShareList(Handle,i,&cTemp,&type))                    
                    {
                        if(!outfile->addStrMark_needfree(cTemp,&cOutput))
                        {
                            outfile->setStrValue(StrNum,P1,cOutput);
                            FREE(cOutput);
                        }
                        else
                            outfile->setStrValue(StrNum,P1,STR_NULL);
                        outfile->setIntValue(StrNum,P2,type);
                    }
                    else
                    {
                        outfile->setStrValue(StrNum,P1,STR_NULL);
                        outfile->setIntValue(StrNum,P2,-1);
                    }
                }
                Samba_freeServerShareHandle(Handle,num);
            }
            FREE(IPAddr);
            FREE(UserName);
            FREE(Password);
            FREE(DomainName);
        }
        break;
    default:
        bCreateOutput=false;
        break;
    }
    if(bCreateOutput)
        outfile->saveFile((char *)argv[2]);
    delete infile;
    delete outfile;
    return 0;
}
