/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpSend.h
 *
 * @history:
 *      Created on : Oct 14, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPSEND_H_
#define VCMTPSEND_H_


#include "../vcmtpBase.h"
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string>

using namespace std;

class vcmtpSend : public vcmtpBase {
public:
    vcmtpSend(uint64_t id,
              const char* recvAddr,
              const unsigned short recvPort);
    ~vcmtpSend();
    void     startGroup(const char* addr, unsigned port);
    void     SendBOMDMsg(uint64_t prodSize, char* prodName, int sizeOfProdName);
    void     CreateUDPSocket(const char* recvName, unsigned short int recvPort);
    void     sendMemData(char* data, uint64_t prodSize, string &prodName);
    /**
     * Transfers a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @return              Index of the product.
     */
    uint32_t vcmtpSend::sendProduct(void* data, size_t dataSize) {
        return sendProduct(data, dataSize, 0, 0);
    };
    /**
     * Transfers application-specific metadata and a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @param[in] metadata  Application-specific metadata to be sent before the
     *                      data. May be 0, in which case no metadata is sent.
     * @param[in] metaSize  Size of the metadata in bytes. Must be less than
     *                      1428. May be 0, in which case no metadata is sent.
     * @return              Index of the product.
     */
    uint32_t vcmtpSend::sendProduct(void* data, size_t dataSize, void* metadata,
            unsigned metaSize);
    void     sendEOMDMsg();
    ssize_t  SendTo(const void* buff, size_t len, int flags);
    ssize_t  SendData(void* header, const size_t headerLen,
                     void* data, const size_t dataLen);

private:
    int                 sock_fd;
    uint64_t            prodId;
    struct sockaddr_in  recv_addr;
};

#endif /* VCMTPSEND_H_ */
