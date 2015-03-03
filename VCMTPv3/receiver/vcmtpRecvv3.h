/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpRecvv3.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 17, 2014
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
 * @brief     Define the interfaces of VCMTPv3 receiver.
 *
 * Receiver side of VCMTPv3 protocol. It handles incoming multicast packets
 * and issues retransmission requests to the sender side.
 */


#ifndef VCMTP_RECEIVER_VCMTPRECVV3_H_
#define VCMTP_RECEIVER_VCMTPRECVV3_H_

#include <condition_variable>
#include <exception>
#include <mutex>
#include "vcmtpBase.h"
#include "RecvAppNotifier.h"
#include <stdint.h>
#include <string>
#include <sys/select.h>
#include <netinet/in.h>
#include <queue>
#include <list>
#include "TcpRecv.h"
#include "ProdBitMap.h"

class vcmtpRecvv3;

struct StartTimerInfo
{
    uint32_t     prodindex;  /*!< product index */
    float        seconds;    /*!< product index */
    vcmtpRecvv3* receiver;   /*!< a poniter to the vcmtpRecvv3 instance */
};


class vcmtpRecvv3 {
public:
    vcmtpRecvv3(std::string tcpAddr,
                const unsigned short tcpPort,
                std::string mcastAddr,
                const unsigned short mcastPort,
                RecvAppNotifier* notifier);
    vcmtpRecvv3(std::string tcpAddr,
                const unsigned short tcpPort,
                std::string mcastAddr,
                const unsigned short mcastPort);
    ~vcmtpRecvv3();
    void    Start();
    void    Stop();

private:
    std::string             tcpAddr;
    unsigned short          tcpPort;
    std::string             mcastAddr;
    unsigned short          mcastPort;
    int                     mcastSock;
    int                     retxSock;
    struct sockaddr_in      mcastgroup;
    /** struct of multicast object */
    struct ip_mreq          mreq;
    /** temporary header buffer for each vcmtp packet */
    VcmtpHeader             vcmtpHeader;
    std::mutex              vcmtpHeaderMutex;
    /** begin of product struct */
    BOPMsg                  BOPmsg;
    /** callback function of the receiving application */
    RecvAppNotifier*        notifier;
    /** pointer to a start point in product queue */
    void*                   prodptr;
    TcpRecv*                tcprecv;
    ProdBitMap*             bitmap;
    std::queue<INLReqMsg>   msgqueue;
    std::condition_variable msgQfilled;
    std::mutex              msgQmutex;
    /** track all the missing BOP until received */
    std::list<uint32_t>     misBOPlist;
    std::mutex              BOPListMutex;
    /*!< the state of EOP, true: received false: missing */
    bool                    EOPStatus;
    std::mutex              EOPStatMtx;
    pthread_t               retx_rq; /*!< Retransmission request thread */
    pthread_t               retx_t;  /*!< Retransmission receive thread */
    pthread_t               mcast_t; /*!< Multicast receiver thread     */
    pthread_t               timer_t; /*!< BOP timer thread              */
    /** a queue containing timerParam structure for each product */
    std::queue<timerParam>  timerParamQ;
    std::condition_variable timerQfilled;
    std::mutex              timerQmtx;
    std::condition_variable timerWake;
    std::mutex              timerWakemtx;
    std::mutex              exitMutex;
    std::exception          except;
    bool                    exceptIsSet;

