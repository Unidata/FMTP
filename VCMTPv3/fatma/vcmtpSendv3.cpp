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
 * @param[in] tcpAddr       TCP  address.
 * @param[in] tcpPort       TCP  port.
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
	udpsocket = 0;
	tcpsocket = 0;
	prodIndex = 0;
	product   = 0;

	pthread_mutex_init(&prod_list_mutex, 0); //for receiver retx threads

	vcmtpSendv3(tcpAddr, tcpPort, mcastAddr, mcastPort, prodIndex);
}

/**
 * This constructor should be used when sending a stream of product.
 *
 * @param[in] tcpAddr         TCP  address.
 * @param[in] tcpPort         TCP  port.
 * @param[in] mcastAddr       Multicast group address.
 * @param[in] mcastPort       Multicast group port.
 * @param[in] initProdIndex   Initial product index set by the user application
 *                            to identify the first product index of the stream.
 *
 */

//add get function for the tcp port
vcmtpSendv3::vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort,
            uint32_t     initProdIndex)
{
	prodIndex = initProdIndex;
	product   = 0;
	udpsocket = new UdpSocket(mcastAddr,mcastPort);
	tcpsocket = new TcpSocket(tcpAddr,tcpPort,this);

}

vcmtpSendv3::~vcmtpSendv3() {
	// TODO Auto-generated destructor stub
}
/**
 * Get the tcp port assigned by the operating system, this function should be used if
 * the user passed 0 as the tcpPort value to the constructor
 *
 * @return      tcp port assigned by the OS
 *
 */
