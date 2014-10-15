/*
 *faVCMTPSender.cpp
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */
#include"faVCMTPSender.h"
#include <endian.h>


/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: faVCMTPSender()
 *
 * Description: constructor, initialize the upd socket and the fileId
 *
 * Input:  u_int64_t id: 8 bytes file identifier
 * Output: none
 ****************************************************************************/
faVCMTPSender::faVCMTPSender(uint64_t id,const char* recvAddr,const ushort recvPort)
{
    updSocket = new UdpComm(recvAddr,recvPort);
    prodId=id;
	//bzero(fileName,sizeof(fileName));
	//bzero(prodId,sizeof(prodId));
	//seq_num=0;
	//fileSize=0;
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
 * Function Name: SendBOFMessage()
 *
 * Description: create the BOF message and send it to a receiver through udp socket
 *
 * Input: dataLength: size of the file, fileName: the file name
 * Output: none
 ****************************************************************************/
void faVCMTPSender::SendBOFMessage(uint64_t fileSize, const char* fileName)
{
	//strcpy(this->fileName, fileName);
	
	//this->fileName=fileName;
	//this->fileSize=fileSize;
	
	cout<<"faVCMTPSender::SendBOFMessage: "<<endl;
	cout<<"file size= "<<fileSize<<" file name = "<<fileName<<endl;
	// Send the BOF message to all receivers before starting the file transfer
	/*// create the content of the vcmtp header
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
	 msg->transfer_type = MEMORY_TO_MEMORY;
	 msg->file_size = fileSize;
	 bzero(msg->file_name, sizeof(msg->file_name));
	 strcpy(msg->file_name, fileName);
	 
	 
	 cout<<"transfer type="<< msg->transfer_type << " size of the field = "<<sizeof(msg->transfer_type)<<endl;
	 cout<<"file size ="<<msg->file_size << " size of the field = "<<sizeof(msg->file_size)<<endl;
	 cout<<"file name="<<msg->file_name<< " size of the field = "<<sizeof(msg->file_name)<<endl;
	 */
    unsigned char vcmtp_packet[VCMTP_PACKET_LEN];
    unsigned char *vcmtp_header = vcmtp_packet;
    unsigned char *vcmtp_data = vcmtp_packet + VCMTP_HEADER_LEN;
	
    bzero(vcmtp_packet, sizeof(vcmtp_packet));
	
	
    uint64_t fileid = htobe64(prodId);
    uint64_t seqNum = htobe64(0);//for the BOF sequence number is always zero
    uint64_t payLen = htobe64(1428);
    uint64_t flags = htobe64(VCMTP_BOF);
	
    uint64_t filesize = htobe64(fileSize);
	
    memcpy(vcmtp_header,    &fileid, 8);
    memcpy(vcmtp_header+8,  &seqNum, 8);
    memcpy(vcmtp_header+16, &payLen, 8);
    memcpy(vcmtp_header+24, &flags,  8);
    memcpy(vcmtp_data,    &filesize,   8);
    memcpy(vcmtp_data+8,    fileName,    256);
	
	
	//send the bof message
	//if (updSocket->SendTo(&msg_packet,VCMTP_HLEN + sizeof(VcmtpSenderMessage), 0) < 0)
	if (updSocket->SendTo(vcmtp_packet, 1460, 0) < 0)
		cout<<"faVCMTPSender::SendBOFMessage()::SendTo error\n";
	else
		cout<<"faVCMTPSender::SendBOFMessage()::SendTo success\n";
}
/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: SendBOMDMessage()
 *
 * Description: create the BOMD message and send it to a receiver through udp socket
 *
 * Input:  dataLength: size of the file, fileName: the file name
 * Output: none
 ****************************************************************************/
void faVCMTPSender::SendBOMDMessage(uint64_t prodSize, char* prodName, int sizeOfProdName)
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
	//	strcpy(prodbame,prodName);
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
        cout<<"faVCMTPSender::SendBOMDMessage()::SendTo error\n";
    else
        cout<<"faVCMTPSender::SendBOMDMessage()::SendTo success\n";
	
	
}
/*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: sendmcastUserData()
 *
 * Description: Send data from memory to a receiver through upd socket
 *
 * Input:  data: a pointer to the data in the memory, dataLength: the length of the data, &prodId: mD5checksum
 * Output: none
 ****************************************************************************/
void faVCMTPSender::sendMemoryData(void* data, uint64_t dataLength, string &prodName)
{
	SendBOMDMessage(dataLength,&prodName[0],prodName.length()); //send the BOMD before sending the data
	unsigned char vcmtpHeader[VCMTP_HEADER_LEN]; //create a vcmtp header
	//	VcmtpPacketHeader* header = (VcmtpPacketHeader*) vcmtpHeader; //create a pointer to the vcmtp header
	unsigned char* header= vcmtpHeader; 
	uint64_t seqNum= 0;
	
    //convert the variables from native binary representation to network binary representation
	uint64_t prodid = htobe64(prodId);
	//	uint64_t payLen = htobe64(VCMTP_DATA_LEN);
	uint64_t payLen;
	uint64_t flags = htobe64(VCMTP_MEM_DATA);
	
	int count = 0;// this is for testing
	
	size_t remained_size = dataLength; //to keep track of how many bytes of the whole data remain
	while (remained_size > 0) //check of there is more data to send
	{
		cout<<"faVCMTPSender::sendMemoryData: remained_size= "<<remained_size<<endl;
		
		count++;
		uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;
		cout<<"faVCMTPSender::sendMemoryData: data_size= "<<data_size<<endl;
		
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
		data = (char*)data + data_size; //move the data pointer to the beginning of the next block
		seqNum += data_size;
	}
	
    sendEOMDMessage();
    prodId++;//increment the file id to use it for the next data transmission
	
}

void faVCMTPSender::sendEOMDMessage()
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
        cout<<"faVCMTPSender::SendEOMDMessage()::SendTo error\n";
    else
        cout<<"faVCMTPSender::SendEOMDMessage()::SendTo success\n";
	
	
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

/*
 * /*****************************************************************************
 * Class Name: faVCMTPSender
 * Function Name: sendmcastUserData()
 *
 * Description: Send a file to a receiver through upd socket
 *
 * Input:  dataLength: size of the file, fileName: the file name
 * Output: none
 ****************************************************************************
 void faVCMTPSender::sendFile(uint64_t dataLength, const char* fileName)
 {
 // implement mcast mem2mem / file2file transfer function here.
 SendBOFMessage(dataLength,fileName);
 uint64_t remained_size = fileSize;
 
 // open file for reading only
 int fd = open(fileName, O_RDONLY);
 cout<<"fd = "<<fd<<endl;
 if (fd < 0)
 cout<<"VCMTPSender()::sendmcastUserData(): File open error!";
 else
 cout<<"VCMTPSender()::sendmcastUserData(): File open succeed!";
 
 
 char* buffer;
 off_t offset = 0;
 while (remained_size > 0) {
 uint map_size = remained_size < VCMTP_PACKET_LEN ? remained_size
 : VCMTP_PACKET_LEN;
 buffer = (char*) mmap(0, map_size, PROT_READ, MAP_FILE | MAP_SHARED, fd,offset);
 if (buffer == MAP_FAILED) {
 cout<<"faVCMTPSender::sendmcastUserData()::mmap() error";
 }
 
 DoMemoryTransfer(buffer, map_size, offset);
 cout<<"Send the block out "<<offset<<endl;
 
 munmap(buffer, map_size);
 
 offset += map_size;
 remained_size -= map_size;
 }
 
 }
 */