    void    joinGroup(std::string mcastAddr, const unsigned short mcastPort);
    static void*  StartRetxRequester(void* ptr);
    static void*  StartRetxHandler(void* ptr);
    static void*  StartMcastHandler(void* ptr);
    void    StartRetxProcedure();
    void    mcastHandler();
    void    retxHandler();
    void    retxRequester();
    /**
     * Decodes the header of a VCMTP packet in-place.
     *
     * @param[in,out] header  The VCMTP header to be decoded.
     */
    void decodeHeader(VcmtpHeader& header);
    /**
     * Decodes a VCMTP packet header.
     *
     * @param[in]  packet         The raw packet.
     * @param[in]  nbytes         The size of the raw packet in bytes.
     * @param[out] header         The decoded packet header.
     * @param[out] payload        Payload of the packet.
     * @throw std::runtime_error  if the packet is too small.
     * @throw std::runtime_error  if the packet has in invalid payload length.
     */
    void decodeHeader(char* const packet, const size_t nbytes,
                      VcmtpHeader& header, char** const payload);
    void checkPayloadLen(const VcmtpHeader& header, const size_t nbytes);
    /**
     * Parse BOP message and call notifier to notify receiving application.
     *
     * @param[in] header           Header associated with the packet.
     * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
     * @throw std::runtime_error   if the payload is too small.
     */
    void BOPHandler(const VcmtpHeader& header,
                    const char* const  VcmtpPacketData);
    bool rmMisBOPinList(uint32_t prodindex);
    bool addUnrqBOPinList(uint32_t prodindex);
    void mcastEOPHandler(const VcmtpHeader& header);
    void retxEOPHandler(const VcmtpHeader& header);
    void EOPHandler(const VcmtpHeader& header);
    /**
     * Handles a multicast BOP message given a peeked-at VCMTP header.
     *
     * @pre                           The multicast socket contains a VCMTP BOP
     *                                packet.
     * @param[in] header              The associated, already-decoded VCMTP header.
     * @throw     std::system_error   if an error occurs while reading the socket.
     * @throw     std::runtime_error  if the packet is invalid.
     */
    void BOPHandler(const VcmtpHeader& header);
    /**
     * Reads the data portion of a VCMTP data-packet into the location specified
     * by the receiving application.
     *
     * @pre                       The socket contains a VCMTP data-packet.
     * @param[in] header          The associated, peeked-at and decoded header.
     * @throw std::system_error   if an error occurs while reading the multicast
     *                            socket.
     * @throw std::runtime_error  if the packet is invalid.
     */
    void readMcastData(const VcmtpHeader& header);
    /**
     * Pushes a request for a data-packet onto the retransmission-request queue.
     *
     * @param[in] prodindex  Index of the associated data-product.
     * @param[in] seqnum     Sequence number of the data-packet.
     * @param[in] datalen    Amount of data in bytes.
     */
    void pushMissingDataReq(const uint32_t prodindex, const uint32_t seqnum,
                            const uint16_t datalen);
    /**
     * Pushes a request for a BOP-packet onto the retransmission-request queue.
     *
     * @param[in] prodindex  Index of the associated data-product.
     */
    void pushMissingBopReq(const uint32_t prodindex);
    /**
     * Pushes a request for a EOP-packet onto the retransmission-request queue.
     *
     * @param[in] prodindex  Index of the associated data-product.
     */
    void pushMissingEopReq(const uint32_t prodindex);
    /**
     * Requests data-packets that lie between the last previously-received
     * data-packet of the current data-product and its most recently-received
     * data-packet.
     *
     * @param[in] seqnum  The most recently-received data-packet of the current
     *                    data-product.
     */
    void requestAnyMissingData(uint32_t mostRecent);
    /**
     * Requests BOP packets for data-products that come after the current
     * data-product up to and including a given data-product.
     *
     * @param[in] prodindex  Index of the last data-product whose BOP packet was
     *                       missed.
     */
    void requestMissingBops(const uint32_t prodindex);
    /**
     * Handles a multicast VCMTP data-packet given the associated peeked-at and
     * decoded VCMTP header. Directly store and check for missing blocks.
     *
     * @pre                       The socket contains a VCMTP data-packet.
     * @param[in] header          The associated, peeked-at and decoded header.
     * @throw std::system_error   if an error occurs while reading the socket.
     * @throw std::runtime_error  if the packet is invalid.
     */
    void recvMemData(const VcmtpHeader& header);
    /**
     * request EOP retx if EOP is not received yet and return true if
     * the request is sent out. Otherwise, return false.
     * */
    bool reqEOPifMiss(const uint32_t prodindex);

    bool  sendBOPRetxReq(uint32_t prodindex);
    bool  sendEOPRetxReq(uint32_t prodindex);
    bool  sendDataRetxReq(uint32_t prodindex, uint32_t seqnum,
                          uint16_t payloadlen);
    bool  sendRetxEnd(uint32_t prodindex);
    bool  hasLastBlock();
    void  startTimerThread();
    static void* runTimerThread(void* ptr);
    void  timerThread();

    void setEOPReceived();
    void clearEOPState();
    bool isEOPReceived();
    void taskExit(const std::exception&);
};

#endif /* VCMTP_RECEIVER_VCMTPRECVV3_H_ */
