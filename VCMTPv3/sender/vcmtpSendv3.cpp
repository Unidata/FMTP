/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpSendv3.cpp
 * @author    Fatma Alali <fha6np@virginia.edu>
 *            Shawn Chen  <sc7cq@virginia.edu>
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
 * @brief     Define the entity of VCMTPv3 sender side method function.
 *
 * Sender side of VCMTPv3 protocol. It multicasts packets out to multiple
 * receivers and retransmits missing blocks to the receivers.
 */


#include "vcmtpSendv3.h"
#include <math.h>
#include <stdio.h>
#include <stdexcept>
#include <string.h>
#include <system_error>

#ifndef NULL
    #define NULL 0
#endif

using namespace std;

/**
 * Constructs a sender instance and initializes a udpsend object pointer, a
 * tcpsend object pointer, a senderMetadata object pointer and set prodIndex to
 * 0 and make it maintained by sender itself. retxTimeoutRatio will be
 * initialized to 50.0 as a default value.
 *
 * @param[in] tcpAddr       Unicast address of the sender.
 * @param[in] tcpPort       Unicast port of the sender.
 * @param[in] mcastAddr     Multicast group address.
 * @param[in] mcastPort     Multicast group port.
 */
vcmtpSendv3::vcmtpSendv3(const char*          tcpAddr,
                         const unsigned short tcpPort,
                         const char*          mcastAddr,
                         const unsigned short mcastPort)
:
    udpsend(new UdpSend(mcastAddr,mcastPort)),
    tcpsend(new TcpSend(tcpAddr, tcpPort)),
    sendMeta(new senderMetadata()),
    prodIndex(0),
    retxTimeoutRatio(50.0),
    notifier(0)
{
}


/**
 * Constructs a sender instance with prodIndex specified and initialized by
 * receiving applications. VCMTP sender will start from this given prodindex.
 * retxTimeoutRatio will be initialized to 50.0 as a default value.
 *
 * @param[in] tcpAddr         Unicast address of the sender.
 * @param[in] tcpPort         Unicast port of the sender or 0, in which case one
 *                            is chosen by the operating-system.
 * @param[in] mcastAddr       Multicast group address.
 * @param[in] mcastPort       Multicast group port.
 * @param[in] initProdIndex   Initial prodIndex set by receiving applications.
 * @param[in] notifier        Sending application notifier.
 */
vcmtpSendv3::vcmtpSendv3(const char*                 tcpAddr,
                         const unsigned short        tcpPort,
                         const char*                 mcastAddr,
                         const unsigned short        mcastPort,
                         uint32_t                    initProdIndex,
                         SendAppNotifier* notifier)
:
    udpsend(new UdpSend(mcastAddr,mcastPort)),
    tcpsend(new TcpSend(tcpAddr, tcpPort)),
    sendMeta(new senderMetadata()),
    prodIndex(initProdIndex),
    retxTimeoutRatio(50.0),
    notifier(notifier)
{
}


/**
 * Constructs a sender instance with prodIndex specified and initialized by
 * receiving applications. VCMTP sender will start from this given prodindex.
 *
 * @param[in] tcpAddr         Unicast address of the sender.
 * @param[in] tcpPort         Unicast port of the sender or 0, in which case one
 *                            is chosen by the operating-system.
 * @param[in] mcastAddr       Multicast group address.
 * @param[in] mcastPort       Multicast group port.
 * @param[in] initProdIndex   Initial prodIndex set by receiving applications.
 * @param[in] timeoutRatio    retranmission timeout factor to tradeoff between
 *                            performance and robustness.
 * @param[in] notifier        Sending application notifier.
 */
vcmtpSendv3::vcmtpSendv3(const char*                 tcpAddr,
                         const unsigned short        tcpPort,
                         const char*                 mcastAddr,
                         const unsigned short        mcastPort,
                         uint32_t                    initProdIndex,
                         float                       timeoutRatio,
                         SendAppNotifier* notifier)
:
    udpsend(new UdpSend(mcastAddr,mcastPort)),
    tcpsend(new TcpSend(tcpAddr, tcpPort)),
    sendMeta(new senderMetadata()),
    prodIndex(initProdIndex),
    retxTimeoutRatio(timeoutRatio),
    notifier(notifier)
{
}


