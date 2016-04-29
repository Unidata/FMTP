/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpSendv3.cpp
 * @author    Shawn Chen  <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 16, 2014
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
 * @brief     Define the entity of VCMTPv3 sender side method function.
 *
 * Sender side of VCMTPv3 protocol. It multicasts packets out to multiple
 * receivers and retransmits missing blocks to the receivers.
 */


#include "vcmtpSendv3.h"

#include <algorithm>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdexcept>
#include <system_error>



#ifndef NULL
    #define NULL 0
#endif
#ifndef MIN
    #define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif

#define DROPSEQ 0*VCMTP_DATA_LEN


/**
 * Constructs a sender instance with prodIndex specified and initialized by
 * receiving applications. VCMTP sender will start from this given prodindex.
 * Besides, timeoutratio for all the products will be passed in, which means
 * timeoutratio is only associated with a particular network. And TTL will
 * also be set to override the default value 1.
 *
 * @param[in] tcpAddr        Unicast address of the sender.
 * @param[in] tcpPort        Unicast port of the sender or 0, in which case one
 *                           is chosen by the operating-system.
 * @param[in] mcastAddr      Multicast group address.
 * @param[in] mcastPort      Multicast group port.
 * @param[in] notifier       Sending application notifier.
 * @param[in] ttl            Time to live, if not specified, default value is 1.
 * @param[in] ifAddr         IP address of the interface to be used to send
 *                           multicast packets. "0.0.0.0" obtains the default
 *                           multicast interface.
 * @param[in] initProdIndex  Initial prodIndex set by receiving applications.
 * @param[in] timeoutRatio   retransmission timeout factor to tradeoff between
 *                           performance and robustness.
 */
vcmtpSendv3::vcmtpSendv3(const char*                 tcpAddr,
                         const unsigned short        tcpPort,
                         const char*                 mcastAddr,
                         const unsigned short        mcastPort,
                         SendAppNotifier*            notifier,
                         const unsigned char         ttl,
                         const std::string           ifAddr,
                         const uint32_t              initProdIndex,
                         const float                 tsnd)
:
    udpsend(new UdpSend(mcastAddr, mcastPort, ttl, ifAddr)),
    tcpsend(new TcpSend(tcpAddr, tcpPort)),
    sendMeta(new senderMetadata()),
    notifier(notifier),
    prodIndex(initProdIndex),
    linkspeed(0),
    exitMutex(),
    except(),
    exceptIsSet(false),
    coor_t(),
    timer_t(),
    tsnd(tsnd),
    txdone(false)
{
}


/**
 * Destructs the sender instance and release the initialized resources.
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
 * Clears the prodset by a given range after each run is finished.
 * This is for the use of testapp only.
 *
 * @param[in] run    Run number just finished.
 */
void vcmtpSendv3::clearRuninProdSet(int run)
{
    suppressor->clearrange((uint32_t(run * PRODNUM)));
}


/**
 * Blocks until a product is acknowledged by all receivers, returns the product
 * index.
 * This is for the use of testapp only.
 *
 * @return   Product index of the most recent ACKed product.
 */
uint32_t vcmtpSendv3::getNotify()
{
    std::unique_lock<std::mutex> lock(notifycvmtx);
    notify_cv.wait(lock);
    return suppressor->query();
}


/**
 * Returns the local port number.
 *
 * @return                   The local port number in host byte-order.
 * @throw std::runtime_error  The port number cannot be obtained.
 */
unsigned short vcmtpSendv3::getTcpPortNum()
{
    return tcpsend->getPortNum();
}


/**
 * Blocks until a product is confirmed to be removed by the time.
 * This is for the use of testapp only.
 *
 * @return   Product index of the most recent ACKed product.
 */
uint32_t vcmtpSendv3::releaseMem()
{
    std::unique_lock<std::mutex> lock(notifyprodmtx);
    memrelease_cv.wait(lock);
    return notifyprodidx;
}


