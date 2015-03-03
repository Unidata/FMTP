/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      senderMetadata.cpp
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Nov 28, 2014
 *
 * @section   LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or（at your option）
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details at http://www.gnu.org/copyleft/gpl.html
 *
 * @brief     Implement the interfaces and structures of VCMTPv3 sender side
 *            retransmission metadata.
 *
 * VCMTPv3 sender side retransmission metadata method functions. It supports
 * add/rm, query and modify operations.
 */


#include "senderMetadata.h"


using namespace std;

#ifndef NULL
    #define NULL 0
#endif


/**
 * Construct the senderMetadata class, initialize the index-metadata mapping
 * lock.
 *
 * @param[in] none
 */
senderMetadata::senderMetadata()
{
    pthread_rwlock_init(&indexMetaMapLock, NULL);
}


/**
 * Destruct the senderMetadata class, destroy the index-metadata mapping lock.
 * Clear the whole prodindex-metadata map.
 *
 * @param[in] none
 */
senderMetadata::~senderMetadata()
{
    for (map<uint32_t, RetxMetadata*>::iterator it = indexMetaMap.begin();
         it != indexMetaMap.end(); ++it)
    {
        delete(it->second);
        indexMetaMap.erase(it);
    }
    pthread_rwlock_destroy(&indexMetaMapLock);
}


/**
 * Fetch the requested RetxMetadata entry identified by a given prodindex. If
 * found nothing, return NULL pointer. Otherwise return the pointer to that
 * RetxMetadata struct. By default, the RetxMetadata pointer should be
 * initialized to NULL.
 *
 * @param[in] prodindex         specific product index
 */
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


/**
 * Add the new RetxMetadata entry into the prodindex-RetxMetadata map. A read/
 * write lock is added to ensure no conflict happening when adding a new entry.
 *
 * @param[in] ptrMeta           A pointer to the new RetxMetadata struct
 */
void senderMetadata::addRetxMetadata(RetxMetadata* ptrMeta)
{
    pthread_rwlock_wrlock(&indexMetaMapLock);
    indexMetaMap[ptrMeta->prodindex] = ptrMeta;
    pthread_rwlock_unlock(&indexMetaMapLock);
}


/**
 * Remove the RetxMetadata identified by a given product index. This function
 * doesn't have any read/write lock to protect. The caller needs to do all the
 * protection. It returns a boolean status value to indicate whether the remove
 * is successful or not. If successful, it's a true, otherwise it's a false.
 *
 * @param[in] prodindex         product index of the requested product
 */
bool senderMetadata::rmRetxMetadataNoLock(uint32_t prodindex)
{
    bool rmSuccess;
    map<uint32_t, RetxMetadata*>::iterator it;
    if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
    {
        delete it->second;
        indexMetaMap.erase(it);
        rmSuccess = true;
    }
    else
    {
        rmSuccess = false;
    }
    return rmSuccess;
}


/**
 * Remove the RetxMetadata identified by a given prodindex. Actually calling
 * the non-lock remove function and put a read/write lock to the map.
 *
 * @param[in] prodindex         product index of the requested product
 */
bool senderMetadata::rmRetxMetadata(uint32_t prodindex)
{
    pthread_rwlock_wrlock(&indexMetaMapLock);
    bool rmSuccess = rmRetxMetadataNoLock(prodindex);
    pthread_rwlock_unlock(&indexMetaMapLock);
    return rmSuccess;
}


/**
 * Remove the particular receiver identified by the retxsockfd from the
 * finished receiver set. And check if the set is empty after the operation.
 * If it is, then remove the whole entry from the map. Otherwise, just clear
 * that receiver.
 *
 * @param[in] prodindex         product index of the requested product
 * @param[in] retxsockfd        sock file descriptor of the retransmission tcp
 *                              connection.
 */
bool senderMetadata::clearUnfinishedSet(uint32_t prodindex, int retxsockfd)
{
    bool prodRemoved;
    map<uint32_t, RetxMetadata*>::iterator it;
    pthread_rwlock_wrlock(&indexMetaMapLock);
    if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
    {
        it->second->unfinReceivers.erase(retxsockfd);
        if (it->second->unfinReceivers.empty()) {
            prodRemoved = rmRetxMetadataNoLock(prodindex);
        }
        else {
            prodRemoved = false;
        }
    }
    else
    {
        prodRemoved = false;
    }
    pthread_rwlock_unlock(&indexMetaMapLock);
    return prodRemoved;
}