/**
 * Deconstructs the sender instance and release the initialized resources.
 *
 * @param[in] none
 */
vcmtpSendv3::~vcmtpSendv3()
{
    delete udpsend;
    delete tcpsend;
    delete sendMeta;
}


/**
 * Sends the BOP message to the receiver. metadata and metaSize must always be
 * a valid value. These two parameters will be checked by the calling function
 * before being passed in.
 *
 * @param[in] prodSize       The size of the product.
 * @param[in] metadata       Application-specific metadata to be sent before the
 *                           data. May be 0, in which case no metadata is sent.
 * @param[in] metaSize       Size of the metadata in bytes. May be 0, in which
 *                           case no metadata is sent.
 * @throw std::runtime_error if the UdpSend::SendTo() fails.
 */
void vcmtpSendv3::SendBOPMessage(uint32_t prodSize, void* metadata,
                                 const unsigned metaSize)
{
    VcmtpHeader   header;   // in network byte order
    BOPMsg        bopMsg;   // in network byte order
    struct iovec  ioVec[3]; // gather-send used to eliminate copying

    /* Set the VCMTP packet header. */
    header.prodindex  = htonl(prodIndex);
    header.seqnum     = 0;
    header.payloadlen = htons(metaSize + (VCMTP_DATA_LEN - AVAIL_BOP_LEN));
    header.flags      = htons(VCMTP_BOP);
    ioVec[0].iov_base = &header;
    ioVec[0].iov_len  = sizeof(VcmtpHeader);

    /* Set the VCMTP BOP message. */
    bopMsg.prodsize = htonl(prodSize);
    bopMsg.metasize = htons(metaSize);
    ioVec[1].iov_base = &bopMsg;
    /* The metadata is referenced in a separate I/O vector. */
    ioVec[1].iov_len  = sizeof(bopMsg) - sizeof(bopMsg.metadata);

    /* Reference the metadata for the gather send. */
    ioVec[2].iov_base = metadata; // might be 0
    ioVec[2].iov_len  = metaSize; // will be 0 if `metadata == 0`

    /* Send the BOP message on multicast socket */
    if (udpsend->SendTo(ioVec, 3) < 0)
        throw std::runtime_error(
                "vcmtpSendv3::SendBOPMessage(): SendTo() error");
}


/**
 * Transfers a contiguous block of memory (without metadata).
 *
 * @param[in] data      Memory data to be sent.
 * @param[in] dataSize  Size of the memory data in bytes.
 * @return              Index of the product.
 * @throws std::invalid_argument  if `data == 0`.
 * @throws std::invalid_argument  if `dataSize` exceeds the maximum allowed
 *                                value.
 * @throws std::runtime_error     if retrieving sender side RetxMetadata fails.
 * @throws std::runtime_error     if UdpSend::SendData() fails.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, size_t dataSize)
{
    // TODO: need an accurate model to give a default timeout ratio. Though
    // default value has been fixed to 50.0. It's still capable to update the
    // value again here.
    return sendProduct(data, dataSize, 0, 0);
}

/**
 * Adds and entry for a data-product to the retransmission set.
 *
 * @param[in] data      The data-product.
 * @param[in] dataSize  The size of the data-product in bytes.
 * @return              The corresponding retransmission entry.
 * @throw std::runtime_error  if a retransmission entry couldn't be created.
 */
RetxMetadata* vcmtpSendv3::addRetxMetadata(
        void* const data,
        const size_t dataSize)
{
    /* Create a new RetxMetadata struct for this product */
    RetxMetadata* senderProdMeta = new RetxMetadata();
    if (senderProdMeta == NULL)
        throw std::runtime_error(
                "vcmtpSendv3::addRetxMetadata(): create RetxMetadata error");

    /* Update current prodindex in RetxMetadata */
    senderProdMeta->prodindex        = prodIndex;

    /* Update current product length in RetxMetadata */
    senderProdMeta->prodLength       = dataSize;

    /* Update current product pointer in RetxMetadata */
    senderProdMeta->dataprod_p       = (void*) data;

    /* Update the per-product timeout ratio */
    senderProdMeta->retxTimeoutRatio = retxTimeoutRatio;

    /* Get a full list of current connected sockets and add to unfinished set */
    list<int> currSockList = tcpsend->getConnSockList();
    list<int>::iterator it;
    for (it = currSockList.begin(); it != currSockList.end(); ++it)
        senderProdMeta->unfinReceivers.insert(*it);

    /* Add current RetxMetadata into sendMetadata::indexMetaMap */
    sendMeta->addRetxMetadata(senderProdMeta);

    /* Update multicast start time in RetxMetadata */
    senderProdMeta->mcastStartTime = clock();

    return senderProdMeta;
}

