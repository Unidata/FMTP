/*
 *vcmtpSendv3.h
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#ifndef VCMTPSENDV3_H_
#define VCMTPSENDV3_H_


#include <sys/types.h>
#include "UdpSocket.h"
#include "TcpSocket.h"
#include "ProductInfo.h"
#include "vcmtpBase.h"
#include <pthread.h>
#include <map>

typedef struct prodInfo {
    uint32_t   prodindex;
    uint32_t   prodsize;
    void* 	   dataptr;
    uint16_t   metasize;
    char       metadata[AVAIL_BOP_LEN];
    pthread_mutex_t numReceivLock;
    int        numOfReceivers;

} prodinfo;

class vcmtpSendv3 {
friend class TcpSocket;
public:
    vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort);
    vcmtpSendv3(
            const char*  tcpAddr,
            const ushort tcpPort,
            const char*  mcastAddr,
            const ushort mcastPort,
            uint32_t     initProdIndex);

    ~vcmtpSendv3();

    uint32_t sendProduct(void* data, size_t dataSize);
    uint32_t sendProduct(void* data, size_t dataSize, void* metadata,
            unsigned metaSize);
	ushort getTcpPort();


private:
    uint32_t prodIndex;
    UdpSocket* udpsocket;
    TcpSocket* tcpsocket;
    //prodInfo* product;
    ProductInfo* product;
    //map<uint32_t , prodInfo> productList; //map the prodIndex requested by the receiver through tcp connection to the productInfo
    map<uint32_t , ProductInfo> productList;
    //TODO find a better name for this list
    map<uint32_t , int> prodEndRetxList; //map roductIndex to a counter of the number of receivers for this specific prodIndex
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    void sendEOPMessage();
    static void RunRetransThread(int sock);

	pthread_mutex_t prod_list_mutex;


};

#endif /* VCMTPSENDV3_H_ */



