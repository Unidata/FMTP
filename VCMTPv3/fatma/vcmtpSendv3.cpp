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

/**
 * Construct a sender instance with prodIndex maintained by sender itself.
 *
 * @param[in] tcpAddr       Unicast address of the sender.
 * @param[in] tcpPort       Unicast port of the sender.
 * @param[in] mcastAddr     Multicast group address.
 * @param[in] mcastPort     Multicast group port.
 */
vcmtpSendv3::vcmtpSendv3(const char*  tcpAddr,
                         const ushort tcpPort,
                         const char*  mcastAddr,
                         const ushort mcastPort)
{
    udpsocket=0;
    prodIndex=0;
    vcmtpSendv3(tcpAddr, tcpPort, mcastAddr, mcastPort, prodIndex);
}


/**
 * Construct a sender instance with prodIndex specified and initialized by
 * receiving applications.
 *
 * @param[in] tcpAddr         Unicast address of the sender.
 * @param[in] tcpPort         Unicast port of the sender or 0, in which case one
 *                            is chosen by the operating-sytem.
 * @param[in] mcastAddr       Multicast group address.
 * @param[in] mcastPort       Multicast group port.
 * @param[in] initProdIndex   Initial prodIndex set by receiving applications.
 */
vcmtpSendv3::vcmtpSendv3(const char*  tcpAddr,
                         const ushort tcpPort,
                         const char*  mcastAddr,
                         const ushort mcastPort,
                         uint32_t     initProdIndex)
{
    prodIndex=initProdIndex;
    udpsocket=new UdpSocket(mcastAddr,mcastPort);
    // TODO: use `tcpAddr` and `tcpPort`
}


/**
 * Deconstruct the sender instance.
 *
 * @param[in] none
 */
vcmtpSendv3::~vcmtpSendv3()
{
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
    uint16_t maxMetaSize = metaSize>AVAIL_BOP_LEN?AVAIL_BOP_LEN:metaSize;

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
       cout<<"vcmtpSendv3::SendBOPMessage::SendTo error\n";
}


/**
 * Transfer a contiguous block of memory (without metadata).
 *
 * @param[in] data      Memory data to be sent.
 * @param[in] dataSize  Size of the memory data in bytes.
 * @return              Index of the product.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, size_t dataSize)
{
    return sendProduct(data, dataSize, 0, 0);
};


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
uint32_t vcmtpSendv3::sendProduct(void* data,
                                  size_t dataSize,
                                  void* metadata,
                                  unsigned metaSize)
{
    SendBOPMessage(dataSize, metadata, metaSize);
    unsigned char vcmtpHeader[VCMTP_HEADER_LEN];
    VcmtpPacketHeader* header = (VcmtpPacketHeader*) vcmtpHeader;

    uint32_t prodindex = htonl(prodIndex);
    uint16_t flags     = htons(VCMTP_MEM_DATA);
    uint16_t payLen;
    uint32_t seqNum    = 0;

    size_t remained_size = dataSize;
    /** check if there is more data to send */
    while (remained_size > 0)
    {
        uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;

        payLen = htons(data_size);
        seqNum = htonl(seqNum);

        memcpy(&header->prodindex,  &prodindex, 4);
        memcpy(&header->seqnum,     &seqNum,    4);
        memcpy(&header->payloadlen, &payLen,    2);
        memcpy(&header->flags,      &flags,     2);

        if (udpsocket->SendData(vcmtpHeader, VCMTP_HEADER_LEN, data,data_size) < 0)
            cout<<"vcmtpSendv3::sendProduct::SendData() error"<<endl;

        remained_size -= data_size;
        /** move the data pointer to the beginning of the next block */
        data = (char*)data + data_size;
        seqNum += data_size;
    }
    sendEOPMessage();

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

    /** send the EOMD message */
    if (udpsocket->SendTo(vcmtp_packet,VCMTP_HEADER_LEN) < 0)
        cout<<"vcmtpSendv3::sendEOPMessage::SendTo error\n";
}
