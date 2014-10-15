/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpSend.h
 *
 * @history:
 *      Created on : Oct 14, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#include "vcmtpSend.h"
#include <endian.h>


/*****************************************************************************
 * Class Name: vcmtpSend
 * Function Name: vcmtpSend()
 *
 * Description: constructor, initialize the upd socket and the fileId
 *
 * Input:  u_int64_t id: 8 bytes file identifier
 * Output: none
 ****************************************************************************/
vcmtpSend::vcmtpSend(uint64_t id,const char* recvAddr,const ushort recvPort)
{
    updSocket = new UdpComm(recvAddr,recvPort);
    prodId=id;
}

/*****************************************************************************
 * Class Name: vcmtpSend
 * Function Name: ~vcmtpSend()
 *
 * Description: destructor, do nithing for now
 *
 * Input:  none
 * Output: none
 ****************************************************************************/
vcmtpSend::~vcmtpSend()
{
}

/*****************************************************************************
 * Class Name: vcmtpSend
 * Function Name: SendBOMDMessage()
 *
 * Description: create the BOMD message and send it to a receiver through udp socket
 *
 * Input:  dataLength: size of the file, fileName: the file name
 * Output: none
 ****************************************************************************/
void vcmtpSend::SendBOMDMsg(uint64_t prodSize, char* prodName, int sizeOfProdName)
{
    unsigned char vcmtp_packet[VCMTP_PACKET_LEN]; //create the vcmtp packet
    unsigned char *vcmtp_header = vcmtp_packet; //create a vcmtp header pointer that point at the beginning of the vcmtp packet
    unsigned char *vcmtp_data = vcmtp_packet + VCMTP_HEADER_LEN; //create vcmtp data pointer that points to the location just after the hea

    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint64_t prodid = htobe64(prodId);
    uint64_t seqNum = htobe64(0);//for the BOMD sequence number is always zero
    uint64_t payLen = htobe64(VCMTP_DATA_LEN);
    uint64_t flags = htobe64(VCMTP_BOMD);

    uint64_t prodsize = htobe64(prodSize);

    char prodname[256];
    bzero(prodname,sizeof(prodname));
    strncpy(prodname,prodName,sizeOfProdName);
    cout<<"prodName= "<<prodname<<endl;
    //create the content of the vcmtp header
    memcpy(vcmtp_header,    &prodid, 8);
    memcpy(vcmtp_header+8,  &seqNum, 8);
    memcpy(vcmtp_header+16, &payLen, 8);
    memcpy(vcmtp_header+24, &flags,  8);
    //create the content of the BOMD
    memcpy(vcmtp_data,      &prodsize,   8);
    memcpy(vcmtp_data+8, prodname,256);
    //send the bomd message
    if (updSocket->SendTo(vcmtp_packet, VCMTP_PACKET_LEN, 0) < 0)
        cout<<"vcmtpSend::SendBOMDMessage()::SendTo error\n";
    else
        cout<<"vcmtpSend::SendBOMDMessage()::SendTo success\n";
}

/*****************************************************************************
 * Class Name: vcmtpSend
 * Function Name: sendMemData()
 *
 * Description: Send data from memory to a receiver through upd socket
 *
 * Input:  data: a pointer to the data in the memory, dataLength: the length of the data, &prodId: mD5checksum
 * Output: none
 ****************************************************************************/
void vcmtpSend::sendMemData(void* data, uint64_t dataLength, string &prodName)
{
	SendBOMDMsg(dataLength,&prodName[0],prodName.length()); //send the BOMD before sending the data
	unsigned char vcmtpHeader[VCMTP_HEADER_LEN]; //create a vcmtp header
	unsigned char* header= vcmtpHeader;
	uint64_t seqNum= 0;

    //convert the variables from native binary representation to network binary representation
	uint64_t prodid = htobe64(prodId);
	uint64_t payLen;
	uint64_t flags = htobe64(VCMTP_MEM_DATA);

	size_t remained_size = dataLength; //to keep track of how many bytes of the whole data remain
	while (remained_size > 0) //check of there is more data to send
	{
		cout<<"vcmtpSend::sendMemoryData: remained_size= "<<remained_size<<endl;

		uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;
		cout<<"vcmtpSend::sendMemoryData: data_size= "<<data_size<<endl;

		payLen = htobe64(data_size);
		seqNum = htobe64(seqNum);
	    //create the content of the vcmtp header
		memcpy(header,    &prodid, 8);
		memcpy(header+8,  &seqNum, 8);
		memcpy(header+16, &payLen, 8);
		memcpy(header+24, &flags,  8);

		if (updSocket->SendData(vcmtpHeader, VCMTP_HEADER_LEN, data,data_size) < 0)
			cout<<"VCMTPSender::sendMemoryData()::SendData() error"<<endl;
		else
			cout<<"VCMTPSender::sendMemoryData()::SendData() success, count= "<<count<<endl;

		remained_size -= data_size;
		data += data_size; //move the data pointer to the beginning of the next block
		seqNum += data_size;
	}

    sendEOMDMsg();
    prodId++;//increment the file id to use it for the next data transmission
}


void vcmtpSend::sendEOMDMsg()
{
    unsigned char vcmtp_packet[VCMTP_HEADER_LEN];
    unsigned char *vcmtp_header = vcmtp_packet;

    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint64_t prodid = htobe64(prodId);
    uint64_t seqNum = htobe64(0);
    uint64_t payLen = htobe64(0);
    uint64_t flags = htobe64(VCMTP_EOMD);

    //create the content of the vcmtp header
    memcpy(vcmtp_header,    &prodid, 8);
    memcpy(vcmtp_header+8,  &seqNum, 8);
    memcpy(vcmtp_header+16, &payLen, 8);
    memcpy(vcmtp_header+24, &flags,  8);
	//send the eomd message
    if (updSocket->SendTo(vcmtp_packet,VCMTP_HEADER_LEN, 0) < 0)
        cout<<"vcmtpSend::SendEOMDMessage()::SendTo error\n";
    else
        cout<<"vcmtpSend::SendEOMDMessage()::SendTo success\n";
}

/*****************************************************************************
 * Class Name: vcmtpSend
 * Function Name: CreateUDPSocket()
 *
 * Description: create a upd socket
 *
 * Input:  recvAddr: the address of the receiver, recvPort: the port of the receiver
 * Output: none
 ****************************************************************************/
void vcmtpSend::CreateUDPSocket(const char* recvAddr,const ushort recvPort)
{
	updSocket = new UdpComm(recvAddr,recvPort);
}
