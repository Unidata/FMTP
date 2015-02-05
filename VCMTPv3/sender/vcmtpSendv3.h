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
#include "UdpSend.h"
#include "TcpSend.h"
#include "vcmtpBase.h"
#include "senderMetadata.h"
#include "SendAppNotifier.h"
#include <map>
#include <set>
#include <pthread.h>
#include <ctime>

class vcmtpSendv3;

/**
 * To contain multiple types of necessary information and transfer to the
 * StartRetxThread() as one single parameter.
 */
struct StartRetxThreadInfo
{
    /**
     * A pointer to the vcmtpSendv3 instance itself which starts the
     * StartNewRetxThread().
     */
    vcmtpSendv3*    retxmitterptr;
    /** The particular retx socket this running thread is listening on */
    int             retxsockfd;
};


/**
 * To contain multiple types of necessary information and transfer to the
 * StartTimerThread() as one single parameter.
 */
struct StartTimerThreadInfo
{
    uint32_t        prodindex; /*!< product index */
    vcmtpSendv3*    sender;    /*!< a poniter to the vcmtpSendv3 instance */
};


/**
 * sender side class handling the multicasting, restransmission and timeout.
 */
class vcmtpSendv3
{
public:
    explicit vcmtpSendv3(
            const char*           tcpAddr,
            const unsigned short  tcpPort,
            const char*           mcastAddr,
            const unsigned short  mcastPort);
    explicit vcmtpSendv3(
            const char*           tcpAddr,
            const unsigned short  tcpPort,
            const char*           mcastAddr,
            const unsigned short  mcastPort,
            uint32_t              initProdIndex,
            SendAppNotifier*      notifier);
    explicit vcmtpSendv3(
            const char*           tcpAddr,
            const unsigned short  tcpPort,
            const char*           mcastAddr,
            const unsigned short  mcastPort,
            uint32_t              initProdIndex,
            float                 timeoutRatio,
            unsigned char         ttl,
            SendAppNotifier*      notifier);
    ~vcmtpSendv3();

    uint32_t sendProduct(void* data, size_t dataSize);
    uint32_t sendProduct(void* data, size_t dataSize, void* metadata,
                         unsigned metaSize);
    void startCoordinator();
    unsigned short getTcpPortNum();

private:
    uint32_t                    prodIndex;
    /** underlying udp layer instance */
    UdpSend*                    udpsend;
    /** underlying tcp layer instance */
    TcpSend*                    tcpsend;
    /** maintaining metadata for retx use. */
    senderMetadata*             sendMeta;
    /** sending application callback hook */
    SendAppNotifier* notifier;
    float                       retxTimeoutRatio;
    /** first: socket fd;  second: pthread_t pointer */
    std::map<int, pthread_t*>   retxSockThreadMap;
    /** first: socket fd;  second: retransmission finished indicator */
    std::map<int, bool>         retxSockFinishMap;
    /** first: socket fd;  second: pointer to the retxThreadInfo struct */
    std::map<int, StartRetxThreadInfo*> retxSockInfoMap;
    /**
     * Adds and entry for a data-product to the retransmission set.
     *
     * @param[in] data      The data-product.
     * @param[in] dataSize  The size of the data-product in bytes.
     * @return              The corresponding retransmission entry.
     * @throw std::runtime_error  if a retransmission entry couldn't be created.
     */
    RetxMetadata* addRetxMetadata(void* const data, const size_t dataSize,
                                  void* const metadata, const size_t metaSize);
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    /**
     * Multicasts the data of a data-product.
     *
     * @param[in] data      The data-product.
     * @param[in] dataSize  The size of the data-product in bytes.
     * @throw std::runtime_error  if an I/O error occurs.
     */
    void sendEOPMessage();
    void sendData(
            void* const  data,
            const size_t dataSize);
    /**
     * Sets the retransmission timeout parameters in a retransmission entry.
     *
     * @param[in] senderProdMeta  The retransmission entry.
     */
    void setTimerParameters(RetxMetadata* const senderProdMeta);
    void StartNewRetxThread(int newtcpsockfd);
    void startTimerThread(uint32_t prodindex);
    /** new coordinator thread */
    static void* coordinator(void* ptr);
    /** new retranmission thread */
    static void* StartRetxThread(void* ptr);
    /** new timer thread */
    static void* runTimerThread(void* ptr);
    void RunRetxThread(int retxsockfd);
    /**
     * Handles a retransmission request.
     *
     * @param[in] recvheader  VCMTP header of the retransmission request.
     * @param[in] retxMeta    Associated retransmission entry.
     */
    void handleRetxReq(VcmtpHeader* const  recvheader,
                       RetxMetadata* const retxMeta,
                       const int           sock);
    /**
     * Handles a notice from a receiver that a data-product has been completely
     * received.
     *
     * @param[in] recvheader  The VCMTP header of the notice.
     * @param[in] retxMeta    The associated retransmission entry.
     * @param[in] sock        The receiver's socket.
     */
    void handleRetxEnd(VcmtpHeader* const  recvheader,
                       RetxMetadata* const retxMeta,
                       const int           sock);
    /**
     * Handles a notice from a receiver that BOP for a product is missing.
     *
     * @param[in] recvheader  The VCMTP header of the notice.
     * @param[in] retxMeta    The associated retransmission entry.
     * @param[in] sock        The receiver's socket.
     */
    void handleBopReq(VcmtpHeader* const  recvheader,
                      RetxMetadata* const retxMeta,
                      const int           sock);
    /**
     * Handles a notice from a receiver that EOP for a product is missing.
     *
     * @param[in] recvheader  The VCMTP header of the notice.
     * @param[in] retxMeta    The associated retransmission entry.
     * @param[in] sock        The receiver's socket.
     */
    void handleEopReq(VcmtpHeader* const  recvheader,
                      RetxMetadata* const retxMeta,
                      const int           sock);
    /**
     * Rejects a retransmission request from a receiver.
     *
     * @param[in] prodindex  Product-index of the request.
     * @param[in] sock       The receiver's socket.
     */
    void rejRetxReq(const uint32_t prodindex, const int sock);
    /**
     * Retransmits data to a receiver.
     *
     * @param[in] recvheader  The VCMTP header of the retransmission request.
     * @param[in] retxMeta    The associated retransmission entry.
     * @param[in] sock        The receiver's socket.
     */
    void retransmit(
            const VcmtpHeader* const  recvheader,
            const RetxMetadata* const retxMeta,
            const int                 sock);
    /**
     * Retransmits BOP packet to a receiver.
     *
     * @param[in] recvheader  The VCMTP header of the retransmission request.
     * @param[in] retxMeta    The associated retransmission entry.
     * @param[in] sock        The receiver's socket.
     */
    void retransBOP(const VcmtpHeader* const  recvheader,
                    const RetxMetadata* const retxMeta,
                    const int                 sock);
    /**
     * Retransmits EOP packet to a receiver.
     *
     * @param[in] recvheader  The VCMTP header of the retransmission request.
     * @param[in] sock        The receiver's socket.
     */
    void retransEOP(const VcmtpHeader* const  recvheader,
                    const int                 sock);
    /* Prevent copying because it's meaningless */
    vcmtpSendv3(vcmtpSendv3&);
    vcmtpSendv3& operator=(const vcmtpSendv3&);
    static uint32_t blockIndex(uint32_t start) {return start/VCMTP_DATA_LEN;}
};

#endif /* VCMTPSENDV3_H_ */
