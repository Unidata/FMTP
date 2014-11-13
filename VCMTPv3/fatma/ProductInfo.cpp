/*
 * ProductInfo.cpp
 *
 *  Created on: Nov 12, 2014
 *      Author: fatmaal-ali
 */

#include "ProductInfo.h"

ProductInfo::ProductInfo() {
	prodindex      = 0;
	prodsize       = 0;
	dataptr        = 0;
	metasize       = 0;
	numOfReceivers = 0;
	metadata[AVAIL_BOP_LEN]=0;
	pthread_mutex_init(&numReceivLock, 0);
}

ProductInfo::~ProductInfo() {
	// TODO Auto-generated destructor stub
}

