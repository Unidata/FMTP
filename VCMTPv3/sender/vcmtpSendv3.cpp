/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpSendv3.cpp
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
 * @brief     Define the entity of VCMTPv3 sender side method function.
 *
 * Sender side of VCMTPv3 protocol. It multicasts packets out to multiple
 * receivers and retransmits missing blocks to the receivers.
 */


#include "vcmtpSendv3.h"
#include <stdio.h>

/**
 * Construct a sender instance with prodIndex maintained by sender itself.
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
{
    udpsocket = 0;
    tcpsend   = 0;
    sendMeta  = 0;
    prodIndex = 0;
    vcmtpSendv3(tcpAddr, tcpPort, mcastAddr, mcastPort, prodIndex);
}


/**
 * Construct a sender instance with prodIndex specified and initialized by
 * receiving applications.
 *
 * @param[in] tcpAddr         Unicast address of the sender.
 * @param[in] tcpPort         Unicast port of the sender or 0, in which case one
 *                            is chosen by the operating-system.
 * @param[in] mcastAddr       Multicast group address.
 * @param[in] mcastPort       Multicast group port.
 * @param[in] initProdIndex   Initial prodIndex set by receiving applications.
 */
vcmtpSendv3::vcmtpSendv3(const char*          tcpAddr,
                         const unsigned short tcpPort,
                         const char*          mcastAddr,
                         const unsigned short mcastPort,
                         uint32_t             initProdIndex)
{
    prodIndex = initProdIndex;
    udpsocket = new UdpSocket(mcastAddr,mcastPort);
    tcpsend   = new TcpSend(tcpAddr, tcpPort);
    sendMeta  = new senderMetadata();
}


/**
 * Deconstruct the sender instance and release the initialized resources.
 *
 * @param[in] none
 */
vcmtpSendv3::~vcmtpSendv3()
{
    delete udpsocket;
    delete tcpsend;
    delete sendMeta;
}


/**
 * Send the BOP message to the receiver.
 *
 * @param[in] prodSize       The size of the product.
 * @param[in] metadata       Application-specific metadata to be sent before the
 *                           data. May be 0, in which case no metadata is sent.
 * @param[in] metaSize       Size of the metadata in bytes. Must be less than
 *                           or equals 1442. May be 0, in which case no metadata
 *                           is sent.
 */
void vcmtpSendv3::SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize)
{
    uint16_t maxMetaSize = metaSize > AVAIL_BOP_LEN ? AVAIL_BOP_LEN : metaSize;

    const int PACKET_SIZE = VCMTP_HEADER_LEN + maxMetaSize + (VCMTP_DATA_LEN - AVAIL_BOP_LEN);
    unsigned char vcmtp_packet[PACKET_SIZE];
    /** create a vcmtp header pointer that point at the beginning of the vcmtp packet */
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet;
    /** create vcmtp data pointer that points to the location just after the header */
    VcmtpBOPMessage*   vcmtp_data   = (VcmtpBOPMessage*) (vcmtp_packet + VCMTP_HEADER_LEN);

    bzero(vcmtp_packet, sizeof(vcmtp_packet));

    /** convert the variables from native binary to network binary representation */
    uint32_t prodindex   = htonl(prodIndex);
    uint32_t seqNum      = htonl(0);        /** for BOP sequence number is always zero */
    uint16_t payLen      = htons(maxMetaSize+(VCMTP_DATA_LEN - AVAIL_BOP_LEN));
    uint16_t flags       = htons(VCMTP_BOP);
    uint32_t prodsize    = htonl(prodSize);
    uint16_t maxmetasize = htons(maxMetaSize);

   /** create the content of the vcmtp header */
   memcpy(&vcmtp_header->prodindex,   &prodindex, 4);
   memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
   memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
   memcpy(&vcmtp_header->flags,       &flags,     2);

   /** create the content of the BOP */
   memcpy(&vcmtp_data->prodsize, &prodsize,             4);
   memcpy(&vcmtp_data->metasize, &maxmetasize,          2);
   memcpy(&vcmtp_data->metadata, metadata,        maxMetaSize);

   /** send the bomd message */
   if (udpsocket->SendTo(vcmtp_packet,PACKET_SIZE) < 0)
       perror("vcmtpSendv3::SendBOPMessage::SendTo error");
}


