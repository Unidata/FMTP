/*
*vcmtpSendv3.cpp
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#include "vcmtpSendv3.h"

vcmtpSendv3::vcmtpSendv3() {
	udpsocket=0;
	prodId=0;
}

vcmtpSendv3::~vcmtpSendv3() {
	// TODO Auto-generated destructor stub
}

void vcmtpSendv3::sendProdStream(const char* streamName, uint32_t initProdIndex)
{
}

void vcmtpSendv3::startGroup(const char* addr, const ushort port)
{
	udpsocket=new UdpSocket(addr,port);
}
uint32_t vcmtpSendv3::SendBOPMessage(uint64_t prodSize, void*metadata, unsigned metaSize)
{
	uint32_t numOfSentBytes;
    unsigned char vcmtp_packet[VCMTP_PACKET_LEN]; //create the vcmtp packet
    unsigned char *vcmtp_header = vcmtp_packet; //create a vcmtp header pointer that point at the beginning of the vcmtp packet
    unsigned char *vcmtp_data = vcmtp_packet + VCMTP_HEADER_LEN; //create vcmtp data pointer that points to the location just after the hea

    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint64_t prodid = htobe64(prodId);
    uint64_t seqNum = htobe64(0);//for BOP sequence number is always zero
    uint64_t payLen = htobe64(metaSize>(1428-10)?VCMTP_DATA_LEN:metaSize+10);
    uint64_t flags = htobe64(VCMTP_BOP);

    uint64_t prodsize = htobe64(prodSize);

    char meta[1428-10];
    int maxMetaSize = metaSize>(1428-10)?1428-10:metaSize;
    bzero(meta,sizeof(meta));
    strncpy(meta,(char*)metadata,maxMetaSize);

    //create the content of the vcmtp header
    memcpy(vcmtp_header,    &prodid, 8);
    memcpy(vcmtp_header+8,  &seqNum, 8);
    memcpy(vcmtp_header+16, &payLen, 8);
    memcpy(vcmtp_header+24, &flags,  8);
    //create the content of the BOP
    memcpy(vcmtp_data,      &prodsize,   8);
    memcpy(vcmtp_data+8, meta,maxMetaSize);
	//send the bomd message
    if (numOfSentBytes+= udpsocket->SendTo(vcmtp_packet, VCMTP_PACKET_LEN) < 0)
        cout<<"vcmtpSendv3::SendBOPMessage::SendTo error\n";
    else
        cout<<"vcmtpSendv3::SendBOPMessage::SendTo success\n";

    return numOfSentBytes;
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
 * @param[in] metaSize  Size of the metadata in bytes. Must be less than
 *                      1428. May be 0, in which case no metadata is sent.
 * @return              Index of the product.
 */
uint32_t vcmtpSendv3::sendProduct(void* data, size_t dataSize, void* metadata, unsigned metaSize)
{
	uint32_t numOfSentBytes=0;

	numOfSentBytes+= SendBOPMessage(dataSize, metadata, metaSize);
	unsigned char vcmtpHeader[VCMTP_HEADER_LEN]; //create a vcmtp header
	unsigned char* header= vcmtpHeader;

	uint64_t prodid = htobe64(prodId);
	uint64_t flags = htobe64(VCMTP_MEM_DATA);
	uint64_t payLen;
	uint64_t seqNum= 0;

	size_t remained_size = dataSize; //to keep track of how many bytes of the whole data remain
	while (remained_size > 0) //check if there is more data to send
	{
	  cout<<"vcmtpSendv3::sendProduct: remained_size= "<<remained_size<<endl;

	  uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;
	  cout<<"vcmtpSendv3::sendProduct: data_size= "<<data_size<<endl;

	  payLen = htobe64(data_size);
	  seqNum = htobe64(seqNum);
	  //create the content of the vcmtp header
	  memcpy(header,    &prodid, 8);
	  memcpy(header+8,  &seqNum, 8);
	  memcpy(header+16, &payLen, 8);
	  memcpy(header+24, &flags,  8);

	  if (numOfSentBytes+= udpsocket->SendData(vcmtpHeader, VCMTP_HEADER_LEN, data,data_size) < 0)
		  cout<<"vcmtpSendv3::sendProduct::SendData() error"<<endl;
	  else
		  cout<<"vcmtpSendv3::sendProduct::SendData() success"<<endl;

	  remained_size -= data_size;
	  data += data_size; //move the data pointer to the beginning of the next block
	  seqNum += data_size;
	}

	numOfSentBytes+=sendEOPMessage();
	prodId++;//increment the file id to use it for the next data transmission

	return 0;
}

uint32_t vcmtpSendv3::sendEOPMessage()
{
	uint32_t numOfSentBytes=0;
    unsigned char vcmtp_packet[VCMTP_HEADER_LEN];
    unsigned char *vcmtp_header = vcmtp_packet;

    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint64_t prodid = htobe64(prodId);
    uint64_t seqNum = htobe64(0);
    uint64_t payLen = htobe64(0);
    uint64_t flags = htobe64(VCMTP_EOP);

    //create the content of the vcmtp header
    memcpy(vcmtp_header,    &prodid, 8);
    memcpy(vcmtp_header+8,  &seqNum, 8);
    memcpy(vcmtp_header+16, &payLen, 8);
    memcpy(vcmtp_header+24, &flags,  8);
        //send the eomd message
    if (numOfSentBytes+= udpsocket->SendTo(vcmtp_packet,VCMTP_HEADER_LEN) < 0)
        cout<<"vcmtpSendv3::sendEOPMessage::SendTo error\n";
    else
        cout<<"vcmtpSendv3::sendEOPMessage::SendTo success\n";

    return numOfSentBytes;


}


