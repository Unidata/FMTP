/*
 * ProductInfo.h
 *
 *  Created on: Nov 12, 2014
 *      Author: fatmaal-ali
 */

#ifndef PRODUCTINFO_H_
#define PRODUCTINFO_H_

#include "vcmtpBase.h"
#include <pthread.h>

class ProductInfo {
public:
	ProductInfo();
	~ProductInfo();

private:
    uint32_t   prodindex;
    uint32_t   prodsize;
    void* 	   dataptr;
    uint16_t   metasize;
    char       metadata[AVAIL_BOP_LEN];
    pthread_mutex_t numReceivLock;
    int        numOfReceivers;

};

#endif /* PRODUCTINFO_H_ */
