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


RateShaper::RateShaper()
{
    bucket_volume       = 0;
    tokens_in_bucket    = 0;
    token_unit          = 0;
    token_time_interval = 200;
    overflow_tolerance  = 1500;

    time_spec.tv_sec    = 0;
    time_spec.tv_nsec   = 0;
}


RateShaper::~RateShaper()
{
    // TODO Auto-generated destructor stub
}


void RateShaper::SetRate(double rate_bps)
{
    token_unit = token_time_interval / 1000000.0 * rate_bps / 8;
    tokens_in_bucket = token_unit;
    overflow_tolerance = rate_bps * 0.0;  // allow 0ms burst tolerance
    bucket_volume = overflow_tolerance + token_unit;

    AccessCPUCounter(&cpu_counter.hi, &cpu_counter.lo);
    last_check_time = 0.0;
}


void RateShaper::RetrieveTokens(int num_tokens)
{
    if (tokens_in_bucket >= num_tokens) {
        tokens_in_bucket -= num_tokens;
        return;
    }
    else {
        double elapsed_sec = GetElapsedSeconds(cpu_counter);
        double time_interval = elapsed_sec - last_check_time;
        while (time_interval * 1000000 < token_time_interval) {
            time_spec.tv_nsec = (token_time_interval -
                                 time_interval * 1000000) * 1000;
            nanosleep(&time_spec, NULL);

            elapsed_sec = GetElapsedSeconds(cpu_counter);
            time_interval = elapsed_sec - last_check_time;
        }

        last_check_time = elapsed_sec;
        int tokens = time_interval * 1000000.0 / token_time_interval
                     * token_unit;
        tokens_in_bucket += tokens - num_tokens;
        if (tokens_in_bucket > bucket_volume) {
            tokens_in_bucket = bucket_volume;
        }
    }
}
