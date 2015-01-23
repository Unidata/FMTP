#ifndef PRODBITMAP_H_
#define PRODBITMAP_H_

#include <vector>
#include <stdint.h>
#include <mutex>

class ProdBitMap
{
public:
    ProdBitMap(const uint32_t bitmapsize);
    ~ProdBitMap();
    void set(uint32_t blockindex);
    bool isComplete();

private:
    std::vector<bool>* map;
    uint32_t           mapsize;
    uint32_t		   recvblocks;
    std::mutex		   mutex;

    uint32_t count();
};

#endif /* PRODBITMAP_H_ */
