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


#ifndef NULL
    #define NULL 0
#endif


/**
 * Construct the senderMetadata class
 *
 * @param[in] none
 */
senderMetadata::senderMetadata()
{
}


/**
 * Destruct the senderMetadata class. Clear the whole prodindex-metadata map.
 *
 * @param[in] none
 */
senderMetadata::~senderMetadata()
{
    std::unique_lock<std::mutex> lock(indexMetaMapLock);
    for (std::map<uint32_t, RetxMetadata*>::iterator it = indexMetaMap.begin();
         it != indexMetaMap.end(); ++it) {
        delete(it->second);
    }
    indexMetaMap.clear();
}


/**
 * Add the new RetxMetadata entry into the prodindex-RetxMetadata map. A mutex
 * lock is added to ensure no conflict happening when adding a new entry.
 *
 * @param[in] ptrMeta           A pointer to the new RetxMetadata struct
 */
void senderMetadata::addRetxMetadata(RetxMetadata* ptrMeta)
{
    std::unique_lock<std::mutex> lock(indexMetaMapLock);
    indexMetaMap[ptrMeta->prodindex] = ptrMeta;
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
    std::map<uint32_t, RetxMetadata*>::iterator it;
    {
        std::unique_lock<std::mutex> lock(indexMetaMapLock);
        if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end()) {
            it->second->unfinReceivers.erase(retxsockfd);
            if (it->second->unfinReceivers.empty()) {
                prodRemoved = rmRetxMetadataNoLock(prodindex);
            }
            else {
                prodRemoved = false;
            }
        }
        else {
            prodRemoved = false;
        }
    }
    return prodRemoved;
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
    std::map<uint32_t, RetxMetadata*>::iterator it;
    {
        std::unique_lock<std::mutex> lock(indexMetaMapLock);
        if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end())
            temp = it->second;
    }
    return temp;
}


/**
 * Remove the RetxMetadata identified by a given prodindex. Actually calling
 * the non-lock remove function and put a mutex lock on the map.
 *
 * @param[in] prodindex         product index of the requested product
 */
bool senderMetadata::rmRetxMetadata(uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(indexMetaMapLock);
    bool rmSuccess = rmRetxMetadataNoLock(prodindex);
    return rmSuccess;
}


/**
 * Remove the RetxMetadata identified by a given product index. This function
 * doesn't have any mutex locks to protect. The caller needs to do all the
 * protection. It returns a boolean status value to indicate whether the remove
 * is successful or not. If successful, it's a true, otherwise it's a false.
 *
 * @param[in] prodindex         product index of the requested product
 */
bool senderMetadata::rmRetxMetadataNoLock(uint32_t prodindex)
{
    bool rmSuccess;
    std::map<uint32_t, RetxMetadata*>::iterator it;
    if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end()) {
        delete it->second;
        indexMetaMap.erase(it);
        rmSuccess = true;
    }
    else {
        rmSuccess = false;
    }
    return rmSuccess;
}