/**
 * Multicasts the data of a data-product.
 *
 * @param[in] data      The data-product.
 * @param[in] dataSize  The size of the data-product in bytes.
 * @throw std::runtime_error  if an I/O error occurs.
 */
void vcmtpSendv3::sendData(
        void*  data,
        size_t dataSize)
{
    VcmtpHeader header;
    uint32_t    seqNum = 0;
    header.prodindex = htonl(prodIndex);
    header.flags     = htons(VCMTP_MEM_DATA);

    /* check if there is more data to send */
    while (dataSize > 0)
    {
        unsigned int payloadlen = dataSize < VCMTP_DATA_LEN ?
                                  dataSize : VCMTP_DATA_LEN;

        header.seqnum     = htons(seqNum);
        header.payloadlen = htons(payloadlen);

        if(udpsend->SendData(&header, sizeof(header), data, payloadlen) < 0)
            throw std::runtime_error("vcmtpSendv3::sendProduct::SendData() error");

        dataSize -= payloadlen;
        data      = (char*)data + payloadlen;
        seqNum   += payloadlen;
    }
}

/**
 * Sets the retransmission timeout parameters in a retransmission entry.
 *
 * @param[in] senderProdMeta  The retransmission entry.
 */
void vcmtpSendv3::setTimerParameters(
    RetxMetadata* const senderProdMeta)
{
    /* Get end time of multicasting for measuring product transmit time */
    senderProdMeta->mcastEndTime = clock();

    /* Cast clock_t type value into float type seconds */
    float mcastPeriod = ((float) (senderProdMeta->mcastEndTime -
                        senderProdMeta->mcastStartTime)) / CLOCKS_PER_SEC;

    /* Set up timer timeout period */
    senderProdMeta->retxTimeoutPeriod = mcastPeriod *
                                        senderProdMeta->retxTimeoutRatio;
}

/**
 * Transfers Application-specific metadata and a contiguous block of memory.
 * Construct sender side RetxMetadata and insert the new entry into a global
 * map. The retransmission timeout period should also be set by considering
 * the essential properties.
 *
 * @param[in] data         Memory data to be sent.
 * @param[in] dataSize     Size of the memory data in bytes.
 * @param[in] metadata     Application-specific metadata to be sent before the
 *                         data. May be 0, in which case `metaSize` must be 0
 *                         and no metadata is sent.
 * @param[in] metaSize     Size of the metadata in bytes. Must be less than or
 *                         equal 1442 bytes. May be 0, in which case no
 *                         metadata is sent.
 * @param[in] perProdTimeoutRatio
 *                         the per-product timeout ratio to balance performance
 *                         and robustness (reliability).
 * @return                 Index of the product.
 * @throws std::invalid_argument  if `data == 0`.
 * @throws std::invalid_argument  if `dataSize` exceeds the maximum allowed
 *                                value.
 * @throw std::runtime_error      if a retransmission entry couldn't be created.
 * @throws std::runtime_error     if UdpSend::SendData() fails.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, size_t dataSize, void* metadata,
                                  unsigned metaSize)
{
    if (data == NULL)
        throw std::invalid_argument("vcmtpSendv3::sendProduct() data pointer is NULL");
    if (dataSize > 0xFFFFFFFFu)
        throw std::invalid_argument("vcmtpSendv3::sendProduct() dataSize out of range");
    if (metadata) {
        if (AVAIL_BOP_LEN < metaSize)
            throw std::invalid_argument(
                    "vcmtpSendv3::SendBOPMessage(): metaSize too large");
    }
    else {
        if (metaSize)
            throw std::invalid_argument(
                    "vcmtpSendv3::SendBOPMessage(): Non-zero metaSize");
    }

    /* Add a retransmission entry */
    RetxMetadata* senderProdMeta = addRetxMetadata(data, dataSize);

    /* send out BOP message */
    SendBOPMessage(dataSize, metadata, metaSize);

    /* Send the data */
    sendData(data, dataSize);

    /* Send out EOP message */
    sendEOPMessage();

    /* Set the retransmission timeout parameters */
    setTimerParameters(senderProdMeta);

    /* start a new timer for this product in a separate thread */
    startTimerThread(prodIndex);

    return prodIndex++;
}


