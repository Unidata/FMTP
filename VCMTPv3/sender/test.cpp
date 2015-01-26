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
	//sender = new vcmtpSendv3("192.168.0.101", 1234, "128.143.137.117", 5173, 0);
	sender = new vcmtpSendv3("127.0.0.1", 1234, "233.0.225.123", 5173, 0, 0);
	char *filename = "RANDDATA";
	int fd;
	fd = open(filename,O_RDONLY);
	if(fd>0)
	{
		void* data;
		data = (char*) mmap(0, 5792, PROT_READ, MAP_FILE | MAP_SHARED, fd,0);
		if (data == MAP_FAILED)
			std::cout << "file map failed" << std::endl;

        sender->startCoordinator();
        sleep(2);
		sender->sendProduct(data, 5792, metadata, metaSize);
        while(1);

		munmap(data, 5792);
		close(fd);
	}
	else
		std::cout << "test::main()::open(): error" << std::endl;

    delete sender;
	return 0;
}



