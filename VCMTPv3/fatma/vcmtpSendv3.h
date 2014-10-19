/*
 *vcmtpSendv3.h
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#ifndef VCMTPSENDV3_H_
#define VCMTPSENDV3_H_


#include <sys/types.h>
#include"UdpSocket.h"
#include"vcmtpBase.h"
class vcmtpSendv3 {
public:
    vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort);
    vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort,
            uint32_t     initProdIndex);

    ~vcmtpSendv3();
    uint32_t sendProduct(void* data, size_t dataSize);
    uint32_t sendProduct(void* data, size_t dataSize, void* metadata,
            unsigned metaSize);

private:
    uint32_t prodIndex;
    UdpSocket* udpsocket;
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    void sendEOPMessage();

};

#endif /* VCMTPSENDV3_H_ */



