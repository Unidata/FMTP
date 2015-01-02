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

#define NULL 0

/**
 * Constructs a sender instance and initializes a udpsend object pointer, a
 * tcpsend object pointer, a senderMetadata object pointer and set prodIndex to
 * 0 and make it maintained by sender itself.
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
    notifier(0)
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
 * @param[in] notifier        Sending application notifier.
 */
vcmtpSendv3::vcmtpSendv3(const char*                 tcpAddr,
                         const unsigned short        tcpPort,
                         const char*                 mcastAddr,
                         const unsigned short        mcastPort,
                         uint32_t                    initProdIndex,
                         SendingApplicationNotifier* notifier)
:
    udpsend(new UdpSend(mcastAddr,mcastPort)),
    tcpsend(new TcpSend(tcpAddr, tcpPort)),
    sendMeta(new senderMetadata()),
    prodIndex(initProdIndex),
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
 * Sends the BOP message to the receiver.
 *
 * @param[in] prodSize       The size of the product.
 * @param[in] metadata       Application-specific metadata to be sent before the
 *                           data. May be 0, in which case no metadata is sent.
 * @param[in] metaSize       Size of the metadata in bytes. Must be less than
 *                           or equals 1442. May be 0, in which case no metadata
 *                           is sent.
 */
void vcmtpSendv3::SendBOPMessage(uint32_t prodSize, void* metadata,
                                 unsigned metaSize)
{
    uint16_t maxMetaSize = metaSize > AVAIL_BOP_LEN ? AVAIL_BOP_LEN : metaSize;

    const int PACKET_SIZE = VCMTP_HEADER_LEN + maxMetaSize +
                            (VCMTP_DATA_LEN - AVAIL_BOP_LEN);
    unsigned char vcmtp_packet[PACKET_SIZE];
    /** points to the beginning of the vcmtp packet header */
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet;
    /** points to the beginning of the vcmtp packet payload */
    VcmtpBOPMessage*   vcmtp_data   = (VcmtpBOPMessage*) (vcmtp_packet +
                                                          VCMTP_HEADER_LEN);

    (void) memset(vcmtp_packet, 0, sizeof(vcmtp_packet));

    /** convert the variables from native to network binary representation */
    uint32_t prodindex   = htonl(prodIndex);
    /** for BOP sequence number is always zero */
    uint32_t seqNum      = htonl(0);
    uint16_t payLen      = htons(maxMetaSize + (VCMTP_DATA_LEN -
                                 AVAIL_BOP_LEN));
    uint16_t flags       = htons(VCMTP_BOP);
    uint32_t prodsize    = htonl(prodSize);
    uint16_t maxmetasize = htons(maxMetaSize);

   /** copy the vcmtp header content into header struct */
   memcpy(&vcmtp_header->prodindex,   &prodindex, 4);
   memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
   memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
   memcpy(&vcmtp_header->flags,       &flags,     2);

   /** copy the content of BOP into vcmtp packet payload */
   memcpy(&vcmtp_data->prodsize, &prodsize,             4);
   memcpy(&vcmtp_data->metasize, &maxmetasize,          2);
   memcpy(&vcmtp_data->metadata, metadata,        maxMetaSize);

   /** send the BOP message on multicast socket */
   if (udpsend->SendTo(vcmtp_packet, PACKET_SIZE) < 0)
       throw std::runtime_error("vcmtpSendv3::SendBOPMessage() SendTo error");
}


/**
 * Transfers a contiguous block of memory (without metadata).
 *
 * @param[in] data      Memory data to be sent.
 * @param[in] dataSize  Size of the memory data in bytes.
 * @return              Index of the product.
 */
uint32_t vcmtpSendv3::sendProduct(char* data, size_t dataSize)
{
    return sendProduct(data, dataSize, 0, 0);
}


/**
 * Transfers Application-specific metadata and a contiguous block of memory.
 *
 * @param[in] data      Memory data to be sent.
 * @param[in] dataSize  Size of the memory data in bytes.
 * @param[in] metadata  Application-specific metadata to be sent before the
 *                      data. May be 0, in which case no metadata is sent.
 * @param[in] metaSize  Size of the metadata in bytes. Must be less than or
 *                      equal 1442 bytes. May be 0, in which case no metadata
 *                      is sent.
 * @return              Index of the product.
 */