/**
 * Transfers a contiguous block of memory (without metadata).
 *
 * @param[in] data      Memory data to be sent.
 * @param[in] dataSize  Size of the memory data in bytes.
 * @return              Index of the product.
 * @throws std::runtime_error  if `data == 0`.
 * @throws std::runtime_error  if `dataSize` exceeds the maximum allowed
 *                                value.
 * @throws std::runtime_error     if retrieving sender side RetxMetadata fails.
 * @throws std::runtime_error     if UdpSend::SendData() fails.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, uint32_t dataSize)
{
    return sendProduct(data, dataSize, 0, 0);
}


/**
 * Transfers Application-specific metadata and a contiguous block of memory.
 * Construct sender side RetxMetadata and insert the new entry into a global
 * map. The retransmission timeout period should also be set by considering
 * the essential properties. If an exception is thrown inside this function,
 * it will be caught by the handler. As a result, the exception will cause
 * all the threads in this process to terminate. If any exception is thrown,
 * the Stop() will be effectively called.
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
 * @throws std::runtime_error  if `data == 0`.
 * @throws std::runtime_error  if `dataSize` exceeds the maximum allowed
 *                                value.
 * @throws std::runtime_error  if `metadata` != 0 and metaSize is too large
 * @throws std::runtime_error  if `metadata` == 0 and metaSize != 0
 * @throws std::runtime_error     if a runtime error occurs.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, uint32_t dataSize, void* metadata,
                                  uint16_t metaSize)
{
    try {
        if (data == NULL)
            throw std::runtime_error(
                    "vcmtpSendv3::sendProduct() data pointer is NULL");
        if (dataSize > 0xFFFFFFFFu)
            throw std::runtime_error(
                    "vcmtpSendv3::sendProduct() dataSize out of range");
        if (metadata) {
            if (AVAIL_BOP_LEN < metaSize)
                throw std::runtime_error(
                        "vcmtpSendv3::SendBOPMessage(): metaSize too large");
        }
        else {
            if (metaSize)
                throw std::runtime_error(
                        "vcmtpSendv3::SendBOPMessage(): Non-zero metaSize");
        }

        /* Add a retransmission metadata entry */
        RetxMetadata* senderProdMeta = addRetxMetadata(data, dataSize,
                                                       metadata, metaSize);
        /* send out BOP message */
        SendBOPMessage(dataSize, metadata, metaSize);
        /* Send the data */
        sendData(data, dataSize);
        /* Send out EOP message */
        sendEOPMessage();

        /* Set the retransmission timeout parameters */
        setTimerParameters(senderProdMeta);
        /* start a new timer for this product in a separate thread */
        timerDelayQ.push(prodIndex, senderProdMeta->retxTimeoutPeriod);
    }
    catch (std::runtime_error& e) {
        taskExit(e);
        std::rethrow_exception(except);
    }

#ifdef MODBASE
    uint32_t tmpidx = prodIndex % MODBASE;
#else
    uint32_t tmpidx = prodIndex;
#endif

#ifdef DEBUG1
    std::string debugmsg = "Product #" + std::to_string(tmpidx);
    debugmsg += " has been sent.";
    std::cout << debugmsg << std::endl;
#endif

    return prodIndex++;
}


/**
 * Transfers stream of files.
 * Expected behavior from application: fetch_next_product() should:
 * (i): return the pointers and metadata of next file
 * (ii): block until there is a new file
 * (iii): return 0 to indicate the end of a file stream
 */
void vcmtpSendv3::sendProductStream()
{
    while(notifier->fetch_next_product()) {
    }
}


/**
 * Sets sending rate. The timer thread needs this link speed to calculate
 * the sleep time. It is an alternative solution to tc rate limiting.
 *
 * @param[in] speed         Given link speed, which supports up to 18000 Pbps,
 *                          speed should be in the form of bits per second.
 */
void vcmtpSendv3::SetSendRate(uint64_t speed)
{
    rateshaper.SetRate(speed);
    std::unique_lock<std::mutex> lock(linkmtx);
    linkspeed = speed;
}


/**
 * Starts the coordinator thread and timer thread from this function. And
 * passes a vcmtpSendv3 type pointer to each newly created thread so that
 * coordinator and timer can have access to all the resources inside this
 * vcmtpSendv3 instance. If this method succeeds, then the caller must call
 * `Stop()` before this instance is destroyed. Returns immediately.
 *
 * **Exception Safety:** No guarantee
 *
 * @throw  std::runtime_error if a runtime error occurs.
 * @throw  std::runtime_error  if a system error occurs.
 */
void vcmtpSendv3::Start()
{
    /* start listening to incoming connections */
    tcpsend->Init();
    /* initialize UDP connection */
    udpsend->Init();

    /* initializes a new SilenceSuppressor instance. */
    suppressor = new SilenceSuppressor(PRODNUM * EXPTRUN);

    int retval = pthread_create(&timer_t, NULL, &vcmtpSendv3::timerWrapper, this);
    if(retval != 0) {
        throw std::runtime_error(
                "vcmtpSendv3::Start() pthread_create() timerWrapper error with"
                " retval = " + std::to_string(retval));
    }

    retval = pthread_create(&coor_t, NULL, &vcmtpSendv3::coordinator, this);
    if(retval != 0) {
        (void)pthread_cancel(timer_t);
        throw std::runtime_error(
                "vcmtpSendv3::Start() pthread_create() coordinator error with"
                " retval = " + std::to_string(retval));
    }
}


/**
 * Stops this instance. Must be called if `Start()` succeeds. Doesn't return
 * until all threads have stopped.
 *
 * @throws std::exception  If an exception was thrown on a thread.
 */
