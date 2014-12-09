#include "timer.h"
#include <unistd.h>


Timer::Timer(uint32_t prodindex, senderMetadata* sendmeta)
{
	trigger(prodindex, sendmeta);
}


Timer::~Timer()
{
}


void Timer::trigger(uint32_t prodindex, senderMetadata* sendmeta)
{
	RetxMetadata* perProdMeta = sendmeta->getMetadata(prodindex);
	if (perProdMeta != NULL)
	{
		sleep(perProdMeta->timeoutSec);
		usleep(perProdMeta->timeoutuSec);
		sendmeta->rmRetxMetadata(prodindex);
	}
	else
		;// throw an error here
}
