#ifndef TIMER_H_
#define TIMER_H_

#include "senderMetadata.h"

class Timer
{
public:
	Timer();
	~Timer();
	void trigger(uint32_t prodindex, senderMetadata& sendmeta);

private:
};

#endif /* TIMER_H_ */
