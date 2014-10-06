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

	const uint64_t id=1;
	const uint64_t fileSize=0x64;
	faVCMTPSender* sender;
	sender= new faVCMTPSender(id);
	cout<<"main(): create new vcmtp sender with file id = "<<id<<endl;
	//pass the address and port number of the receiver
	sender->CreateUPDSocket("128.143.137.117",5173);

	while(1){
	sender->SendBOFMessage(fileSize,"FHA");
	sleep(2);
	}
}



