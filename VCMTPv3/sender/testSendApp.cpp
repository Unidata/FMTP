/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      testSendApp.cpp
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Feb 15, 2015
 *
 * @section   LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or（at your option）
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details at http://www.gnu.org/copyleft/gpl.html
 *
 * @brief     A testing application to use the sender side protocol.
 *
 * Since the LDM could be too heavy to use for testing purposes only. This
 * testSendApp is written as a replacement. It can create an instance of the
 * vcmtpSendv3 instance and mock the necessary components to get it functioning.
 */


#include "vcmtpSendv3.h"

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <random>
#include <string>


/**
 * Generates a random sized file filled with random bytes. The filename is
 * fixed as test.dat. But for every iteration its size will be changed, caller
 * needs to re-open it and read again.
 */
void randDataGen()
{
    std::random_device rd;
    /* random file size range 1KB - 10MB */
    unsigned int rand = rd() % 10240 + 1;
    rand = rand * 1024;

    char* data = new char[rand];
    std::ifstream fp("/dev/urandom", std::ios::binary);
    if (fp.is_open()) {
        fp.read(data, rand);
    }

    std::ofstream rdfile("test.dat");
    if (rdfile.is_open()) {
        rdfile.write(data, rand);
    }
    fp.close();
    rdfile.close();
    delete[] data;
    data = NULL;
}


/**
 * Since the LDM could be too heavy to use for testing purposes only. This main
 * function is a light weight replacement of the LDM sending application. It
 * sets up the whole environment and call sendProduct() to send data. All the
 * arguments are passed in through command line.
 *
 * @param[in] tcpAddr      IP address of the sender.
 * @param[in] tcpPort      Port number of the sender.
 * @param[in] mcastAddr    multicast address of the group.
 * @param[in] mcastPort    Port number of the multicast group.
 * @param[in] ifAddr       IP of the interface to set as default.
 */
int main(int argc, char const* argv[])
{
    if (argc < 4) {
        std::cerr << "ERROR: Insufficient arguments." << std::endl;
        return 1;
    }

    std::string tcpAddr(argv[1]);
    const unsigned short tcpPort = (unsigned short)atoi(argv[2]);
    std::string mcastAddr(argv[3]);
    const unsigned short mcastPort = (unsigned short)atoi(argv[4]);
    std::string ifAddr(argv[5]);
    std::string filename("test.dat");

    char tmp[] = "test metadata";
    char* metadata = tmp;
    unsigned int metaSize = sizeof(tmp);

    vcmtpSendv3* sender =
        new vcmtpSendv3(tcpAddr.c_str(), tcpPort, mcastAddr.c_str(), mcastPort,
                        0, 0);

    sender->Start();
    sender->SetDefaultIF(ifAddr.c_str());
    sleep(2);

    for(int i=0; i<100; ++i) {
        /* generate random sized data */
        randDataGen();

        /** use the filename to get filesize */
        struct stat filestatus;
        stat(filename.c_str(), &filestatus);
        size_t datasize = filestatus.st_size;

        int fd = open(filename.c_str(), O_RDONLY);
        if(fd > 0)
        {
            void* data = (char*) mmap(0, datasize, PROT_READ,
                                      MAP_FILE | MAP_SHARED, fd, 0);
            if (data == MAP_FAILED)
                std::cerr << "file map failed" << std::endl;

            sender->sendProduct(data, datasize, metadata, metaSize);
            /* 1 sec interval between two products */
            sleep(1);

            munmap(data, datasize);
            close(fd);
        }
        else
            std::cerr << "test::main()::open(): error" << std::endl;
    }
    delete sender;

    while(1);
    return 0;
}
