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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 * @return    True if RetxMetadata is removed, otherwise false.
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
                if (it->second->inuse) {
                    it->second->remove = true;
                }
                else {
                    it->second->~RetxMetadata();
                    indexMetaMap.erase(it);
                }
                prodRemoved = true;
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
 * @return    A pointer to the original RetxMetadata in the map.
 */
RetxMetadata* senderMetadata::getMetadata(uint32_t prodindex)
{
    RetxMetadata* temp = NULL;
    std::map<uint32_t, RetxMetadata*>::iterator it;
    {
        std::unique_lock<std::mutex> lock(indexMetaMapLock);
        if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end()) {
            temp = it->second;
            /* sets up exclusive flag to acquire this RetxMetadata */
            it->second->inuse = true;
        }
    }
    return temp;
}


/**
 * Releases the acquired RetxMetadata. If it is marked as in use, reset
 * the in use flag. If it is marked as remove, remove it correspondingly.
 *
 * @param[in] prodindex         product index of the requested product
 * @return    True if release operation is successful, otherwise false.
 */
bool senderMetadata::releaseMetadata(uint32_t prodindex)
{
    bool relstate;
    std::unique_lock<std::mutex> lock(indexMetaMapLock);
    std::map<uint32_t, RetxMetadata*>::iterator it;
    if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end()) {
        if (it->second->inuse) {
            it->second->inuse = false;
        }
        if (it->second->remove) {
            it->second->~RetxMetadata();
            indexMetaMap.erase(it);
        }
        relstate = true;
    }
    else {
        relstate = false;
    }
    return relstate;
}


/**
 * Remove the RetxMetadata identified by a given prodindex. Actually calling
 * the non-lock remove function and put a mutex lock on the map.
 *
 * @param[in] prodindex         product index of the requested product
 * @return    True if removal is successful, otherwise false.
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
 * @return    True if removal is successful, otherwise false.
 */
bool senderMetadata::rmRetxMetadataNoLock(uint32_t prodindex)
{
    bool rmSuccess;
    std::map<uint32_t, RetxMetadata*>::iterator it;
    if ((it = indexMetaMap.find(prodindex)) != indexMetaMap.end()) {
        /**
         * The deletion could either be an immediate one or a delayed one.
         * Either case the removal can be considered successful as long as
         * next call guarantees to delete it when it sees the remove flag.
         */
        if (it->second->inuse) {
            it->second->remove = true;
        }
        else {
            it->second->~RetxMetadata();
            indexMetaMap.erase(it);
        }
        rmSuccess = true;
    }
    else {
        rmSuccess = false;
    }
    return rmSuccess;
}
