/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      testRecvApp.cpp
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Feb 14, 2015
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
 * @brief     A testing application to use the receiver side protocol.
 *
 * Since the LDM could be to heavy to use for testing purposes only. This
 * testRecvApp is written as a replacement. It can create an instance of the
 * vcmtpRecvv3 instance and mock the necessary components to get it functioning.
 */


#include "vcmtpRecvv3.h"

#include <iostream>


int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cerr << "ERROR: Insufficient arguments." << std::endl;
        return 1;
    }
    /*
    string tcpAddr                 = "127.0.0.1";
    const unsigned short tcpPort   = 1234;
    string mcastAddr               = "233.0.225.123";
    const unsigned short mcastPort = 5173;
    */
    std::string tcpAddr(argv[1]);
    const unsigned short tcpPort = (unsigned short)atoi(argv[2]);
    std::string mcastAddr(argv[3]);
    const unsigned short mcastPort = (unsigned short)atoi(argv[4]);

    vcmtpRecvv3 vcmtpRecvv3(tcpAddr, tcpPort, mcastAddr, mcastPort);
    vcmtpRecvv3.SetLinkSpeed(1000000000);
    vcmtpRecvv3.Start();
    return 0;
}
