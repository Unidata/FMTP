#include "time.h"
#include "timer.h"
#include <math.h>
#include <unistd.h>
#include <stdexcept>


Timer::Timer(uint32_t prodindex, senderMetadata* sendmeta, bool& rmState)
{
	trigger(prodindex, sendmeta, rmState);
}


Timer::~Timer()
{
}


void Timer::trigger(uint32_t prodindex, senderMetadata* sendmeta, bool& rmState)
{
    RetxMetadata* perProdMeta = sendmeta->getMetadata(prodindex);

    if (perProdMeta != NULL)
    {
        float           seconds;
        float           fraction = modff(perProdMeta->retxTimeoutPeriod,
                &seconds);
        struct timespec timespec;

        timespec.tv_sec = seconds;
        timespec.tv_nsec = fraction * 1e9f;
        (void)nanosleep(&timespec, 0);
        rmState = sendmeta->rmRetxMetadata(prodindex);
    }
    else
        throw std::runtime_error("Timer::trigger() get RetxMetadata error");
}
