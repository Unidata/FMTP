#include "ProdBitMap.h"


ProdBitMap::ProdBitMap(const uint32_t bitmapsize)
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
    map->at(blockindex) = true;
}


uint32_t ProdBitMap::count()
{
    uint32_t blkcnt = 0;
    for(uint32_t i=0; i < mapsize; ++i) {
        blkcnt += (uint32_t) map->at(i);
    }

    return blkcnt;
}


bool ProdBitMap::checkMiss()
{
    if (count() != mapsize)
        return true;
    else
        return false;
}
