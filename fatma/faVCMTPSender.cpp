/*
 * faVCMTPSender.cpp
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */

#include "faVCMTPSender.h"
/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: faVCMTPSender()
 *
 * Description: constructor, initialize the upd socket and the fileId
 *
 * Input:  u_int64_t id: 8 bytes file identifier
 * Output: none
 ****************************************************************************/
faVCMTPSender::faVCMTPSender(u_int64_t id)
{
	updSocket = 0;
	fileId=id;
}
/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: ~faVCMTPSender()
 *
 * Description: destructor, do nithing for now
 *
 * Input:  none
 * Output: none
 ****************************************************************************/
faVCMTPSender::~faVCMTPSender() {
	// TODO Auto-generated destructor stub
}
/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: SendBOFMessage
 *
 * Description: create the BOF message and send it to a receiver through udp socket
 *
 * Input:  dataLength: size of the file, fileName: the file name
 * Output: none
 ****************************************************************************/
void faVCMTPSender::SendBOFMessage(uint64_t dataLength, const char* fileName)
{
	cout<<"faVCMTPSender::SendBOFMessage: "<<endl;
	cout<<"file size= "<<dataLength<<" file name = "<<fileName<<endl;
	// Send the BOF message to all receivers before starting the file transfer
	// create the content of the vcmtp header
	char msg_packet[1460];
	VcmtpHeader* header = (VcmtpHeader*) msg_packet;
	header->file_id = fileId;//send the file_id to the fileId passed to the sendBOFMessage
	header->seq_number = 0;// the sequence number for the bof message is always zero
	header->vcmtp_payload_size = 1428;
	header->flags = VCMTP_BOF;

	cout<<"file id="<<	header->file_id << " size of the field = "<<sizeof(header->file_id)<<endl;
	cout<<"seq_num=  "<<	header->seq_number<< " size of the field = "<<sizeof(header->seq_number)<<endl;
	cout<<"vcmtp payload size = "<<	header->vcmtp_payload_size << " size of the field = "<<sizeof(header->vcmtp_payload_size)<<endl;
	cout<<"flags="<< header->flags<< " size of the field = "<<sizeof(header->flags)<<endl;

	//create the content of the BOF message
	VcmtpSenderMessage* msg = (VcmtpSenderMessage*) (msg_packet + VCMTP_HLEN);
	msg->transfer_type =MEMORY_TO_MEMORY;
	msg->file_size = dataLength;
	strcpy(msg->file_name, fileName);

	cout<<"transfer type="<<	msg->transfer_type<< " size of the field = "<<sizeof(msg->transfer_type)<<endl;
	cout<<"file size ="<<msg->file_size << " size of the field = "<<sizeof(msg->file_size)<<endl;
	cout<<"file name="<<msg->file_name<< " size of the field = "<<sizeof(msg->file_name)<<endl;


	//send the bof message
	if (updSocket->SendTo(&msg_packet,VCMTP_HLEN + sizeof(VcmtpSenderMessage), 0) < 0)
			//SysError("faVCMTPSender::SendFile()::SendData() error");
		cout<<"faVCMTPSender::SendMemoryData()::SendTo error\n";
		else
			cout<<"faVCMTPSender::SendMemoryData()::SendTo success\n";
}
/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: CreateUPDSocket()
 *
 * Description: create a upd socket
 *
 * Input:  recvAddr: the address of the receiver, recvPort: the port of the receiver
 * Output: none
 ****************************************************************************/
void faVCMTPSender::CreateUPDSocket(const char* recvAddr,const ushort recvPort)
{
	updSocket = new UdpComm(recvAddr,recvPort);

}

