/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      ProdBitMap.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Jan 22, 2015
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
 * @brief     Define the interfaces of ProdBitMap class.
 *
 * A per-product bitmap class, used to track all the data blocks of a product.
 */


#ifndef VCMTP_RECEIVER_PRODBITMAP_H_
#define VCMTP_RECEIVER_PRODBITMAP_H_


#include <stdint.h>
#include <mutex>
#include <vector>


class ProdBitMap
{
public:
    ProdBitMap(const uint32_t bitmapsize);
    ~ProdBitMap();
    bool isComplete();
    void set(uint32_t blockindex);
    uint32_t getMapSize() {return mapsize;};

private:
    /* count the received data block number */
    uint32_t count();

    std::vector<bool>* map;
    uint32_t           mapsize;
    uint32_t           recvblocks;
    std::mutex         mutex;
};


#endif /* VCMTP_RECEIVER_PRODBITMAP_H_ */
