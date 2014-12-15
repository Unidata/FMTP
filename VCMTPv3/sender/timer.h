#ifndef TIMER_H_
#define TIMER_H_

#include "senderMetadata.h"

class Timer
{
public:
	Timer(uint32_t prodindex, senderMetadata* sendmeta);
	~Timer();
    bool getRmState();

private:
	void trigger(uint32_t prodindex, senderMetadata* sendmeta);
    bool rmSuccess;
};

#endif /* TIMER_H_ */
