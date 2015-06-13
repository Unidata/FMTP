/**
 * Copyright (C) 2015 University of Virginia. All rights reserved.
 *
 * @file      RateShaper.cpp
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
 * @brief     Rate shaper to limit rate in application layer.
 */

#include "RateShaper.h"


RateShaper::RateShaper() {
    rate = 0;
    bucket_volume = 0;
    tokens_in_bucket = 0;
    token_unit = 0;

    token_time_interval = 200;
    overflow_tolerance = 1500;

    time_spec.tv_sec = 0;
    time_spec.tv_nsec = 0;
}


RateShaper::~RateShaper() {
    // TODO Auto-generated destructor stub
}


void RateShaper::SetRate(double rate_bps) {
    rate = rate_bps;
    token_unit = token_time_interval / 1000000.0 * rate_bps / 8;
    tokens_in_bucket = token_unit;
    overflow_tolerance = rate_bps * 0.005;	// allow 5ms burst tolerance
    bucket_volume = overflow_tolerance + token_unit;

    AccessCPUCounter(&cpu_counter.hi, &cpu_counter.lo);
    last_check_time = 0.0;
}


void RateShaper::RetrieveTokens(int num_tokens) {
    if (tokens_in_bucket >= num_tokens) {
        tokens_in_bucket -= num_tokens;
        return;
    }
    else {
        double elapsed_sec = GetElapsedSeconds(cpu_counter);
        double time_interval = elapsed_sec - last_check_time;
        while (time_interval * 1000000 < token_time_interval) {
            time_spec.tv_nsec = (token_time_interval - time_interval * 1000000) * 1000;
            nanosleep(&time_spec, NULL);

            elapsed_sec = GetElapsedSeconds(cpu_counter);
            time_interval = elapsed_sec - last_check_time;
        }

        last_check_time = elapsed_sec;
        int tokens = time_interval * 1000000.0 / token_time_interval * token_unit;
        tokens_in_bucket += tokens - num_tokens;
        if (tokens_in_bucket > bucket_volume) {
            tokens_in_bucket = bucket_volume;
        }
    }
}

void RateShaper::StartTimer() {
    /* Install our SIGPROF signal handler */
    signal_action.sa_sigaction = RateShaper::AddTokensHandler;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = SA_SIGINFO; /* we want a siginfo_t */
    if (sigaction(SIGALRM, &signal_action, 0)) {
        cout << "ReceiveBufferMgr::StartNackRetransTimer()::sigaction()" << endl;
        exit(-1);
    }

    // Define sigEvent
    // This information will be forwarded to the signal-handler function
    memset(&signal_event, 0, sizeof(signal_event));
    // With the SIGEV_SIGNAL flag we say that there is sigev_value
    signal_event.sigev_notify = SIGEV_SIGNAL;
    // Now it's possible to give a pointer to the object
    signal_event.sigev_value.sival_ptr = (void*) this;
    // Declare this signal as Alarm Signal
    signal_event.sigev_signo = SIGALRM;

    // Install the Timer
    if (timer_create(CLOCK_REALTIME, &signal_event, &timer_id) != 0) {
        cout << "ReceiveBufferMgr::StartNackRetransTimer()::timer_create()" << endl;
        exit(-1);
    }

    /* Request SIGPROF */
    timer_specs.it_interval.tv_sec = 0;
    timer_specs.it_interval.tv_nsec = 100 * 1000;
    timer_specs.it_value.tv_sec = 0;
    timer_specs.it_value.tv_nsec = 100 * 1000;
    //setitimer(SIGALRM, &timer_specs, NULL);

    if (timer_settime(timer_id, 0, &timer_specs, NULL) == -1) {
        cout << "Could not start timer" << endl;
        exit(-1);
    }
}



void RateShaper::AddTokensHandler(int cause, siginfo_t *si, void *ucontext) {
    RateShaper* ptr = (RateShaper*)si->si_value.sival_ptr;
    ptr->AddTokens();
}


void RateShaper::AddTokens() {
    tokens_in_bucket += token_unit;
    if (tokens_in_bucket > bucket_volume)
        tokens_in_bucket = bucket_volume;
}
