/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: main.cpp
 *
 * @history:
 *      Created on : Oct 14, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#include "vcmtpSend.h"
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

int main()
{
    const uint64_t id = 1;
    string prodName = "memdata";

    vcmtpSend sender(id, "127.0.0.1", 5173);
    cout << "(VCMTP Header) id:" << id << endl;

    char* filename = "memdata";
    int fd;
    fd = open(filename, O_RDONLY);
    if(fd < 0)
        cout << "test::main()::open(): error" << endl;

    char* data;
    data = (char*) mmap(0, 2856, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    if(data == MAP_FAILED)
        cout << "file map failed" << endl;

    sender.sendMemData(data, 2856, prodName);

    munmap(data, 2856);
    close(fd);
    return 0;
}
