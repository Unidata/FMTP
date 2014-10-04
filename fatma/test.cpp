/*
 * test.cpp

 *
 *  Created on: Oct 4, 2014
 *      Author: fatmaal-ali
 */
#include"VCMTPSender.h"
#include<string>

int main()
{
	char data[]="Hello";
	const u_int32_t id=1;
	//char* dataptr;
	//dataptr= &data;
	VCMTPSender* sender;
	cout<<"create new vcmtp sender with file id = "<<id<<endl;
	sender= new VCMTPSender(id);
	//sender.CreateUPDSocket("cs.virginia.edu/tye",1234);
	sender->CreateUPDSocket("128.143.137.117",5000);
	cout<<"create udp socket "<<endl;

	while(1){
	sender->SendMemoryData((char *)data,sizeof(data),"FHA");
	cout<< "send the bof message\n";
	sleep(2);
	}
}



