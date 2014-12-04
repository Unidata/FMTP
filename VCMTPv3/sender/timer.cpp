#include "timer.h"
#include <unistd.h>

Timer::Timer()
{
}


Timer::~Timer()
{
}


void Timer::trigger(uint32_t prodindex, senderMetadata* sendmeta)
{
	RetxMetadata* perProdMeta = sendmeta->getMetadata(prodindex);
	sleep(perProdMeta->timeoutSec);
	usleep(perProdMeta->timeoutuSec);
	sendmeta->rmRetxMetadata(prodindex);

	/** should time thread be destroyed by itself or by caller? */
	delete this;
}
