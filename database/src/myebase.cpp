#include "Utility/database/myebase.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>


MyEbase::MyEbase(char *RamdbFileName,char *dbFileName)
{
	    strcpy (m_dbFileName, dbFileName);
		strcpy (m_RamdbFileName, RamdbFileName);
		char tmp[512];
		sprintf(tmp,"cp %s %s",m_dbFileName,m_RamdbFileName);
		//if cp fail ,it means m_dbFileName is not exist or too large to cp to ramfs
		//but we dont care,In this case,OpenDatabase will fail,and we will create new db in ramfs
		int ret = system(tmp);
		
};


int MyEbase::CreateDatabase(){
	return Ebase::CreateDatabase(m_RamdbFileName);
};


int MyEbase::OpenDatabase(){
	return Ebase::OpenDatabase(m_RamdbFileName);
};


int MyEbase::Sync(){
	char tmp[512];
	sprintf(tmp,"cp %s %s",m_RamdbFileName,m_dbFileName);
	int ret=system(tmp);
	//if error return -1 else return 0
	if(ret != -1)return 0;
	return -1;
};