uint32_t vcmtpSendv3::sendProduct(char* data, size_t dataSize, char* metadata,
                                  unsigned metaSize)
{
    if (data == NULL)
        throw std::runtime_error("vcmtpSendv3::sendProduct() data pointer is NULL");
    if (dataSize > 0xFFFFFFFFu)
        throw std::runtime_error("vcmtpSendv3::sendProduct() dataSize out of range");
    if (metadata == NULL)
        metaSize = 0;
    /** creates a new RetxMetadata struct for this product */
    RetxMetadata* senderProdMeta = new RetxMetadata();
    if (senderProdMeta == NULL)
        throw std::runtime_error("vcmtpSendv3::sendProduct() create RetxMetadata error");

    /** update current prodindex in RetxMetadata */
    senderProdMeta->prodindex 	   = prodIndex;
    /** update current product length in RetxMetadata */
    senderProdMeta->prodLength     = dataSize;
    /** update current product pointer in RetxMetadata */
    senderProdMeta->dataprod_p     = (void*) data;
    /** get a full list of current connected sockets and add to unfinished set */
    list<int> currSockList = tcpsend->getConnSockList();
    list<int>::iterator it;
    for (it = currSockList.begin(); it != currSockList.end(); ++it)
    {
        senderProdMeta->unfinReceivers.insert(*it);
    }
    /** add current RetxMetadata into sendMetadata::indexMetaMap */
    sendMeta->addRetxMetadata(senderProdMeta);
    /** update multicast start time in RetxMetadata */
    senderProdMeta->mcastStartTime = clock();

    /** send out BOP message */
    SendBOPMessage(dataSize, metadata, metaSize);

    char vcmtpHeader[VCMTP_HEADER_LEN];
    VcmtpHeader* header = (VcmtpHeader*) vcmtpHeader;

    uint32_t prodindex = htonl(prodIndex);
    uint32_t seqNum    = 0;
    uint16_t payLen;
    uint16_t flags     = htons(VCMTP_MEM_DATA);

    size_t remained_size = dataSize;
    /** check if there is more data to send */
    while (remained_size > 0)
    {
        unsigned int data_size = remained_size < VCMTP_DATA_LEN ?
                                 remained_size : VCMTP_DATA_LEN;

        payLen = htons(data_size);
        seqNum = htonl(seqNum);

        memcpy(&header->prodindex,  &prodindex, 4);
        memcpy(&header->seqnum,     &seqNum,    4);
        memcpy(&header->payloadlen, &payLen,    2);
        memcpy(&header->flags,      &flags,     2);

        if(udpsend->SendData(vcmtpHeader, VCMTP_HEADER_LEN, data, data_size)
           < 0)
        {
            throw std::runtime_error("vcmtpSendv3::sendProduct::SendData() error");
        }

        remained_size -= data_size;
        /** move the data pointer to the beginning of the next block */
        data = (char*) data + data_size;
        seqNum += data_size;
    }
    /** send out EOP message */
    sendEOPMessage();

    /** get end time of multicasting for measuring product transmit time */
    senderProdMeta->mcastEndTime = clock();

    /** set up timer timeout period */
    senderProdMeta->retxTimeoutPeriod = 8.5;

    /** start a new timer for this product in a separate thread */
    startTimerThread(prodIndex);

    return prodIndex++;
}


/**
 * Sends the EOP message to the receiver to indicate the end of a product
 * transmission.
 *
 * @param[in] none
 */
void vcmtpSendv3::sendEOPMessage()
{
    char vcmtp_packet[VCMTP_HEADER_LEN];
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet;
    (void) memset(vcmtp_packet, 0, sizeof(vcmtp_packet));

    uint32_t prodindex = htonl(prodIndex);
    /** seqNum for the EOP should always be zero */
    uint32_t seqNum    = htonl(0);
    /** payload for the EOP should always be zero */
    uint16_t payLen    = htons(0);
    uint16_t flags     = htons(VCMTP_EOP);
    /** copy the content of the vcmtp header into vcmtp struct */
    memcpy(&vcmtp_header->prodindex,   &prodindex, 4);
    memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
    memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
    memcpy(&vcmtp_header->flags,       &flags,     2);

    /** send the EOP message out */
    if (udpsend->SendTo(vcmtp_packet, VCMTP_HEADER_LEN) < 0)
        throw std::runtime_error("vcmtpSendv3::sendEOPMessage::SendTo error");
}