void vcmtpSendv3::Stop()
{
    timerDelayQ.disable(); // will cause timer thread to exit
    (void)pthread_cancel(coor_t);
    /* cancels all the threads in list and empties the list */
    retxThreadList.shutdown();

    (void)pthread_join(timer_t, NULL);
    (void)pthread_join(coor_t, NULL);

    {
        std::unique_lock<std::mutex> lock(exitMutex);
        if (exceptIsSet) {
            std::rethrow_exception(except);
        }
    }
}


/**
 * Adds and entry for a data-product to the retransmission set.
 *
 * @param[in] data      The data-product.
 * @param[in] dataSize  The size of the data-product in bytes.
 * @return              The corresponding retransmission entry.
 * @throw std::runtime_error  if a retransmission entry couldn't be created.
 */
RetxMetadata* vcmtpSendv3::addRetxMetadata(void* const data,
                                           const uint32_t dataSize,
                                           void* const metadata,
                                           const uint16_t metaSize)
{
    /* Create a new RetxMetadata struct for this product */
    RetxMetadata* senderProdMeta = new RetxMetadata();
    if (senderProdMeta == NULL) {
        throw std::runtime_error(
                "vcmtpSendv3::addRetxMetadata(): create RetxMetadata error");
    }

    /**
     * Since the metadata pointer is not guaranteed to be persistent,
     * the content of metadata is copied to a dynamically allocated array
     * and saved in senderProdMeta.
     */
    char* metadata_ptr = new char[metaSize];
    (void)memcpy(metadata_ptr, metadata, metaSize);

    /* Update current prodindex in RetxMetadata */
    senderProdMeta->prodindex        = prodIndex;

    /* Update current product length in RetxMetadata */
    senderProdMeta->prodLength       = dataSize;

    /* Update current metadata size in RetxMetadata */
    senderProdMeta->metaSize         = metaSize;

    /* Update current metadata pointer in RetxMetadata */
    senderProdMeta->metadata         = (void*)metadata_ptr;

    /* Update current product pointer in RetxMetadata */
    senderProdMeta->dataprod_p       = (void*)data;

    /* Get a full list of current connected sockets and add to unfinished set */
    std::list<int> currSockList = tcpsend->getConnSockList();
    std::list<int>::iterator it;
    for (it = currSockList.begin(); it != currSockList.end(); ++it)
        senderProdMeta->unfinReceivers.insert(*it);

    /* Add current RetxMetadata into sendMetadata::indexMetaMap */
    sendMeta->addRetxMetadata(senderProdMeta);

    return senderProdMeta;
}


/**
 * The sender side coordinator thread. Listen for incoming TCP connection
 * requests in an infinite loop and assign a new socket for the corresponding
 * receiver. Then pass that new socket as a parameter to start a receiver-
 * specific thread.
 *
 * @param[in] *ptr    void type pointer that points to whatever data structure.
 * @return            void type pointer that points to whatever return value.
 */
void* vcmtpSendv3::coordinator(void* ptr)
{
    vcmtpSendv3* sendptr = static_cast<vcmtpSendv3*>(ptr);
    try {
        while(1) {
            int newtcpsockfd = sendptr->tcpsend->acceptConn();
            int initState;
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &initState);
            sendptr->StartNewRetxThread(newtcpsockfd);
            int ignoredState;
            pthread_setcancelstate(initState, &ignoredState);
        }
    }
    catch (std::runtime_error& e) {
        sendptr->taskExit(e);
    }
    return NULL;
}


/**
 * Handles a retransmission request from a receiver.
 *
 * @param[in] recvheader  VCMTP header of the retransmission request.
 * @param[in] retxMeta    Associated retransmission entry or `0`, in which case
 *                        the request will be rejected.
 * @param[in] sock        The receiver's socket.
 */
void vcmtpSendv3::handleRetxReq(VcmtpHeader* const  recvheader,
                                RetxMetadata* const retxMeta,
                                const int           sock)
{
    if (retxMeta) {
        retransmit(recvheader, retxMeta, sock);
    }
    else {
        /**
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
void vcmtpSendv3::handleRetxEnd(VcmtpHeader* const  recvheader,
                                RetxMetadata* const retxMeta,
                                const int           sock)
{
    if (retxMeta) {
        /**
         * Remove the specific receiver out of the unfinished receiver
         * set. Only if the product is removed by clearUnfinishedSet(),
         * it returns a true value.
         */
        if (sendMeta->clearUnfinishedSet(recvheader->prodindex, sock)) {
            /**
             * Only if the product is removed by clearUnfinishedSet()
             * since this receiver is the last one in the unfinished set,
             * notify the sending application.
             */
            if (notifier) {
                notifier->notify_of_eop(recvheader->prodindex);
            }
            else {
                suppressor->remove(recvheader->prodindex);
                /**
                 * Updates the most recently acknowledged product and notifies
                 * a dummy notification handler (getNotify()).
                 */
                {
                    std::unique_lock<std::mutex> lock(notifyprodmtx);
                    notifyprodidx = recvheader->prodindex;
                }
                notify_cv.notify_one();
                memrelease_cv.notify_one();
            }
        }
    }
}


