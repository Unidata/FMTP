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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief     A testing application to use the receiver side protocol.
 *
 * Since the LDM could be to heavy to use for testing purposes only. This
 * testRecvApp is written as a replacement. It can create an instance of the
 * vcmtpRecvv3 instance and mock the necessary components to get it functioning.
 */


#include "vcmtpRecvv3.h"

#include <iostream>
#include <thread>


/**
 * A separate thread to run VCMTP receiver.
 *
 * @param[in] *ptr    A pointer to a vcmtpRecvv3 object.
 */
void runVCMTP(void* ptr)
{
    vcmtpRecvv3 *recv = static_cast<vcmtpRecvv3*>(ptr);
    recv->Start();
}


/**
 * Since the LDM could be too heavy to use for testing purposes only. This main
 * function is a light weight replacement of the LDM receiving application. It
 * sets up the whole environment and call Start() to start receiving. All the
 * arguments are passed in through command line.
 *
 * @param[in] tcpAddr      IP address of the sender.
 * @param[in] tcpPort      Port number of the sender.
 * @param[in] mcastAddr    multicast address of the group.
 * @param[in] mcastPort    Port number of the multicast group.
 * @param[in] ifAddr       IP of the interface to listen for multicast packets.
 */
int main(int argc, char* argv[])
{
    if (argc < 5) {
        std::cerr << "ERROR: Insufficient arguments." << std::endl;
        return 1;
    }
    std::string tcpAddr(argv[1]);
    const unsigned short tcpPort = (unsigned short)atoi(argv[2]);
    std::string mcastAddr(argv[3]);
    const unsigned short mcastPort = (unsigned short)atoi(argv[4]);
    std::string ifAddr(argv[5]);

    vcmtpRecvv3* recv = new vcmtpRecvv3(tcpAddr, tcpPort, mcastAddr,
                                        mcastPort, NULL, ifAddr);
    recv->SetLinkSpeed(1000000000);
    std::thread t(runVCMTP, recv);
    t.join();

    delete recv;
    return 0;
}