/**
 * Sends the EOP message to the receiver to indicate the end of a product
 * transmission.
 *
 * @param[in]               none
 * @throw  runtime_error    if UdpSend::SendTo() fails.
 */
void vcmtpSendv3::sendEOPMessage()
{
    VcmtpHeader header;

    header.prodindex  = htonl(prodIndex);
    header.seqnum     = 0;
    header.payloadlen = 0;
    header.flags      = htons(VCMTP_EOP);

    if (udpsend->SendTo((char*)&header, sizeof(header)) < 0)
        throw std::runtime_error("vcmtpSendv3::sendEOPMessage::SendTo error");
}


/**
 * Starts the coordinator thread from this function. And passes a vcmtpSendv3
 * type pointer to the coordinator thread so that coordinator can have access
 * to all the resources inside this vcmtpSendv3 instance.
 *
 * @throw  std::system_error  if pthread_create() fails.
 */
void vcmtpSendv3::startCoordinator()
{
    pthread_t new_t;
    int retval = pthread_create(&new_t, NULL, &vcmtpSendv3::coordinator, this);
    if(retval != 0)
    {
        throw std::system_error(retval, std::system_category(),
                "vcmtpSendv3::startCoordinator() pthread_create() error");
    }
    pthread_detach(new_t);
}


/**
 * The sender side coordinator thread. Listen for incoming TCP connection
 * requests in an infinite loop and assign a new socket for the corresponding
 * receiver. Then pass that new socket as a parameter to start a receiver-
 * specific thread.
 *
 * @param[in] *ptr    void type pointer that points to whatever data structure.
 * @return            void type pointer that ponits to whatever return value.
 */
void* vcmtpSendv3::coordinator(void* ptr)
{
    vcmtpSendv3* sendptr = static_cast<vcmtpSendv3*>(ptr);
    while(1)
    {
        int newtcpsockfd = sendptr->tcpsend->acceptConn();
        sendptr->StartNewRetxThread(newtcpsockfd);
    }
    return NULL;
}


/**
 * Return the local port number.
 *
 * @return                    The local port number in host byte-order.
 */
unsigned short vcmtpSendv3::getTcpPortNum()
{
    return tcpsend->getPortNum();
}


/**
 * Create all the necessary information and fill into the StartRetxThreadInfo
 * structure. Pass the pointer of this struture as a set of parameters to the
 * new thread.
 *
 * @param[in] newtcpsockfd
 * @throw     std::system_error  if pthread_create() fails.
 */
void vcmtpSendv3::StartNewRetxThread(int newtcpsockfd)
{
    pthread_t t;
    retxSockThreadMap[newtcpsockfd] = &t;
    retxSockFinishMap[newtcpsockfd] = false;

    StartRetxThreadInfo* retxThreadInfo = new StartRetxThreadInfo();
    retxThreadInfo->retxmitterptr       = this;
    retxThreadInfo->retxsockfd          = newtcpsockfd;

    retxSockInfoMap[newtcpsockfd] = retxThreadInfo;

    int retval = pthread_create(&t, NULL, &vcmtpSendv3::StartRetxThread,
                                retxThreadInfo);
    if(retval != 0)
    {
        throw std::system_error(retval, std::system_category(),
                "vcmtpSendv3::StartNewRetxThread() error pthread_create()");
    }
    pthread_detach(t);
}


/**
 * Use the passed-in pointer to extract parameters. The first pointer as a
 * pointer of vcmtpSendv3 instance can start vcmtpSendv3 member function.
 * The second parameter is sockfd, the third one is the prodindex-prodptr map.
 *
 * @param[in] *ptr    a void type pointer that points to whatever data struct.
 */