/**
 * Transfer a contiguous block of memory (without metadata).
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
 * Transfer Application-specified metadata and a contiguous block of memory.
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
uint32_t vcmtpSendv3::sendProduct(char* data,
                                  size_t dataSize,
                                  char* metadata,
                                  unsigned metaSize)
{
	RetxMetadata* senderProdMeta = new RetxMetadata(); /*!< init struct like a class */
	/** get start time of multicasting for measuring product transmit time */
	if (senderProdMeta == NULL)
		perror("vcmtpSendv3::sendProduct() error creating new RetxMetadata instance");

	/** update current prodindex in RetxMetadata */
	senderProdMeta->prodindex 	   = prodIndex;
	/** update current product length in RetxMetadata */
	senderProdMeta->prodLength     = dataSize;
	/** update current product pointer in RetxMetadata */
	senderProdMeta->dataprod_p     = (void*) data;
	/** get a full list of current connected sockets and add to unfinished set */
	list<int> currSockList = tcpsend->getConnSockList();
	for (list<int>::iterator it = currSockList.begin(); it != currSockList.end(); ++it)
	{
		senderProdMeta->unfinReceivers.insert(*it);
	}
	/** add current RetxMetadata into sendMetadata::indexMetaMap */
	sendMeta->addRetxMetadata(senderProdMeta);
	/** update multicast start time in RetxMetadata */
	senderProdMeta->mcastStartTime = clock();

    SendBOPMessage(dataSize, metadata, metaSize);
    char vcmtpHeader[VCMTP_HEADER_LEN];
    VcmtpPacketHeader* header = (VcmtpPacketHeader*) vcmtpHeader;

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

        if(udpsocket->SendData(vcmtpHeader, VCMTP_HEADER_LEN, data, data_size) < 0)
            perror("vcmtpSendv3::sendProduct::SendData() error");

        remained_size -= data_size;
        /** move the data pointer to the beginning of the next block */
        data = (char*) data + data_size;
        seqNum += data_size;
    }
    sendEOPMessage();

	/** get end time of multicasting for measuring product transmit time */
	senderProdMeta->mcastEndTime = clock();

	/** set up timer and trigger */
	senderProdMeta->timeoutSec = 5;
	senderProdMeta->timeoutuSec = 500000;

	/** start a new timer for this product */
	startTimerThread(prodIndex);

    return prodIndex++;
}


/**
 * Send the EOP message to the receiver to indicate the end of a product
 * transmission.
 *
 * @param[in] none
 */
void vcmtpSendv3::sendEOPMessage()
{
    unsigned char vcmtp_packet[VCMTP_HEADER_LEN];
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet;
    bzero(vcmtp_packet, sizeof(vcmtp_packet));

    uint32_t prodindex = htonl(prodIndex);
    /** seqNum for the EOP should always be zero */
    uint32_t seqNum    = htonl(0);
    uint16_t payLen    = htons(0);
    uint16_t flags     = htons(VCMTP_EOP);
    /** create the content of the vcmtp header */
    memcpy(&vcmtp_header->prodindex,   &prodindex, 4);
    memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
    memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
    memcpy(&vcmtp_header->flags,       &flags,     2);

    /** send the EOP message */
    if (udpsocket->SendTo(vcmtp_packet, VCMTP_HEADER_LEN) < 0)
        perror("vcmtpSendv3::sendEOPMessage::SendTo error");
}


void vcmtpSendv3::startCoordinator()
{
	pthread_t new_t;
	pthread_create(&new_t, NULL, &vcmtpSendv3::coordinator, this);
	pthread_detach(new_t);
}


void* vcmtpSendv3::coordinator(void* ptr)
{
	vcmtpSendv3* sendptr = (vcmtpSendv3*) ptr;
	int newtcpsockfd;
	while(1)
	{
		newtcpsockfd = sendptr->tcpsend->acceptConn();
		sendptr->StartNewRetxThread(newtcpsockfd);
	}
	return NULL;
}


/**
 * Return the local port number.
 *
 * @return 	              The local port number in host byte-order.
 * @throws std::system_error  The port number cannot be obtained.
 */
unsigned short vcmtpSendv3::getTcpPortNum()
{
	return tcpsend->getPortNum();
}


void vcmtpSendv3::StartNewRetxThread(int newtcpsockfd)
{
	pthread_t t;
	retxSockThreadMap[newtcpsockfd] = &t;
	retxSockFinishMap[newtcpsockfd] = false;

	StartRetxThreadInfo* retxThreadInfo = new StartRetxThreadInfo();
	retxThreadInfo->retxmitterptr 		= this;
	retxThreadInfo->retxsockfd 			= newtcpsockfd;
	retxThreadInfo->retxIndexProdptrMap = new map<uint32_t, void*>();

	retxSockInfoMap[newtcpsockfd] = retxThreadInfo;

	pthread_create(&t, NULL, &vcmtpSendv3::StartRetxThread, retxThreadInfo);
	pthread_detach(t);
}

