/**
 * Copyright (C) 2015 University of Virginia. All rights reserved.
 *
 * @file      RateShaper.h
 * @author    Jie Li
 *            Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      June 13, 2015
 *
 * @section   LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or（at your option）
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details at http://www.gnu.org/copyleft/gpl.html
 *
 * @brief     Rate shaper header file.
 */

#ifndef VCMTP_VCMTPV3_RATESHAPER_H_
#define VCMTP_VCMTPV3_RATESHAPER_H_


#include "Timer.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


class RateShaper {
public:
    RateShaper();
    virtual ~RateShaper();

    void SetRate(double rate_bps);
    void RetrieveTokens(int num_tokens);

private:
    /* maximum rate in bytes per second */
    double rate;
    int bucket_volume;
    int overflow_tolerance;
    int tokens_in_bucket;
    int token_unit;
    /* the gap between two generated tokens in microseconds */
    int token_time_interval;

    CpuCycleCounter cpu_counter;
    double last_check_time;
    struct timespec time_spec;

    timer_t timer_id;
    struct sigevent signal_event;
    struct sigaction signal_action;
    struct itimerspec timer_specs;

    void StartTimer();
    static void AddTokensHandler(int cause, siginfo_t *si, void *ucontext);
    void AddTokens();
};

#endif /* VCMTP_VCMTPV3_RATESHAPER_H_ */
