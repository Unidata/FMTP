/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpRecvv3.h
 *
 * @history:
 *      Created on : Oct 17, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPRECVV3_H_
#define VCMTPRECVV3_H_

#include "vcmtpBase.h"
#include <sys/socket.h>
#include <arpa/inet.h>

class vcmtpRecvv3 {
public:
    vcmtpRecvv3();
    virtual ~vcmtpRecvv3();

    /**
     * Join a multicast group
     *
     * @param[in] mcastAddr      multicast group ip address.
     * @param[in] mcastPort      multicast group port number.
     */
    void joinGroup(const char* mcastAddr, unsigned short mcastPort);

    void BOPHandler(char* msgptr, size_t msgsize);
    /**
     * Receive a contiguous block of memory.
     *
     * @param[in] data      Memory data to be received.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @return              Index of the product.
     */
    uint32_t recvProduct(void* data, size_t dataSize);

    /**
     * Receive Application-specific metadata and a contiguous block of memory.
     *
     * @param[in] data      Memory data to be received.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @param[in] metadata  Application-specific metadata to be sent before the
     *                      data. May be 0, in which case no metadata is sent.
     * @param[in] metaSize  Size of the metadata in bytes. Must be less than
     *                      1428. May be 0, in which case no metadata is sent.
     * @return              Index of the product.
     */
    uint32_t recvProduct(void* data, size_t dataSize, void* metadata,
                         unsigned metaSize);
};

private:
    struct sockaddr_in  mcastgroup;
    int                 mcast_sock;
    VcmtpHeader         vcmtpHeader;
    BOPMsg              BOPmsg;

#endif /* VCMTPRECVV3_H_ */
