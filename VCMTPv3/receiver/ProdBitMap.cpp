/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      ProdBitMap.cpp
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
 * @brief     Implement the interfaces of ProdBitMap class.
 *
 * A per-product bitmap class, used to track all the data blocks of a product.
 */


#include "ProdBitMap.h"


/**
 * Constructor of the ProdBitMap class. It creates a new bitmap according to
 * the given bitmap size, and as a matter of fact, it's using the vector<bool>
 * data structure to implement the bitmap.
 *
 * @param[in] bitmapsize        Number of bits that needs to be created in the
 *                              new bitmap.
 */
ProdBitMap::ProdBitMap(const uint32_t bitmapsize) : recvblocks(0), mutex()
{
    mapsize = bitmapsize;
    map = new std::vector<bool>(bitmapsize, false);
}


/**
 * Destructor of the ProdBitMap class.
 *
 * @param[in] none
 */
ProdBitMap::~ProdBitMap()
{
    delete map;
}


/**
 * Use the given block index to set the corresponding bit. Meanwhile, keep
 * updating the received block counter.
 *
 * @param[in] blockindex        The index of the block in the bitmap.
 */
void ProdBitMap::set(uint32_t blockindex)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!map->at(blockindex))
    {
        map->at(blockindex) = true;
        recvblocks++;
    }
}


/**
 * Queries the number of received blocks.
 *
 * @param[in] none
 */
uint32_t ProdBitMap::count()
{
    return recvblocks;
}


/**
 * Checks if all the bits, which represents the blocks are set.
 *
 * @param[in] none
 */
bool ProdBitMap::isComplete()
{
    std::unique_lock<std::mutex> lock(mutex);
    return (count() == mapsize);
}
