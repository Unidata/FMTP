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

    //void sendProdStream(const char* streamName, uint32_t initProdIndex);
    //void startGroup(const char* addr,const ushort port);

    /**
     * Transfers a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @return              Index of the product.
     */
    uint32_t sendProduct(void* data, size_t dataSize);

    /**
     * Transfers Application-specific metadata and a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @param[in] metadata  Application-specific metadata to be sent before the
     *                      data. May be 0, in which case no metadata is sent.
     * @param[in] metaSize  Size of the metadata in bytes. Must be less than
     *                      1428. May be 0, in which case no metadata is sent.
     * @return              Index of the product.
     */
    uint32_t sendProduct(void* data, size_t dataSize, void* metadata,
            unsigned metaSize);

private:
    uint32_t prodIndex;
    UdpSocket* udpsocket;
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    void sendEOPMessage();

};

#endif /* VCMTPSENDV3_H_ */



