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
    unsigned int timeoutSec, timeoutuSec;
	RetxMetadata* perProdMeta = sendmeta->getMetadata(prodindex);
	if (perProdMeta != NULL)
	{
	    timeoutSec = (unsigned int) perProdMeta->retxTimeoutPeriod;
	    timeoutuSec = (unsigned int) (perProdMeta->retxTimeoutPeriod -
	                                    timeoutSec) * 1000000;
		sleep(timeoutSec);
		usleep(timeoutuSec);
		sendmeta->rmRetxMetadata(prodindex);
	}
	else
		;// throw an error here
}
