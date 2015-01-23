#ifndef PRODBITMAP_H_
#define PRODBITMAP_H_

#include <vector>
#include <stdint.h>

class ProdBitMap
{
public:
    ProdBitMap(const uint32_t bitmapsize);
    ~ProdBitMap();
    void set(uint32_t blockindex);
    uint32_t count();
    bool checkMiss();

private:
    std::vector<bool>* map;
    uint32_t           mapsize;
};

#endif /* PRODBITMAP_H_ */
