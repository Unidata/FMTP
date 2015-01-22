#include "ProdBitMap.h"


ProdBitMap::ProdBitMap(const uint32_t bitmapsize)
{
    mapsize = bitmapsize;
    map = new std::bitset<1>[bitmapsize];
}


ProdBitMap::~ProdBitMap()
{
    delete map;
}


void ProdBitMap::set(uint32_t blockindex)
{
    /**
     * Order positions are counted from the rightmost bit,
     * which is order position 0.
     */
    map[blockindex].set(0, 1);
}


uint32_t ProdBitMap::count()
{
    uint32_t blkcnt;
    for(uint32_t i=0; i < mapsize; ++i)
    {
        blkcnt += map[i].count();
    }
    return blkcnt;
}
