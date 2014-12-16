#ifndef TIMER_H_
#define TIMER_H_

#include "senderMetadata.h"

class Timer
{
public:
	Timer(uint32_t prodindex, senderMetadata* sendmeta, bool& rmState);
	~Timer();

private:
	void trigger(uint32_t prodindex, senderMetadata* sendmeta, bool& rmState);
};

#endif /* TIMER_H_ */
