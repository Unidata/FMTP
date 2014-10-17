/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpSend.cpp
 *
 * @history:
 *      Created on : Oct 14, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#include "vcmtpSend.h"
#include <endian.h>
#include <strings.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/uio.h>

using namespace std;

/**
 * Causes multicast packets to be sent to a given service address.
 *
 * @param[in] addr  Internet address in dotted decimal notation.
 * @param[in] port  Port number.
 */
void vcmtpSend::startGroup(const char* addr, unsigned port)
{
    // TODO
}

uint32_t vcmtpSend::sendProduct(void* data, size_t dataSize, void* metadata,
        unsigned metaSize)
{
    // TODO
}

/*****************************************************************************
 * Class Name: vcmtpSend
 * Function Name: vcmtpSend()
 *
 * Description: constructor, initialize the upd socket and the fileId
 *
 * Input:  u_int64_t id: 8 bytes file identifier
 * Output: none
 ****************************************************************************/
vcmtpSend::vcmtpSend(uint64_t id,
                     const char* recvAddr,
                     const unsigned short recvPort)
{
    if( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("vcmtpSend::vcmtpSend() create socket error.");
    }
    bzero(&recv_addr, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = inet_addr(recvAddr);
    recv_addr.sin_port = htons(recvPort);

    connect(sock_fd, (struct sockaddr *) &recv_addr, sizeof(recv_addr));
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
    std::cout<<"prodName= "<<prodname<<std::endl;
    //create the content of the vcmtp header
    memcpy(vcmtp_header,    &prodid, 8);
    memcpy(vcmtp_header+8,  &seqNum, 8);
    memcpy(vcmtp_header+16, &payLen, 8);
    memcpy(vcmtp_header+24, &flags,  8);
    //create the content of the BOMD
    memcpy(vcmtp_data,      &prodsize,   8);
    memcpy(vcmtp_data+8, prodname,256);
    //send the bomd message
    if (SendTo(vcmtp_packet, VCMTP_PACKET_LEN, 0) < 0)
        std::cout<<"vcmtpSend::SendBOMDMessage()::SendTo error\n";
    else
        std::cout<<"vcmtpSend::SendBOMDMessage()::SendTo success\n";
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
void vcmtpSend::sendMemData(char* data, uint64_t dataLength, string &prodName)
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
		std::cout<<"vcmtpSend::sendMemoryData: remained_size= "<<remained_size<<std::endl;

		uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;
		std::cout<<"vcmtpSend::sendMemoryData: data_size= "<<data_size<<std::endl;

		payLen = htobe64(data_size);
		seqNum = htobe64(seqNum);
	    //create the content of the vcmtp header
		memcpy(header,    &prodid, 8);
		memcpy(header+8,  &seqNum, 8);
		memcpy(header+16, &payLen, 8);
		memcpy(header+24, &flags,  8);

		if (SendData(vcmtpHeader, VCMTP_HEADER_LEN, data,data_size) < 0)
			std::cout << "VCMTPSender::sendMemoryData()::SendData() error" << std::endl;
		else
			std::cout << "VCMTPSender::sendMemoryData()::SendData() success" << std::endl;

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
    if (SendTo(vcmtp_packet,VCMTP_HEADER_LEN, 0) < 0)
        std::cout << "vcmtpSend::SendEOMDMessage()::SendTo error\n" << std::endl;
    else
        std::cout << "vcmtpSend::SendEOMDMessage()::SendTo success\n" << std::endl;
}

ssize_t vcmtpSend::SendTo(const void* buff, size_t len, int flags)
{
    return sendto(sock_fd, buff, len, flags, (struct sockaddr *) &recv_addr, sizeof(recv_addr));
}

ssize_t vcmtpSend::SendData(void* header, const size_t headerLen, void* data, const size_t dataLen)
{
    struct iovec iov[2];//vector including the two memory locations
    iov[0].iov_base = header;
    iov[0].iov_len  = headerLen;
    iov[1].iov_base = data;
    iov[1].iov_len  = dataLen;

    return writev(sock_fd, iov, 2);
}