/**
 * Handles the RETX_BOP request from receiver. If the corresponding metadata
 * is still in the RetxMetadata map, then issue a BOP retransmission.
 * Otherwise, reject the request.
 *
 * @param[in] recvheader  The VCMTP header of the retransmission request.
 * @param[in] retxMeta    Associated retransmission entry.
 * @param[in] sock        The receiver's socket.
 */
void vcmtpSendv3::handleBopReq(VcmtpHeader* const  recvheader,
                               RetxMetadata* const retxMeta,
                               const int           sock)
{
    if (retxMeta) {
        retransBOP(recvheader, retxMeta, sock);
    }
    else {
        /**
         * Reject the request because the retransmission entry was removed by
         * the per-product timer thread.
         */
        rejRetxReq(recvheader->prodindex, sock);
    }
}


/**
 * Handles the RETX_EOP request from receiver. If the corresponding metadata
 * is still in the RetxMetadata map, then issue a EOP retransmission.
 * Otherwise, reject the request.
 *
 * @param[in] recvheader  The VCMTP header of the retransmission request.
 * @param[in] retxMeta    Associated retransmission entry.
 * @param[in] sock        The receiver's socket.
 */
void vcmtpSendv3::handleEopReq(VcmtpHeader* const  recvheader,
                               RetxMetadata* const retxMeta,
                               const int           sock)
{
    if (retxMeta) {
        retransEOP(recvheader, sock);
    }
    else {
        /**
         * Reject the request because the retransmission entry was removed by
         * the per-product timer thread.
         */
        rejRetxReq(recvheader->prodindex, sock);
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
        /** Receive the message from tcp connection and parse the header */
        if (tcpsend->parseHeader(retxsockfd, &recvheader) < 0) {
            throw std::runtime_error("vcmtpSendv3::RunRetxThread() receive "
                                     "header error");
        }

        /* Acquires the product metadata as in exclusive use */
        RetxMetadata* retxMeta = sendMeta->getMetadata(recvheader.prodindex);

        if (recvheader.flags == VCMTP_RETX_REQ) {
            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(recvheader.prodindex);
                debugmsg += ": RETX_REQ received";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
            handleRetxReq(&recvheader, retxMeta, retxsockfd);
        }
        else if (recvheader.flags == VCMTP_RETX_END) {
            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(recvheader.prodindex);
                debugmsg += ": RETX_END received";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
            handleRetxEnd(&recvheader, retxMeta, retxsockfd);
        }
        else if (recvheader.flags == VCMTP_BOP_REQ) {
            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(recvheader.prodindex);
                debugmsg += ": BOP_REQ received";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
            handleBopReq(&recvheader, retxMeta, retxsockfd);
        }
        else if (recvheader.flags == VCMTP_EOP_REQ) {
            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(recvheader.prodindex);
                debugmsg += ": EOP_REQ received";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
            handleEopReq(&recvheader, retxMeta, retxsockfd);
        }

        /* Releases the product metadata in exclusive use */
        sendMeta->releaseMetadata(recvheader.prodindex);
    }
}


/**
 * Rejects a retransmission request from a receiver. A sender side timeout or
 * received RETX_END message from all receivers and then receiving retx
 * requests again would cause the rejection.
 *
 * @param[in] prodindex  Product-index of the request.
 * @param[in] sock       The receiver's socket.
 */
void vcmtpSendv3::rejRetxReq(const uint32_t prodindex, const int sock)
{
    VcmtpHeader sendheader;

    sendheader.prodindex  = htonl(prodindex);
    sendheader.seqnum     = 0;
    sendheader.payloadlen = 0;
    sendheader.flags      = htons(VCMTP_RETX_REJ);
    tcpsend->sendData(sock, &sendheader, NULL, 0);
}


/**
 * Retransmits data to a receiver. Requested retransmition block size will be
 * checked to make sure the request is valid.
 *
 * @param[in] recvheader  The VCMTP header of the retransmission request.
 * @param[in] retxMeta    The associated retransmission entry.
 * @param[in] sock        The receiver's socket.
 *
 * @throw std::runtime_error if TcpSend::send() fails.
 */
