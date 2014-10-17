/*
 *test.cpp
 
 *
 *  Created on: Oct 4, 2014
 *      Author: fatmaal-ali
 */
#include"faVCMTPSender.h"
#include<string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
int main()
{
	const u_int64_t id=1;
	string prodName="FHA2";
	
	faVCMTPSender* sender;
	sender= new faVCMTPSender(id,"128.143.137.117",5173);
	cout<<"main(): create new vcmtp sender with file id = "<<id<<endl;
	//pass the address and port number of the receiver
	//sender->CreateUPDSocket("128.143.137.117",5173);
	char *filename = "/home/fatma/FatmaVCMTP/FHA2";
	int fd;
	fd = open(filename,O_RDONLY);
	if(fd>0)
	{
		void* data;
		data = (char*) mmap(0, 2856, PROT_READ, MAP_FILE | MAP_SHARED, fd,0);
		if (data == MAP_FAILED)
			cout<<"file map failed"<<endl;
		
		//	while(1){
		sender->sendMemoryData( data,2856, prodName);
		//	sleep(1);
		//	}
		
		munmap(data, 2856);
		close(fd);
	}
	else
		cout<<"test::main()::open(): error"<<endl;
	
	return 0;
}



