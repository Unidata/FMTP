#include "ProdBitMap.h"


ProdBitMap::ProdBitMap(const uint32_t bitmapsize) : recvblocks(0), mutex()
{
    mapsize = bitmapsize;
    map = new std::vector<bool>(bitmapsize, false);
}


ProdBitMap::~ProdBitMap()
{
    delete map;
}


void ProdBitMap::set(uint32_t blockindex)
{
	std::unique_lock<std::mutex> lock(mutex);
    if (!map->at(blockindex))
    {
        map->at(blockindex) = true;
        recvblocks++;
    }
}


uint32_t ProdBitMap::count()
{
    return recvblocks;
}


bool ProdBitMap::isComplete()
{
    std::unique_lock<std::mutex> lock(mutex);
    return (count() == mapsize);
}