ushort vcmtpSendv3::getTcpPort()
{
	return tcpsocket->getTcpPort();
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
void vcmtpSendv3::SendBOPMessage(uint32_t prodSize, void* metadata, unsigned maxMetaSize)
{
    const int PACKET_SIZE = VCMTP_HEADER_LEN + maxMetaSize + (VCMTP_DATA_LEN - AVAIL_BOP_LEN);
    unsigned char vcmtp_packet[PACKET_SIZE]; //create the vcmtp packet
    VcmtpPacketHeader* vcmtp_header = (VcmtpPacketHeader*) vcmtp_packet; //create a vcmtp header pointer that point at the beginning of the vcmtp packet
    VcmtpBOPMessage*   vcmtp_data   = (VcmtpBOPMessage*)   (vcmtp_packet + VCMTP_HEADER_LEN); //create vcmtp data pointer that points to the location just after the header

    bzero(vcmtp_packet, sizeof(vcmtp_packet)); //clear up the vcmtp packet

    //convert the variables from native binary  to network binary representation
    uint32_t prodindex   = htonl(prodIndex);
    uint32_t seqNum      = htonl(0);//for BOP sequence number is always zero
    uint16_t payLen      = htons(maxMetaSize+(VCMTP_DATA_LEN - AVAIL_BOP_LEN));
    uint16_t flags       = htons(VCMTP_BOP);
    uint32_t prodsize    = htonl(prodSize);
    uint16_t maxmetasize = htons(maxMetaSize);

    /*create the content of the vcmtp header*/
   memcpy(&vcmtp_header->prodindex,   &prodindex, 4);//fix these offset numbers
   memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
   memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
   memcpy(&vcmtp_header->flags,       &flags,     2);

   /*create the content of the BOP*/
   memcpy(&vcmtp_data->prodsize, &prodsize,             4);
   memcpy(&vcmtp_data->metasize, &maxmetasize,          2);
   memcpy(&vcmtp_data->metadata, metadata,        maxMetaSize);


   /*send the bomd message*/
   if (udpsocket->SendTo(vcmtp_packet,PACKET_SIZE) < 0)
	   cout<<"vcmtpSendv3::SendBOPMessage::SendTo error\n";

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
	/*if the user passed a metadata greater than the maximum available size for
	 * the metadate, the size will be set to the maximum available size for the metadata
	 */
	uint16_t maxMetaSize = metaSize > AVAIL_BOP_LEN ? AVAIL_BOP_LEN : metaSize;

	/*add the product index to the list, and initialize it to the number of receivers*/
	//prodEndRetxList[prodIndex] = tcpsocket->getNumOfReceivers();

	/*TODO change the list to store a map of productInfo class instead of prodInfo structure, I've already start changing this,
	 * need to go through the whole code and change from the structure to the class.
	 * also there is no need to the prodEndRetxList just use NumOfReceivers of the class to keep track of
	 * the numbers of receivers associated with each productIndex
	 */
	//store the information of the product
	productList[prodIndex].dataptr   = data;
	productList[prodIndex].prodindex = prodIndex;
	productList[prodIndex].prodsize  = dataSize;
	productList[prodIndex].metasize  = maxMetaSize;
	memcpy(productList[prodIndex].metadata, metadata, maxMetaSize);
	productList[prodIndex].numOfReceivers  =tcpsocket->getNumOfReceivers();

	SendBOPMessage(dataSize, metadata, maxMetaSize);
	unsigned char vcmtpHeader[VCMTP_HEADER_LEN]; //create a vcmtp header
	VcmtpPacketHeader* header = (VcmtpPacketHeader*) vcmtpHeader;

	uint32_t prodindex = htonl(prodIndex); // change the name to prodIndex
	uint16_t flags     = htons(VCMTP_MEM_DATA);
	uint16_t payLen;
	uint32_t seqNum    = 0; //The first data packet will always start with block seqNum =0;

	size_t remained_size = dataSize; //to keep track of how many bytes of the whole data remain
	while (remained_size > 0) //check if there is more data to send
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
    uint32_t prodindex = htonl(prodIndex);
    uint32_t seqNum    = htonl(0); //seqNum for the EOP should always be zero
    uint16_t payLen    = htons(0);
    uint16_t flags     = htons(VCMTP_EOP);
    //create the content of the vcmtp header
    memcpy(&vcmtp_header->prodindex,   &prodindex, 4);
    memcpy(&vcmtp_header->seqnum,      &seqNum,    4);
    memcpy(&vcmtp_header->payloadlen,  &payLen,    2);
    memcpy(&vcmtp_header->flags,       &flags,     2);

	//send the eomd message
    if (udpsocket->SendTo(vcmtp_packet,VCMTP_HEADER_LEN) < 0)
        cout<<"vcmtpSendv3::sendEOPMessage::SendTo error\n";
  }

/**
 * Receiver-specific retransmission thread, receive and handle messages from the receiver
 * through a tcp socket. 3 types of messages:
 * VCMTP_RETX_REQ : send the missing blocks to the receiver
 * VCMTP_BOP_REQ  : send the missed POB message to the receiver
 * VCMTP_RETX_END : the receiver received all the blocks of a specific product,
 *                  update the list, if all the receivers sent VCMTP_RETX_END
 *                  or the timer of this product expired (whichever comes first),
 *                  remove the product from the product list and notify the user
 *                  application
 *
 * @param[in] sock    the file descriptor of the tcp socket where the
 *                    receiver is connected
 */
void vcmtpSendv3::RunRetransThread(int sock) {


	/* to save the product information requesteb by the receiver*/
	uint32_t reqProdIndex;
	uint32_t reqSeqNum;
	uint16_t flags;
	uint16_t payLen;

	uint32_t startpos;
	uint16_t length;

	map<uint32_t , prodInfo>::iterator it;

	void *prodPtr;

	/* to receive the message from the socket*/
	char recv_buf[VCMTP_HEADER_LEN];
	VcmtpHeader* recv_header = (VcmtpHeader*)recv_buf;

	/* to send a message to the socket*/
	char send_buf[VCMTP_HEADER_LEN];
	VcmtpHeader* send_header = (VcmtpHeader*)send_buf;

	while (true) {
		if (tcpsocket->Receive(sock, recv_header, VCMTP_HEADER_LEN) <= 0) {
			cout<<"VCMTPSender::RunRetransThread()::receive header error"<<endl;
		}

		/* Handle a retransmission request */
		if (ntohs(recv_header->flags) & VCMTP_RETX_REQ) {

			char recv_retx_msg_buf[RETX_REQ_LEN];
			VcmtpRetxReqMessage* recv_retx_msg = (VcmtpRetxReqMessage*)recv_retx_msg_buf;

			if (tcpsocket->Receive(sock, recv_retx_msg_buf, RETX_REQ_LEN ) <= 0) {
				cout<<"VCMTPSender::RunRetransThread()::receive RetxReq message constant error"<<endl;
				continue;
			}
			reqProdIndex = ntohl(recv_header->prodindex);

			if ( (productList.find(reqProdIndex)) == productList.end()){
				cout << "Error: could not find the requested product"<< endl;
				continue;
			}
			//TODO: else check the time out

			reqSeqNum=ntohl(recv_retx_msg->startpos);
			length = ntohs(recv_retx_msg->length);

			prodPtr=productList[reqProdIndex].dataptr;
			prodPtr=(char*)prodPtr+reqSeqNum; /*move the pointer to the location of the missed block*/


			size_t remained_size = length; /*to keep track of how many bytes of the whole data remain*/
			while (remained_size > 0) /*check if there is more data to send*/
			{
				uint data_size = remained_size < VCMTP_DATA_LEN ? remained_size: VCMTP_DATA_LEN;

				//TODO fix this , remove the copy to the local variable and eliminate memcpy
				send_header->prodindex  = htonl(reqProdIndex);
				send_header->seqnum     = htonl(reqSeqNum);
				send_header->payloadlen = htons(data_size);
				send_header->flags      = htons(VCMTP_MEM_DATA);

				/*
				reqProdIndex = htonl(reqProdIndex);
				reqSeqNum    = htonl(reqSeqNum);
				flags        = htons(VCMTP_MEM_DATA);
				payLen       = htons(data_size);
				//create the content of the vcmtp header
				memcpy(&send_header->prodindex,   &reqProdIndex, 4);
				memcpy(&send_header->seqnum,      &reqSeqNum,    4);
				memcpy(&send_header->payloadlen,  &payLen,    2);
				memcpy(&send_header->flags,       &flags,     2);
				*/

				if (tcpsocket->SendData(send_header, VCMTP_HEADER_LEN, prodPtr,data_size,sock) < 0)
							cout<<"vcmtpSendv3::RunRetransThread::SendData() error"<<endl;

				remained_size -= data_size;
				prodPtr = (char*)prodPtr + data_size; /*move the data pointer to the beginning of the next block*/
				reqSeqNum += data_size;
			}
		}

		else if (htons(recv_header->flags) & VCMTP_BOP_REQ)
		{
			char bop_buf[(productList[reqProdIndex].metasize) + (VCMTP_DATA_LEN - AVAIL_BOP_LEN)];
		    VcmtpBOPMessage*   send_pob   = (VcmtpBOPMessage*)bop_buf ;

		    /*content of the header*/
			send_header->prodindex  = htonl(reqProdIndex);
			send_header->seqnum     = htonl(0);
			send_header->payloadlen = htons(productList[reqProdIndex].metasize+(VCMTP_DATA_LEN - AVAIL_BOP_LEN));
			send_header->flags      = htons(VCMTP_BOP );

			/*content of the BOP message*/
			memcpy(&send_pob->prodsize, &productList[reqProdIndex].prodsize,             4);
			memcpy(&send_pob->metasize, &productList[reqProdIndex].metasize,             2);
			memcpy(&send_pob->metadata,  productList[reqProdIndex].metadata,        productList[reqProdIndex].metasize);

			if (tcpsocket->SendData(send_header, VCMTP_HEADER_LEN, send_pob,(productList[reqProdIndex].metasize) + (VCMTP_DATA_LEN - AVAIL_BOP_LEN),sock) < 0)
				cout<<"vcmtpSendv3::RunRetransThread::SendData() error"<<endl;
		}

		/*TODO need to fix this part of code to use the ProductInfo class and
		 * there is no need to the prodEndRetxList just use the class
		 */
		else if (htons(recv_header->flags) & VCMTP_RETX_END) {/* remove the productinfo from the list */

			//lock
			/*decrement the number of receivers for this specific prodIndex*/
			prodEndRetxList[productList[reqProdIndex]]--;
			//unlock

			/* check if the number of receivers for this specific
			 * prodIndex reaches zero, delete it and the notify the user
			 * application*/
			//TODO fix this to use the productInfo class
			if(prodEndRetxList[productList[reqProdIndex]] == 0)
			{
				pthread_mutex_lock(&prod_list_mutex);
				it=productList.find(htons(recv_header->prodindex));
				productList.erase(it);
				pthread_mutex_unlock(&prod_list_mutex);

				//TODO: notify ldm
			}
		}
	}
}



