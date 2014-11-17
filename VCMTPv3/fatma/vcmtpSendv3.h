/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpSendv3.h
 * @author    Fatma Alali <fha6np@virginia.edu>
 *            Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 16, 2014
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
 * @brief     Define the interfaces of VCMTPv3 sender side method function.
 *
 * Sender side of VCMTPv3 protocol. It multicasts packets out to multiple
 * receivers and retransmits missing blocks to the receivers.
 */


#ifndef VCMTPSENDV3_H_
#define VCMTPSENDV3_H_


#include <sys/types.h>
#include "UdpSocket.h"
#include "TcpSend.h"
#include "vcmtpBase.h"


class vcmtpSendv3 {
public:
    vcmtpSendv3(
            const char*          tcpAddr,
            const unsigned short tcpPort,
            const char*          mcastAddr,
            const unsigned short mcastPort);
    vcmtpSendv3(
            const char*          tcpAddr,
            const unsigned short tcpPort,
            const char*          mcastAddr,
            const unsigned short mcastPort,
            uint32_t             initProdIndex);

    ~vcmtpSendv3();
    uint32_t sendProduct(char* data, size_t dataSize);
    uint32_t sendProduct(char* data, size_t dataSize, char* metadata,
                         unsigned metaSize);
    void acceptConn();

private:
    uint32_t prodIndex;
    UdpSocket* udpsocket;
    TcpSend*   tcpsend;
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    void sendEOPMessage();

};

#endif /* VCMTPSENDV3_H_ */