/**
 * Starts the coordinator thread from this function. And passes a vcmtpSendv3
 * type pointer to the coordinator thread so that coordinator can have access
 * to all the resources inside this vcmtpSendv3 instance.
 *
 * @param[in] none
 */
void vcmtpSendv3::startCoordinator()
{
    pthread_t new_t;
    int retval = pthread_create(&new_t, NULL, &vcmtpSendv3::coordinator, this);
    if(retval != 0)
    {
        throw std::runtime_error("vcmtpSendv3::startCoordinator() pthread_create error");
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
 * @throws std::system_error  The port number cannot be obtained.
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
        throw std::runtime_error("vcmtpSendv3::StartNewRetxThread() error pthread_create");
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
 * @param[in] retxsockfd              retx socket associated with a receiver
 */
void vcmtpSendv3::RunRetxThread(int retxsockfd)
{
    VcmtpHeader* recvheader = new VcmtpHeader();
    VcmtpHeader* sendheader = new VcmtpHeader();

    while(1)
    {
        /** receive the message from tcp connection and parse the header */
        if (tcpsend->parseHeader(retxsockfd, recvheader) < 0)
            throw std::runtime_error("vcmtpSendv3::RunRetxThread() receive \
                                     header error");

        /** first try to retrieve the requested product */
        RetxMetadata* retxMeta = sendMeta->getMetadata(recvheader->prodindex);
        /** Handle a retransmission request */
        if (recvheader->flags & VCMTP_RETX_REQ)
        {
            /**
             * send retx_rej msg to receivers if the given condition is
             * satisfied: the per-product timer thread has removed the prodindex.
             */
            if (retxMeta == NULL)
            {
                sendheader->prodindex  = htonl(recvheader->prodindex);
                sendheader->seqnum     = 0;
                sendheader->payloadlen = 0;
                sendheader->flags      = htons(VCMTP_RETX_REJ);
                tcpsend->send(retxsockfd, sendheader, NULL, 0);
            }
            else
            {
                /** get the pointer to the data product in product queue */
                void* prodptr = retxMeta->dataprod_p;
                /** if failed to fetch prodptr, throw an error and skip */
                if (prodptr == NULL)
                {
                    throw std::runtime_error("vcmtpSendv3::RunRetxThread() error retrieving prodptr");
                }

                /** construct the retx data block and send. */
                uint16_t remainedSize  = recvheader->payloadlen;
                uint32_t startPos      = recvheader->seqnum;
                sendheader->prodindex  = htonl(recvheader->prodindex);
                sendheader->flags      = htons(VCMTP_RETX_DATA);

                /**
                 * support for future requirement of sending multiple blocks in
                 * one single shot.
                 */
                while (remainedSize > 0)
                {
                    uint16_t payLen = remainedSize > VCMTP_DATA_LEN ?
                                      VCMTP_DATA_LEN : remainedSize;
                    sendheader->seqnum     = htonl(startPos);
                    sendheader->payloadlen = htons(payLen);
                    tcpsend->send(retxsockfd, sendheader,
                                  (char*)(prodptr) + startPos, (size_t) payLen);
                    startPos += payLen;
                    remainedSize -= payLen;
                }
            }
        }
        else if (recvheader->flags & VCMTP_RETX_END)
        {
            /** if timer is still sleeping, update metadata. Otherwise, skip */
            if (retxMeta != NULL)
            {
                /**
                 * remove the specific receiver out of the unfinished receiver
                 * set. Only if the product is removed by clearUnfinishedSet(),
                 * it returns a true value.
                 * */
                bool prodRemoved = sendMeta->clearUnfinishedSet(
                                   recvheader->prodindex, retxsockfd);
                /**
                 * Only if the product is removed by clearUnfinishedSet()
                 * since this receiver is the last one in the unfinished set,
                 * notify the sending application.
                 */
                if(notifier && prodRemoved)
                {
                    notifier->notify_of_eop(recvheader->prodindex);
                }
            }
        }
    }

    delete recvheader;
    delete sendheader;
}


/**
 * The function for starting a timer thread. It fills the prodindex and the
 * senderMetadata into the StartTimerThreadInfo structure and passes the
 * pointer of this structure to runTimerThread().
 *
 * @param[in] prodindex        product index the timer is supervising on.
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
        throw std::runtime_error("vcmtpSendv3::startTimerThread() \
                                 pthread_create error");
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
    SendingApplicationNotifier* const notifier = sender->notifier;
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
