/*
 * VCMTPSender.cpp
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */

#include "VCMTPSender.h"

VCMTPSender::VCMTPSender(u_int32_t id)
{
	//setFileId(id);
	updSocket = 0;
	fileId=id;

	//updSocket = new UDPsocket(recvName,recvPort);
}

VCMTPSender::~VCMTPSender() {
	// TODO Auto-generated destructor stub
}

void VCMTPSender::SendMemoryData(void * data,uint64_t dataLength, const char* fileName)
{
		// Send the BOF message to all receivers before starting the file transfer
		// create the header file
			char msg_packet[500];
			VcmtpHeader* header = (VcmtpHeader*) msg_packet;
			header->session_id = fileId;
			header->seq_number = 0;
			header->data_len = sizeof(VcmtpSenderMessage);// the size of the VCMTP packet
			header->flags = VCMTP_BOF;

			//create the data block of the packet
			VcmtpSenderMessage* msg = (VcmtpSenderMessage*) (msg_packet + VCMTP_HLEN);
			//msg->session_id = fileId;
			msg->msg_type = FILE_TRANSFER_START;
			msg->data_len = dataLength;
			strcpy(msg->text, fileName);
			//msg->time_stamp = GetElapsedSeconds(global_timer);

			if (updSocket->SendTo(&msg_packet,VCMTP_HLEN + sizeof(VcmtpSenderMessage), 0,0,0) < 0)
					//SysError("VCMTPSender::SendFile()::SendData() error");
				cout<<"VCMTPSender::SendMemoryData()::SendTo error\n";
				else
					cout<<"VCMTPSender::SendMemoryData()::SendTo success\n";




}

void VCMTPSender::CreateUPDSocket(const char* recvAddr,const ushort recvPort)
{
	updSocket = new UdpComm(recvAddr,recvPort);

}

/*int VCMTPSender::getFileId() {
	return fileId;
}

void VCMTPSender::setFileId(int fileId) {
	this->fileId = fileId;
}*/