void* vcmtpSendv3::StartRetxThread(void* ptr)
{
    StartRetxThreadInfo* newptr = static_cast<StartRetxThreadInfo*>(ptr);
    newptr->retxmitterptr->RunRetxThread(newptr->retxsockfd);
    return NULL;
}

/**
 * Rejects a retransmission request from a receiver.
 *
 * @param[in] prodindex  Product-index of the request.
 * @param[in] sock       The receiver's socket.
 */
void vcmtpSendv3::rejRetxReq(
        const uint32_t prodindex,
        const int      sock)
{
    VcmtpHeader sendheader;

    sendheader.prodindex  = htonl(prodindex);
    sendheader.seqnum     = 0;
    sendheader.payloadlen = 0;
    sendheader.flags      = htons(VCMTP_RETX_REJ);
    tcpsend->send(sock, &sendheader, NULL, 0);
}

/**
 * Retransmits data to a receiver.
 *
 * @param[in] recvheader  The VCMTP header of the retransmission request.
 * @param[in] retxMeta    The associated retransmission entry.
 * @param[in] sock        The receiver's socket.
 */
void vcmtpSendv3::retransmit(
        VcmtpHeader* const  recvheader,
        RetxMetadata* const retxMeta,
        const int           sock)
{
    VcmtpHeader sendheader;

    /* Construct the retx data block and send. */
    uint16_t remainedSize = recvheader->payloadlen;
    uint32_t startPos     = recvheader->seqnum;
    sendheader.prodindex  = htonl(recvheader->prodindex);
    sendheader.flags      = htons(VCMTP_RETX_DATA);

    /*
     * Support for future requirement of sending multiple blocks in
     * one single shot.
     */
    while (remainedSize > 0)
    {
        uint16_t payLen = remainedSize > VCMTP_DATA_LEN ?
                          VCMTP_DATA_LEN : remainedSize;
        sendheader.seqnum     = htonl(startPos);
        sendheader.payloadlen = htons(payLen);
        tcpsend->send(sock, &sendheader, (char*)retxMeta->dataprod_p + startPos,
                payLen);
        startPos     += payLen;
        remainedSize -= payLen;
    }
}

/**
 * Handles a retransmission request from a receiver.
 *
 * @param[in] recvheader  VCMTP header of the retransmission request.
 * @param[in] retxMeta    Associated retransmission entry or `0`, in which case
 *                        the request will be rejected.
 * @param[in] sock        The receiver's socket.
 */
void vcmtpSendv3::handleRetxReq(
        VcmtpHeader* const  recvheader,
        RetxMetadata* const retxMeta,
        const int           sock)
{
    if (retxMeta) {
        retransmit(recvheader, retxMeta, sock);
    }
    else {
        /*
         * Reject the request because the retransmission entry was removed by
         * the per-product timer thread.
         */
        rejRetxReq(recvheader->prodindex, sock);
    }
}

/**
 * Handles a notice from a receiver that a data-product has been completely
 * received.
 *
 * @param[in] recvheader  The VCMTP header of the notice.
 * @param[in] retxMeta    Associated retransmission entry or `0`, in which case
 *                        nothing is done.
 * @param[in] sock        The receiver's socket.
 */
void vcmtpSendv3::handleRetxEnd(
        VcmtpHeader* const  recvheader,
        RetxMetadata* const retxMeta,
        const int           sock)
{
    if (retxMeta) {
        /*
         * Remove the specific receiver out of the unfinished receiver
         * set. Only if the product is removed by clearUnfinishedSet(),
         * it returns a true value.
         */
        if (sendMeta->clearUnfinishedSet(recvheader->prodindex, sock)) {
            /*
             * Only if the product is removed by clearUnfinishedSet()
             * since this receiver is the last one in the unfinished set,
             * notify the sending application.
             */
            if (notifier)
                notifier->notify_of_eop(recvheader->prodindex);
        }
    }
}

