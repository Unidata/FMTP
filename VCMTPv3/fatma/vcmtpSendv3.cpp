/*
 *vcmtpSendv3.cpp
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#include "vcmtpSendv3.h"

/**
 * This constructor should be used when sending any memory data.
 *
 * @param[in] tcpAddr       TCP server address.
 * @param[in] tcpPort       TCP server port.
 * @param[in] mcastAddr     Multicast group address.
 * @param[in] mcastPort     Multicast group port.
 *
 */
vcmtpSendv3::vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort)
{
	udpsocket=0;
	prodIndex=0;
	vcmtpSendv3(tcpAddr, tcpPort, mcastAddr, mcastPort, prodIndex);
}

/**
 * This constructor should be used when sending a stream of product.
 *
 * @param[in] tcpAddr         TCP server address.
 * @param[in] tcpPort         TCP server port.
 * @param[in] mcastAddr       Multicast group address.
 * @param[in] mcastPort       Multicast group port.
 * @param[in] initProdIndex   Initial product index set by the user application
 *                            to identify the first product index of the stream.
 *
 */
vcmtpSendv3::vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort,
            uint32_t     initProdIndex)
{
	prodIndex=initProdIndex;
	udpsocket=new UdpSocket(mcastAddr,mcastPort);
	// TODO: use `tcpAddr` and `tcpPort`
}

vcmtpSendv3::~vcmtpSendv3() {
	// TODO Auto-generated destructor stub
}
/**
 * Send the BOP message to the receiver.
 *
 * @param[in] prodSize       The size of the product.
 * @param[in] metadata       metadata  Application-specific metadata to be sent before the
 *                           data. May be 0, in which case no metadata is sent.
 * @param[in] metaSize       Size of the metadata in bytes. Must be less than or equals
 *                           1442. May be 0, in which case no metadata is sent.
 *
 */
void vcmtpSendv3::SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize)
{
    uint16_t maxMetaSize = metaSize>AVAIL_BOP_LEN?AVAIL_BOP_LEN:metaSize;
    //const int PACKET_SIZE = VCMTP_HEADER_LEN + maxMetaSize + 6;

    const int PACKET_SIZE = VCMTP_HEADER_LEN + maxMetaSize + (VCMTP_DATA_LEN - AVAIL_BOP_LEN);
    unsigned char vcmtp_packet[PACKET_SIZE]; //create the vcmtp packet
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet; //create a vcmtp header pointer that point at the beginning of the vcmtp packet
    VcmtpBOPMessage*   vcmtp_data   = (VcmtpBOPMessage*)   (vcmtp_packet + VCMTP_HEADER_LEN); //create vcmtp data pointer that points to the location just after the header

    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint32_t prodindex   = htobe32(prodIndex);
    uint32_t seqNum      = htobe32(0);//for BOP sequence number is always zero
    uint16_t payLen      = htobe16(maxMetaSize+(VCMTP_DATA_LEN - AVAIL_BOP_LEN));
    uint16_t flags       = htobe16(VCMTP_BOP);
    uint32_t prodsize    = htobe32(prodSize);
    uint16_t maxmetasize = htobe16(maxMetaSize);

    char meta[AVAIL_BOP_LEN];
    bzero(meta,sizeof(meta));
    strncpy(meta,(char*)metadata,maxMetaSize);

    //create the content of the vcmtp header
   memcpy(&vcmtp_header->prodindex,   &prodindex, 4);//fix these offset numbers
   memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
   memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
   memcpy(&vcmtp_header->flags,       &flags,     2);

   //create the content of the BOP
   memcpy(&vcmtp_data->prodsize, &prodsize,             4);
   memcpy(&vcmtp_data->metasize, &maxmetasize,          2);
   memcpy(&vcmtp_data->metadata, meta,        maxMetaSize);

   //send the bomd message
   if (udpsocket->SendTo(vcmtp_packet,PACKET_SIZE) < 0)
	   cout<<"vcmtpSendv3::SendBOPMessage::SendTo error\n";
   //else
	   //cout<<"vcmtpSendv3::SendBOPMessage::SendTo success\n";

}

/**
 * Transfers a contiguous block of memory.
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
 * Transfers Application-specific metadata and a contiguous block of memory.
 *
 * @param[in] data      Memory data to be sent.
 * @param[in] dataSize  Size of the memory data in bytes.
 * @param[in] metadata  Application-specific metadata to be sent before the
 *                      data. May be 0, in which case no metadata is sent.
 * @param[in] metaSize  Size of the metadata in bytes. Must be less than or
 *                      equal 1442 bytes. May be 0, in which case no metadata is sent.
 * @return              Index of the product.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, size_t dataSize, void* metadata, unsigned metaSize)
{

	SendBOPMessage(dataSize, metadata, metaSize);
	unsigned char vcmtpHeader[VCMTP_HEADER_LEN]; //create a vcmtp header
	VcmtpPacketHeader* header = (VcmtpPacketHeader*) vcmtpHeader;

	uint32_t prodindex = htobe32(prodIndex); // change the name to prodIndex
	uint16_t flags     = htobe16(VCMTP_MEM_DATA);
	uint16_t payLen;
	uint32_t seqNum    = 0; //The first data packet will always start with block seqNum =0;

	size_t remained_size = dataSize; //to keep track of how many bytes of the whole data remain
	while (remained_size > 0) //check if there is more data to send
	{
		uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;

		payLen = htobe16(data_size);
		seqNum = htobe32(seqNum);

		memcpy(&header->prodindex,  &prodindex, 4);
		memcpy(&header->seqnum,     &seqNum,    4);
		memcpy(&header->payloadlen, &payLen,    2);
		memcpy(&header->flags,      &flags,     2);

		if (udpsocket->SendData(vcmtpHeader, VCMTP_HEADER_LEN, data,data_size) < 0)
			cout<<"vcmtpSendv3::sendProduct::SendData() error"<<endl;
		//else
		//	cout<<"vcmtpSendv3::sendProduct::SendData() success"<<endl;

		remained_size -= data_size;
		data = (char*)data + data_size; //move the data pointer to the beginning of the next block
		seqNum += data_size;
	}

	sendEOPMessage();
	prodIndex++;//increment the file id to use it for the next data transmission

	return prodIndex-1;
}
/**
 * Send the EOP message to the receiver to identify
 * the end of a product transmission
 *
 */
void vcmtpSendv3::sendEOPMessage()
{
    unsigned char vcmtp_packet[VCMTP_HEADER_LEN];
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet;
    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint32_t prodindex = htobe32(prodIndex);
    uint32_t seqNum    = htobe32(0); //seqNum for the EOP should always be zero
    uint16_t payLen    = htobe16(0);
    uint16_t flags     = htobe16(VCMTP_EOP);
    //create the content of the vcmtp header
    memcpy(&vcmtp_header->prodindex,   &prodindex, 4);
    memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
    memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
    memcpy(&vcmtp_header->flags,       &flags,     2);

	//send the eomd message
    if (udpsocket->SendTo(vcmtp_packet,VCMTP_HEADER_LEN) < 0)
        cout<<"vcmtpSendv3::sendEOPMessage::SendTo error\n";
    //else
    //    cout<<"vcmtpSendv3::sendEOPMessage::SendTo success\n";
}




