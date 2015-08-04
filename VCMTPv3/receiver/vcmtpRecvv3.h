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
 * @brief     Define the interfaces of VCMTPv3 receiver.
 *
 * Receiver side of VCMTPv3 protocol. It handles incoming multicast packets
 * and issues retransmission requests to the sender side.
 */


#ifndef VCMTP_RECEIVER_VCMTPRECVV3_H_
#define VCMTP_RECEIVER_VCMTPRECVV3_H_


#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

#include "Measure.h"
#include "ProdBlockMNG.h"
#include "RecvAppNotifier.h"
#include "TcpRecv.h"
#include "vcmtpBase.h"


class vcmtpRecvv3;

struct StartTimerInfo
{
    uint32_t     prodindex;  /*!< product index */
    float        seconds;    /*!< product index */
    vcmtpRecvv3* receiver;   /*!< a poniter to the vcmtpRecvv3 instance */
};

struct ProdTracker
{
    uint32_t     prodsize;
    void*        prodptr;
    uint32_t     seqnum;
    uint16_t     paylen;
};

typedef std::unordered_map<uint32_t, ProdTracker> TrackerMap;
typedef std::unordered_map<uint32_t, bool> EOPStatusMap;


class vcmtpRecvv3 {
public:
    vcmtpRecvv3(const std::string    tcpAddr,
                const unsigned short tcpPort,
                const std::string    mcastAddr,
                const unsigned short mcastPort,
                RecvAppNotifier*     notifier = NULL,
                const std::string    ifAddr = "0.0.0.0");
    ~vcmtpRecvv3();

    uint32_t getNotify();
    void SetLinkSpeed(uint64_t speed);
    void Start();
    void Stop();

private:
    bool addUnrqBOPinList(uint32_t prodindex);
    /**
     * Parse BOP message and call notifier to notify receiving application.
     *
     * @param[in] header           Header associated with the packet.
     * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
     * @throw std::runtime_error   if the payload is too small.
     */
    void BOPHandler(const VcmtpHeader& header,
                    const char* const  VcmtpPacketData);
    void checkPayloadLen(const VcmtpHeader& header, const size_t nbytes);
    void clearEOPStatus(const uint32_t prodindex);
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
    void EOPHandler(const VcmtpHeader& header);
    bool getEOPStatus(const uint32_t prodindex);
    bool hasLastBlock(const uint32_t prodindex);
    void initEOPStatus(const uint32_t prodindex);
    void joinGroup(std::string mcastAddr, const unsigned short mcastPort);
    /**
     * Handles a multicast BOP message given a peeked-at VCMTP header.
     *
     * @pre                           The multicast socket contains a VCMTP BOP
     *                                packet.
     * @param[in] header              The associated, already-decoded VCMTP header.
     * @throw     std::system_error   if an error occurs while reading the socket.
     * @throw     std::runtime_error  if the packet is invalid.
     */
    void mcastBOPHandler(const VcmtpHeader& header);
    void mcastHandler();
    void mcastEOPHandler(const VcmtpHeader& header);
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
    void retxHandler();
    void retxRequester();
    bool rmMisBOPinList(uint32_t prodindex);
    /**
     * Handles a retransmitted BOP message.
     *
     * @param[in] header           Header associated with the packet.
     * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
     */
    void retxBOPHandler(const VcmtpHeader& header,
                        const char* const  VcmtpPacketData);
    void retxEOPHandler(const VcmtpHeader& header);
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
     * Requests data-packets that lie between the last previously-received
     * data-packet of the current data-product and its most recently-received
     * data-packet.
     *
     * @param[in] prodindex Product index.
     * @param[in] seqnum  The most recently-received data-packet of the current
     *                    data-product.
     */
    void requestAnyMissingData(const uint32_t prodindex,
                               const uint32_t mostRecent);
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
    static void* runTimerThread(void* ptr);
    bool sendBOPRetxReq(uint32_t prodindex);
    bool sendEOPRetxReq(uint32_t prodindex);
    bool sendDataRetxReq(uint32_t prodindex, uint32_t seqnum,
                         uint16_t payloadlen);
    bool sendRetxEnd(uint32_t prodindex);
    static void*  StartRetxRequester(void* ptr);
    static void*  StartRetxHandler(void* ptr);
    static void*  StartMcastHandler(void* ptr);
    void StartRetxProcedure();
    void startTimerThread();
    void setEOPStatus(const uint32_t prodindex);
    void timerThread();
    void taskExit(const std::exception&);
    void WriteToLog(const std::string& content);
    void stopJoinRetxRequester();
    void stopJoinRetxHandler();
    void stopJoinTimerThread();
    void stopJoinMcastHandler();

    std::string             tcpAddr;
    unsigned short          tcpPort;
    std::string             mcastAddr;
    unsigned short          mcastPort;
    /* IP address of the default interface */
    std::string             ifAddr;
    int                     mcastSock;
    int                     retxSock;
    struct sockaddr_in      mcastgroup;
    /* struct of multicast object */
    struct ip_mreq          mreq;
    /* the most recently received product index on multicast */
    uint32_t                prodidx_mcast;
    /* mutex for updating multicast product index */
    std::mutex              pidxmtx;
    /* callback function of the receiving application */
    RecvAppNotifier*        notifier;
    TcpRecv*                tcprecv;
    /* a map from prodindex to struct ProdTracker */
    TrackerMap              trackermap;
    std::mutex              trackermtx;
    /* eliminate race conditions between mcast and retx */
    std::mutex              antiracemtx;
    /* a map from prodindex to EOP arrival status */
    EOPStatusMap            EOPmap;
    std::mutex              EOPmapmtx;
    ProdBlockMNG*           pBlockMNG;
    std::queue<INLReqMsg>   msgqueue;
    std::condition_variable msgQfilled;
    std::mutex              msgQmutex;
    /* track all the missing BOP until received */
    std::list<uint32_t>     misBOPlist;
    std::mutex              BOPListMutex;
    /* Retransmission request thread */
    pthread_t               retx_rq;
    /* Retransmission receive thread */
    pthread_t               retx_t;
    /* Multicast receiver thread */
    pthread_t               mcast_t;
    /* BOP timer thread */
    pthread_t               timer_t;
    /* a queue containing timerParam structure for each product */
    std::queue<timerParam>  timerParamQ;
    std::condition_variable timerQfilled;
    std::mutex              timerQmtx;
    std::condition_variable timerWake;
    std::mutex              timerWakemtx;
    std::mutex              exitMutex;
    std::condition_variable exitCond;
    bool                    stopRequested;
    std::exception_ptr      except;
    std::mutex              linkmtx;
    /* max link speed up to 18000 Pbps */
    uint64_t                linkspeed;
    std::atomic_flag        retxHandlerCanceled;
    std::atomic_flag        mcastHandlerCanceled;
    std::mutex              notifyprodmtx;
    uint32_t                notifyprodidx;
    std::condition_variable notify_cv;

    /* member variables for measurement use only */
    Measure*                measure;
    /* member variables for measurement use ends */
};


#endif /* VCMTP_RECEIVER_VCMTPRECVV3_H_ */