/**
 * The actual retransmission handling thread. Each thread listens on a receiver
 * specific socket which is given by retxsockfd. It receives the RETX_REQ or
 * RETX_END message and either issues a RETX_REJ or retransmits the data block.
 * There is only one piece of globally shared senderMetadata structure, which
 * holds a prodindex to RetxMetadata map. Using the given prodindex can find
 * an associated RetxMetadata. Inside that RetxMetadata, there is the unfinished
 * receivers set and timeout value. After the sendProduct() finishing multicast
 * and initializing the RetxMetadata entry, the metadata of that product will
 * be inserted into the senderMetadata map. If the metadata of a requested
 * product can be found, this retx thread will fetch the prodptr and extract
 * the requested data block and pack up to send. Otherwise, retx thread will
 * get a NULL pointer, which indicates that the timer has waken up and removed
 * that metadata out of the map. Thus, retx thread will issue a RETX_REJ to
 * send back to the receiver.
 *
 * @param[in] retxsockfd          retx socket associated with a receiver.
 * @throw  runtime_error          if TcpSend::parseHeader() fails.
 */
void vcmtpSendv3::RunRetxThread(int retxsockfd)
{
    VcmtpHeader recvheader;

    while(1) {
        /* Receive the message from tcp connection and parse the header */
        if (tcpsend->parseHeader(retxsockfd, &recvheader) < 0)
            throw std::runtime_error("vcmtpSendv3::RunRetxThread() receive "
                                     "header error");

        /* Retrieve the retransmission entry of the requested product */
        RetxMetadata* retxMeta = sendMeta->getMetadata(recvheader.prodindex);

        if (recvheader.flags & VCMTP_RETX_REQ) {
            handleRetxReq(&recvheader, retxMeta, retxsockfd);
        }
        else if ((recvheader.flags & VCMTP_RETX_END)) {
            handleRetxEnd(&recvheader, retxMeta, retxsockfd);
        }
    }
}


/**
 * The function for starting a timer thread. It fills the prodindex and the
 * senderMetadata into the StartTimerThreadInfo structure and passes the
 * pointer of this structure to runTimerThread().
 *
 * @param[in] prodindex          product index the timer is supervising on.
 * @throw     std::system_error  if pthread_create() fails.
 */
void vcmtpSendv3::startTimerThread(uint32_t prodindex)
{
    pthread_t t;
    StartTimerThreadInfo* timerinfo = new StartTimerThreadInfo();
    timerinfo->prodindex = prodindex;
    timerinfo->sender = this;
    int retval = pthread_create(&t, NULL, &vcmtpSendv3::runTimerThread,
                                timerinfo);
    if(retval != 0)
    {
        throw std::system_error(retval, std::system_category(),
                "vcmtpSendv3::startTimerThread() pthread_create() error");
    }
    pthread_detach(t);
}


/**
 * The per-product timer. A timer will be created to sleep for a given period
 * of time, which is specified in the RetxMetadata structure. When the timer
 * wakes up from sleeping, it will check and remove the corresponding product
 * from the prodindex-retxmetadata map.
 *
 * @param[in] *ptr          pointer to the StartTimerThreadInfo structure
 */
void* vcmtpSendv3::runTimerThread(void* ptr)
{
    const StartTimerThreadInfo* const timerInfo =
            static_cast<StartTimerThreadInfo*>(ptr);
    const uint32_t                    prodIndex = timerInfo->prodindex;
    vcmtpSendv3* const                sender = timerInfo->sender;
    SendAppNotifier* const notifier = sender->notifier;
    senderMetadata* const             sendMeta = sender->sendMeta;
    const RetxMetadata* const         perProdMeta =
            sendMeta->getMetadata(prodIndex);

    if (perProdMeta != NULL)
    {
        float           seconds;
        /** parse the float type timeout value into seconds and fractions */
        const float     fraction = modff(perProdMeta->retxTimeoutPeriod,
                                         &seconds);
        struct timespec timespec;

        timespec.tv_sec = seconds;
        timespec.tv_nsec = fraction * 1e9f;
        /** sleep for a given amount of seconds and nanoseconds */
        (void) nanosleep(&timespec, 0);

        const bool wasRemoved = sendMeta->rmRetxMetadata(prodIndex);

        /**
         * Only if the product is removed by this remove call, notify the
         * sending application
         */
        if (notifier && wasRemoved)
            notifier->notify_of_eop(prodIndex);
    }

    delete timerInfo;
    return NULL;
}
