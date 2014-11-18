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
	sender = new vcmtpSendv3("127.0.0.1", 1234, "128.143.137.117", 5173, 0);
	char *filename = "TESTDATA";
	int fd;
	fd = open(filename,O_RDONLY);
	if(fd>0)
	{
		char* data;
		data = (char*) mmap(0, 2856, PROT_READ, MAP_FILE | MAP_SHARED, fd,0);
		if (data == MAP_FAILED)
			cout<<"file map failed"<<endl;

		//	while(1){
		sender->sendProduct(data, 2856, metadata, metaSize);
        sender->acceptConn();
		//	sleep(1);
		//	}

		munmap(data, 2856);
		close(fd);
	}
	else
		cout<<"test::main()::open(): error"<<endl;

	return 0;
}



