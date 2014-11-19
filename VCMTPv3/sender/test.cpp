/*
 *test.cpp
 *
 *  Created on: Oct 17, 2014
 *      Author: fatmaal-ali
 */

#include"vcmtpSendv3.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main()
{
	char data[]="ldm checksum";
	char* metadata = data;
	unsigned int metaSize=sizeof(data);

	vcmtpSendv3* sender;
	//sender = new vcmtpSendv3("0.0.0.0", 1234, "233.0.225.123", 5173, 0);
	//sender = new vcmtpSendv3("0.0.0.0", 1234, "172.25.99.89", 5173, 0);
	sender = new vcmtpSendv3("0.0.0.0", 0, "172.25.99.89", 5173, 0);
	char *filename = "TESTDATA";
	int fd;
	fd = open(filename,O_RDONLY);
	if(fd>0)
	{
		char* data;
		data = (char*) mmap(0, 2856, PROT_READ, MAP_FILE | MAP_SHARED, fd,0);
		if (data == MAP_FAILED)
			cout<<"file map failed"<<endl;

        //sender->acceptConn();
        int portnum = sender->getTcpPortNum();
        if(portnum > 0)
        	cout << "Port Number: " << portnum << endl;
		//sender->sendProduct(data, 2856, metadata, metaSize);
        //sender->readSock();
        while(1);

		munmap(data, 2856);
		close(fd);
	}
	else
		cout<<"test::main()::open(): error"<<endl;

	return 0;
}



