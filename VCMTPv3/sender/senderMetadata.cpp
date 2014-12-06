#include "senderMetadata.h"
#include <iostream>

senderMetadata::senderMetadata()
{
	pthread_rwlock_init(&indexMetaMapLock, NULL);
}


senderMetadata::~senderMetadata()
{
	for (map<uint32_t, RetxMetadata*>::iterator it = indexMetaMap.begin();
		 it != indexMetaMap.end(); ++it)
	{
		delete(it->second);
		//TODO: need to erase(it) ?
	}
	pthread_rwlock_destroy(&indexMetaMapLock);
}


RetxMetadata* senderMetadata::getMetadata(uint32_t prodindex)
{
	RetxMetadata* temp = NULL;
	map<uint32_t, RetxMetadata*>::iterator it;
	pthread_rwlock_rdlock(&indexMetaMapLock);
	if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
		temp = it->second;
	pthread_rwlock_unlock(&indexMetaMapLock);
	return temp;
}


void senderMetadata::addRetxMetadata(RetxMetadata* ptrMeta)
{
	pthread_rwlock_wrlock(&indexMetaMapLock);
	indexMetaMap[ptrMeta->prodindex] = ptrMeta;
	pthread_rwlock_unlock(&indexMetaMapLock);
}


void senderMetadata::rmRetxMetadata(uint32_t prodindex)
{
	map<uint32_t, RetxMetadata*>::iterator it;
	pthread_rwlock_wrlock(&indexMetaMapLock);
	if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
	{
		delete it->second;
		indexMetaMap.erase(it);
	}
	pthread_rwlock_unlock(&indexMetaMapLock);
}


void senderMetadata::removeFinishedReceiver(uint32_t prodindex, int retxsockfd)
{
	map<uint32_t, RetxMetadata*>::iterator it;
	pthread_rwlock_wrlock(&indexMetaMapLock);
	if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
		it->second->unfinReceivers.erase(retxsockfd);
	pthread_rwlock_unlock(&indexMetaMapLock);
}


bool senderMetadata::isRetxAllFinished(uint32_t prodindex)
{
	bool isFinished;
	map<uint32_t, RetxMetadata*>::iterator it;
	pthread_rwlock_rdlock(&indexMetaMapLock);
	if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
	{
		isFinished = it->second->unfinReceivers.empty();
	}
	pthread_rwlock_unlock(&indexMetaMapLock);

	return isFinished;
}