void vcmtpSendv3::retransmit(
        const VcmtpHeader* const  recvheader,
        const RetxMetadata* const retxMeta,
        const int                 sock)
{
    if (0 < recvheader->payloadlen) {
        uint32_t start = recvheader->seqnum;
        uint32_t out   = MIN(retxMeta->prodLength,
                start + recvheader->payloadlen);

        VcmtpHeader sendheader;
        sendheader.prodindex  = htonl(recvheader->prodindex);
        sendheader.flags      = htons(VCMTP_RETX_DATA);

        /**
         * aligns starting seqnum to the multiple-of-MTU boundary.
         */
        start = (start/VCMTP_DATA_LEN) * VCMTP_DATA_LEN;
        uint16_t payLen = VCMTP_DATA_LEN;

        /**
         * Support sending multiple blocks.
         */
        for (uint32_t nbytes = out - start; 0 < nbytes; nbytes -= payLen) {
            if (payLen > nbytes)
                /** only last block might be truncated */
                payLen = nbytes;

            sendheader.seqnum     = htonl(start);
            sendheader.payloadlen = htons(payLen);

            #if defined(DEBUG1) || defined(DEBUG2)
                char tmp[1460] = {0};
                int retval = tcpsend->sendData(sock, &sendheader, tmp, payLen);
            #else
                int retval = tcpsend->sendData(sock, &sendheader,
                                (char*)retxMeta->dataprod_p + start, payLen);
            #endif

            if (retval < 0) {
                throw std::runtime_error(
                        "vcmtpSendv3::retransmit() TcpSend::send() error");
            }

            #ifdef MODBASE
                uint32_t tmpidx = recvheader->prodindex % MODBASE;
            #else
                uint32_t tmpidx = recvheader->prodindex;
            #endif

            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(tmpidx);
                debugmsg += ": Data block (SeqNum = ";
                debugmsg += std::to_string(start);
                debugmsg += ") has been retransmitted";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
        }
    }
}

/**
 * Retransmits BOP to a receiver. All necessary metadata will be retrieved
 * from the RetxMetadata map. This implies the addRetxMetadata() operation
 * should be guaranteed to succeed so that a valid metadata can always be
 * retrieved.
 *
 * @param[in] recvheader  The VCMTP header of the retransmission request.
 * @param[in] retxMeta    The associated retransmission entry.
 * @param[in] sock        The receiver's socket.
 *
 * @throw std::runtime_error if TcpSend::send() fails.
 */
void vcmtpSendv3::retransBOP(
        const VcmtpHeader* const  recvheader,
        const RetxMetadata* const retxMeta,
        const int                 sock)
{
    VcmtpHeader   sendheader;
    BOPMsg        bopMsg;

    /* Set the VCMTP packet header. */
    sendheader.prodindex  = htonl(recvheader->prodindex);
    sendheader.seqnum     = 0;
    sendheader.payloadlen = htons(retxMeta->metaSize +
                                  (VCMTP_DATA_LEN - AVAIL_BOP_LEN));
    sendheader.flags      = htons(VCMTP_RETX_BOP);

    /* Set the VCMTP BOP message. */
    bopMsg.prodsize = htonl(retxMeta->prodLength);
    bopMsg.metasize = htons(retxMeta->metaSize);
    memcpy(&bopMsg.metadata, retxMeta->metadata, retxMeta->metaSize);

    /** actual BOPmsg size may not be AVAIL_BOP_LEN, payloadlen is corret */
    int retval = tcpsend->sendData(sock, &sendheader, (char*)(&bopMsg),
                               ntohs(sendheader.payloadlen));
    if (retval < 0) {
        throw std::runtime_error(
                "vcmtpSendv3::retransBOP() TcpSend::send() error");
    }

    #ifdef MODBASE
        uint32_t tmpidx = recvheader->prodindex % MODBASE;
    #else
        uint32_t tmpidx = recvheader->prodindex;
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "Product #" +
            std::to_string(tmpidx);
        debugmsg += ": BOP has been retransmitted";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif
}


/**
 * Retransmits EOP to a receiver.
 *
 * @param[in] recvheader  The VCMTP header of the retransmission request.
 * @param[in] sock        The receiver's socket.
 *
 * @throw std::runtime_error if TcpSend::send() fails.
 */
