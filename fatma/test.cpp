/*
 * test.cpp

 *
 *  Created on: Oct 4, 2014
 *      Author: fatmaal-ali
 */
#include"faVCMTPSender.h"
#include<string>


int main()
{
	const u_int64_t id=1;
	//const uint64_t fileSize=1048576;
	string prodId="5";
	//=0x64;
	faVCMTPSender* sender;
	sender= new faVCMTPSender(id);
	cout<<"main(): create new vcmtp sender with file id = "<<id<<endl;
	//pass the address and port number of the receiver
	sender->CreateUPDSocket("128.143.137.117",5173);

	int fd = open("if13/fha6np/Downloads/FatmaVCMTP3/FHA2", O_RDONLY);
	void* data;
	data = (char*) mmap(0, 2856, PROT_READ, 0, fd,0);

	//char data2[]="Hello";
	//char* ptr=data2;
	while(1){
	sender->sendMemoryData( data,2856, prodId);
	sleep(1);
	}

	munmap(data, 2856);
	close(fd);
}