void* vcmtpSendv3::StartRetxThread(void* ptr)
{
	StartRetxThreadInfo* newptr = (StartRetxThreadInfo*) ptr;
	newptr->retxmitterptr->RunRetxThread(newptr->retxsockfd,
										 *(newptr->retxIndexProdptrMap));
	return NULL;
}


void vcmtpSendv3::RunRetxThread(int retxsockfd,
								map<uint32_t, void*>& retxIndexProdptrMap)
{
	map<uint32_t, void*>::iterator it;

	//char recvbuf[MAX_VCMTP_PACKET_LEN];
	//VcmtpHeader* recvheader = (VcmtpHeader*) recvbuf;
	//char* recvpayload = recvbuf + VCMTP_HEADER_LEN;
	VcmtpHeader* recvheader = new VcmtpHeader();
	VcmtpHeader* sendheader = new VcmtpHeader();
	char sendpayload[VCMTP_DATA_LEN];

	while(1)
	{
		if (tcpsend->parseHeader(retxsockfd, recvheader) < 0)
			perror("vcmtpSendv3::RunRetxThread() receive header error");

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
				sendheader->seqnum	   = htonl(recvheader->seqnum);
				sendheader->payloadlen = htons(recvheader->payloadlen);
				sendheader->flags 	   = htons(VCMTP_RETX_REJ);
				tcpsend->send(retxsockfd, sendheader, NULL, 0);
			}
			else
			{
				/** get the pointer to the data product in product queue */
				void* prodptr;
				if ((it = retxIndexProdptrMap.find(recvheader->prodindex)) !=
					retxIndexProdptrMap.end())
				{
					prodptr = it->second;
				}
				else
				{
					/**
					 * The first time requesting for this prodptr associated
					 * with the given prodindex. Fetch it from the RetxMetaData.
					 * And add it to the retxIndexProdptrMap.
					 */
					prodptr = retxMeta->dataprod_p;
					/** if failed to fetch prodptr, throw an error and skip */
					if (prodptr == NULL)
					{
						perror("vcmtpSendv3::RunRetxThread() error retrieving prodptr");
						continue;
					}
					else
						retxIndexProdptrMap[recvheader->prodindex] = prodptr;
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
					sendheader->seqnum 	   = htonl(startPos);
					sendheader->payloadlen = htons(payLen);

#ifdef DEBUG
    char c0, c1;
    memcpy(&c0, (char*)prodptr+startPos, 1);
    memcpy(&c1, (char*)prodptr+startPos+1, 1);
    cout << "(Retx) data block:" << endl;
    printf("%X ", c0);
    printf("%X", c1);
    printf("\n");
#endif

					tcpsend->send(retxsockfd, sendheader,
								  (char*)(prodptr) + startPos, (size_t) payLen);

					startPos += payLen;
					remainedSize -= payLen;
				}
			}
		}
		else if (recvheader->flags & VCMTP_RETX_END)
		{
			map<uint32_t, void*>::iterator it;
			if ((it = retxIndexProdptrMap.find(recvheader->prodindex)) !=
				retxIndexProdptrMap.end())
			{
				retxIndexProdptrMap.erase(it);
			}

			/** if timer is still sleeping, update metadata. Otherwise, skip */
			if (retxMeta != NULL)
			{
				/** remove the specific receiver out of the unfinished receiver set */
				sendMeta->removeFinishedReceiver(recvheader->prodindex, retxsockfd);

				if(sendMeta->isRetxAllFinished(recvheader->prodindex))
				{
					//TODO: call LDM callback function to release product.
					sendMeta->rmRetxMetadata(recvheader->prodindex);
				}
			}
		}
	}
}


void vcmtpSendv3::startTimerThread(uint32_t prodindex)
{
	pthread_t t;
	StartTimerThreadInfo* timerinfo = new StartTimerThreadInfo();
	timerinfo->prodindex = prodindex;
	timerinfo->sendmeta = sendMeta;
	int retval = pthread_create(&t, NULL, &vcmtpSendv3::runTimerThread, timerinfo);
	if(retval != 0)
	{
		perror("vcmtpSendv3::startTimerThread() pthread_create error");
	}
	pthread_detach(t);
}


void* vcmtpSendv3::runTimerThread(void* ptr)
{
	StartTimerThreadInfo* timerinfo = (StartTimerThreadInfo*) ptr;
	Timer timer(timerinfo->prodindex, timerinfo->sendmeta);
	cout << "timer wakes up, prodindex #" << timerinfo->prodindex << " removed" << endl;
	delete timerinfo;
	timerinfo = NULL;
	return NULL;
}