void vcmtpSendv3::retransEOP(
        const VcmtpHeader* const  recvheader,
        const int                 sock)
{
    VcmtpHeader   sendheader;

    /* Set the VCMTP packet header. */
    sendheader.prodindex  = htonl(recvheader->prodindex);
    sendheader.seqnum     = 0;
    sendheader.payloadlen = 0;
    /** notice the flags field should be set to RETX_EOP other than EOP */
    sendheader.flags      = htons(VCMTP_RETX_EOP);

    int retval = tcpsend->sendData(sock, &sendheader, NULL, 0);
    if (retval < 0) {
        throw std::runtime_error(
                "vcmtpSendv3::retransEOP() TcpSend::send() error");
    }

    #ifdef MODBASE
        uint32_t tmpidx = recvheader->prodindex % MODBASE;
    #else
        uint32_t tmpidx = recvheader->prodindex;
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "Product #" +
            std::to_string(tmpidx);
        debugmsg += ": EOP has been retransmitted";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif
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
 * @throw std::runtime_error  if the UdpSend::SendTo() fails.
 */
void vcmtpSendv3::SendBOPMessage(uint32_t prodSize, void* metadata,
                                 const uint16_t metaSize)
{
    VcmtpHeader   header;
    BOPMsg        bopMsg;
    struct iovec  ioVec[4];

    /* Set the VCMTP packet header. */
    header.prodindex  = htonl(prodIndex);
    header.seqnum     = 0;
    header.payloadlen = htons(metaSize + (uint16_t)(VCMTP_DATA_LEN -
                                                    AVAIL_BOP_LEN));
    header.flags      = htons(VCMTP_BOP);

    ioVec[0].iov_base = &header;
    ioVec[0].iov_len  = sizeof(VcmtpHeader);

    bopMsg.prodsize = htonl(prodSize);
    ioVec[1].iov_base = &bopMsg.prodsize;
    ioVec[1].iov_len  = sizeof(bopMsg.prodsize);

    bopMsg.metasize = htons(metaSize);
    ioVec[2].iov_base = &bopMsg.metasize;
    ioVec[2].iov_len  = sizeof(bopMsg.metasize);

    ioVec[3].iov_base = metadata;
    ioVec[3].iov_len  = metaSize;

    #ifdef MODBASE
        uint32_t tmpidx = prodIndex % MODBASE;
    #else
        uint32_t tmpidx = prodIndex;
    #endif

#ifdef TEST_BOP
    #ifdef DEBUG2
        std::string debugmsg = "Product #" + std::to_string(tmpidx);
        debugmsg += ": Test BOP missing (BOP not sent)";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif
#else
    #ifdef MEASURE
        std::string measuremsg = "Product #" + std::to_string(tmpidx);
        measuremsg += ": Transmission start time (BOP), Prodsize = ";
        measuremsg += std::to_string(prodSize);
        measuremsg += " bytes";
        std::cout << measuremsg << std::endl;
        /* set txdone to false in BOPHandler */
        txdone = false;
        start_t = std::chrono::high_resolution_clock::now();
        WriteToLog(measuremsg);
    #endif

    /* Send the BOP message on multicast socket */
    udpsend->SendTo(ioVec, 4);

    #ifdef DEBUG2
        std::string debugmsg = "Product #" + std::to_string(tmpidx);
        debugmsg += ": BOP has been sent";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif
#endif
}


/**
 * Sends the EOP message to the receiver to indicate the end of a product
 * transmission.
 *
 * @throws std::runtime_error  if UdpSend::SendTo() fails.
 */
void vcmtpSendv3::sendEOPMessage()
{
    VcmtpHeader header;

    header.prodindex  = htonl(prodIndex);
    header.seqnum     = 0;
    header.payloadlen = 0;
    header.flags      = htons(VCMTP_EOP);

    #ifdef MODBASE
        uint32_t tmpidx = prodIndex % MODBASE;
    #else
        uint32_t tmpidx = prodIndex;
    #endif

#ifdef TEST_EOP
    #ifdef DEBUG2
        std::string debugmsg = "Product #" + std::to_string(tmpidx);
        debugmsg += ": EOP missing case (EOP not sent).";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif
#else
    udpsend->SendTo(&header, sizeof(header));

    #ifdef MEASURE
        std::string measuremsg = "Product #" + std::to_string(tmpidx);
        measuremsg += ": Transmission end time (EOP)";
        std::cout << measuremsg << std::endl;
        /* set txdone to true in EOPHandler */
        txdone = true;
        end_t = std::chrono::high_resolution_clock::now();
        WriteToLog(measuremsg);
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "Product #" + std::to_string(tmpidx);
        debugmsg += ": EOP has been sent.";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif
#endif
}


/**
 * Multicasts the data blocks of a data-product. A legal boundary check is
 * performed to make sure all the data blocks going out are multiples of
 * VCMTP_DATA_LEN except the last block.
 *
 * @param[in] data      The data-product.
 * @param[in] dataSize  The size of the data-product in bytes.
 * @throw std::runtime_error  if an I/O error occurs.
 */
void vcmtpSendv3::sendData(void* data, uint32_t dataSize)
{
    VcmtpHeader header;
    uint32_t datasize = dataSize;
    uint32_t seqNum = 0;
    header.prodindex = htonl(prodIndex);
    header.flags     = htons(VCMTP_MEM_DATA);

    /* check if there is more data to send */
    while (datasize > 0) {
        uint16_t payloadlen = datasize < VCMTP_DATA_LEN ?
                              datasize : VCMTP_DATA_LEN;

        header.seqnum     = htonl(seqNum);
        header.payloadlen = htons(payloadlen);

        #ifdef TEST_DATA_MISS
            if (seqNum == DROPSEQ)
            {}
            else {
        #endif

        /**
         * linkspeed is initialized to 0. If SetSendRate() is never called,
         * linkspeed will remain 0, which implies application itself doesn't
         * need to take care of rate shaping. On the other hand, if app
         * should shape its rate, SetSendRate() must be called first. Thus,
         * linkspeed will be a non-zero value. By checking linkspeed, app
         * can decide whether to do rate shaping.
         */
        //TODO: use Rateshaper to replace tc?
        if (linkspeed) {
            rateshaper.CalcPeriod(sizeof(header) + payloadlen);
        }
        if(udpsend->SendData(&header, sizeof(header), data,
                             (size_t)payloadlen) < 0) {
            throw std::runtime_error(
                    "vcmtpSendv3::sendProduct::SendData() error");
        }
        if (linkspeed) {
            rateshaper.Sleep();
        }

        #ifdef MODBASE
            uint32_t tmpidx = prodIndex % MODBASE;
        #else
            uint32_t tmpidx = prodIndex;
        #endif

        #ifdef DEBUG2
            std::string debugmsg = "Product #" + std::to_string(tmpidx);
            debugmsg += ": Data block (SeqNum = ";
            debugmsg += std::to_string(seqNum);
            debugmsg += ") has been sent.";
            std::cout << debugmsg << std::endl;
            WriteToLog(debugmsg);
        #endif

        #ifdef TEST_DATA_MISS
            }
        #endif

        datasize -= payloadlen;
        data      = (char*)data + payloadlen;
        seqNum   += payloadlen;
    }
}


/**
 * Sets the retransmission timeout parameters in a retransmission entry. The
 * start time being recorded is when a new RetxMetadata is added into the
 * metadata map. The end time is when the whole sending process is finished.
 *
 * @param[in] senderProdMeta  The retransmission entry.
 */
void vcmtpSendv3::setTimerParameters(RetxMetadata* const senderProdMeta)
{
    /**
    * use a constant timer. the timer value has been studied and roughly 2 min
    * is the minimum requirement.
    */
    senderProdMeta->retxTimeoutPeriod = tsnd * 60;
}


/**
 * Create all the necessary information and fill into the StartRetxThreadInfo
 * structure. Pass the pointer of this structure as a set of parameters to the
 * new thread. Accepts responsibility for closing the socket in all
 * circumstances.
 *
 * @param[in] newtcpsockfd
 * @throw     std::runtime_error  if pthread_create() fails.
 */
void vcmtpSendv3::StartNewRetxThread(int newtcpsockfd)
{
    pthread_t t;

    StartRetxThreadInfo* retxThreadInfo = new StartRetxThreadInfo();
    retxThreadInfo->retxmitterptr       = this;
    retxThreadInfo->retxsockfd          = newtcpsockfd;

    int retval = pthread_create(&t, NULL, &vcmtpSendv3::StartRetxThread,
                                retxThreadInfo);
    if(retval != 0)
    {
        /*
         * If a new thread can't be created, the newly created socket needs to
         * be closed and removed from the TcpSend::connSockList.
         */
        tcpsend->rmSockInList(newtcpsockfd);
        close(newtcpsockfd);

        #ifdef DEBUG2
            std::string debugmsg = "Error: vcmtpSendv3::StartNewRetxThread() \
                                    creating new thread failed";
            std::cout << debugmsg << std::endl;
            WriteToLog(debugmsg);
        #endif
    }
    else {
        /** track all the newly created retx threads for later termination */
        retxThreadList.add(t);
        pthread_detach(t);
    }
}


/**
 * Use the passed-in pointer to extract parameters. The first pointer as a
 * pointer of vcmtpSendv3 instance can start vcmtpSendv3 member function.
 * The second parameter is sockfd, the third one is the prodindex-prodptr map.
 * If the callee throws any exception, the try-catch structure will catch that
 * and terminates the thread itself as well as other associated resources.
 *
 * @param[in] *ptr    a void type pointer that points to whatever data struct.
 */
void* vcmtpSendv3::StartRetxThread(void* ptr)
{
    StartRetxThreadInfo* newptr = static_cast<StartRetxThreadInfo*>(ptr);
    try {
        newptr->retxmitterptr->RunRetxThread(newptr->retxsockfd);
    }
    catch (std::runtime_error& e) {
        int exitStatus;
        pthread_t t = pthread_self();

        newptr->retxmitterptr->tcpsend->rmSockInList(newptr->retxsockfd);
        close(newptr->retxsockfd);
        newptr->retxmitterptr->retxThreadList.remove(t);
        pthread_exit(&exitStatus);
    }
    return NULL;
}


/**
 * Task terminator. If an exception is caught, this function will be called.
 * It consequently terminates all the other threads by calling the Stop(). This
 * task exit call will not be blocking.
 *
 * @param[in] e                     Exception status
 */
void vcmtpSendv3::taskExit(const std::runtime_error& e)
{
    {
        std::unique_lock<std::mutex> lock(exitMutex);
        if (!exceptIsSet) {
            except = std::make_exception_ptr(e);
            exceptIsSet = true;
        }
    }
    Stop();
}


/**
 * The per-product timer. A product-specified timer element will be created
 * when sendProduct() is called and pushed into the ProductIndexDelayQueue.
 * The timer will keep querying the queue with a blocking operation and fetch
 * the valid element with a mutex-protected pop(). Basically, the delay queue
 * makes the valid element and only the valid element visible to the timer
 * thread. The element is made visible when the designated sleep time expires.
 * The sleep time is specified in the RetxMetadata structure. When the timer
 * wakes up from sleeping, it will check and remove the corresponding product
 * from the prodindex-retxmetadata map.
 *
 * @param[in] none
 */
void vcmtpSendv3::timerThread()
{
    while (1) {
        uint32_t prodindex;
        try {
            prodindex = timerDelayQ.pop();
        }
        catch (std::runtime_error& e) {
            // Product-index delay-queue, `timerDelayQ`, was externally disabled
            return;
        }

        #ifdef MODBASE
            uint32_t tmpidx = prodindex % MODBASE;
        #else
            uint32_t tmpidx = prodindex;
        #endif

        #ifdef DEBUG2
            std::string debugmsg = "Timer: Product #" +
                std::to_string(tmpidx);
            debugmsg += " has waken up";
            std::cout << debugmsg << std::endl;
            WriteToLog(debugmsg);
        #endif

        const bool isRemoved = sendMeta->rmRetxMetadata(prodindex);
        /**
         * Only if the product is removed by this remove call, notify the
         * sending application. Since timer and retx thread access the
         * RetxMetadata exclusively, notify_of_eop() will be called only once.
         */
        if (notifier && isRemoved) {
            notifier->notify_of_eop(prodindex);
        }
        else if (isRemoved) {
            suppressor->remove(prodindex);
            /**
             * Updates the most recently acknowledged product and notifies
             * a dummy notification handler (getNotify()).
             */
            {
                std::unique_lock<std::mutex> lock(notifyprodmtx);
                notifyprodidx = prodindex;
            }
            notify_cv.notify_one();
            memrelease_cv.notify_one();
        }
    }
}


/**
 * A wrapper function which is used to call the real timerThread(). Due to the
 * unavailability of some vcmtpSendv3 private resources (e.g. notifier), this
 * wrapper is called first when timer needs to be started and itself points
 * back to the vcmtpSendv3 instance.
 *
 * @param[in] ptr                a pointer to the vcmtpSendv3 class.
 */
void* vcmtpSendv3::timerWrapper(void* ptr)
{
    vcmtpSendv3* const sender = static_cast<vcmtpSendv3*>(ptr);
    try {
        sender->timerThread();
    }
    catch (std::runtime_error& e) {
        sender->taskExit(e);
    }
    return NULL;
}


/**
 * Write a line of log record into the log file. If the log file doesn't exist,
 * create a new one and then append to it.
 *
 * @param[in] content       The content of the log record to be written.
 */
void vcmtpSendv3::WriteToLog(const std::string& content)
{
    time_t rawtime;
    struct tm *timeinfo;
    char buf[30];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buf, 30, "%Y-%m-%d %I:%M:%S  ", timeinfo);
    std::string time(buf);

    /* code block only effective for measurement code */
    #ifdef MEASURE
    std::string hrclk;
    std::string nanosec;
    if (txdone) {
        hrclk = std::to_string(end_t.time_since_epoch().count())
            + " since epoch, ";
    }
    else {
        hrclk = std::to_string(start_t.time_since_epoch().count())
            + " since epoch, ";
    }

    if (txdone) {
        std::chrono::duration<double> timespan =
            std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
        nanosec = ", Elapsed time: " + std::to_string(timespan.count());
        nanosec += " seconds.";
    }
    #endif
    /* code block ends */

    std::ofstream logfile("VCMTPv3_SENDER.log",
            std::ofstream::out | std::ofstream::app);
    #ifdef MEASURE
        if (txdone) {
            logfile << time << hrclk << content << nanosec << std::endl;
        }
        else {
            logfile << time << hrclk << content << std::endl;
        }
    #else
        logfile << time << content << std::endl;
    #endif
    logfile.close();
}
