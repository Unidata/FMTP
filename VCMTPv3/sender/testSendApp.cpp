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

#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>


/**
 * Read a metadata file which contains the file sizes of a pareto distribution.
 */
void metaParse(unsigned int* pvec, unsigned int pvecsize, std::string& filename)
{
    std::string line;
    std::ifstream fp(filename, std::ios::binary);
    if (fp.is_open()) {
        for(int i=0; i < pvecsize; ++i) {
            std::getline(fp, line);
            pvec[i] = std::stoi(line);
        }
    }
    fp.close();
}


/**
 * Generates a pareto distributed file filled with random bytes. The content
 * of this file would be stored in heap and pointed by a pointer.
 */
char* paretoGen(unsigned int size)
{
    char* data = new char[size];
    std::ifstream fp("/dev/urandom", std::ios::binary);
    if (fp.is_open()) {
        fp.read(data, size);
    }
    fp.close();
    return data;
}


/**
 * Delete dynamically allocated heap pointer.
 */
void paretoDestroy(char* data)
{
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
 * @param[in] filename     filename of the metadata.
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
    std::string filename(argv[6]);

    char tmp[] = "test metadata";
    char* metadata = tmp;
    unsigned int metaSize = sizeof(tmp);

    vcmtpSendv3* sender =
        new vcmtpSendv3(tcpAddr.c_str(), tcpPort, mcastAddr.c_str(), mcastPort,
                        0, 1, ifAddr.c_str());

    sender->Start();
    sleep(2);

    /* specify how many metadata files to send */
    unsigned int pvecsize = 10;
    unsigned int * pvec = new unsigned int[pvecsize];
    metaParse(pvec, pvecsize, filename);

    for(int i=0; i < pvecsize; ++i) {
        /* generate pareto distributed data */
        char * data = paretoGen(pvec[i]);
        sender->sendProduct(data, pvec[i], metadata, metaSize);
        paretoDestroy(data);
        /* 1 sec interval between two products */
        sleep(1);
    }
    delete[] pvec;

    while(1);

    delete sender;
    return 0;
}
