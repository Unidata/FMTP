/**
 * Copyright (C) 2015 University of Virginia. All rights reserved.
 *
 * @file      RateShaper.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      June 19, 2015
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


#include <time.h>
#include <chrono>

typedef std::chrono::high_resolution_clock HRC;


class RateShaper {
public:
    RateShaper();
    ~RateShaper();
    /* sets the expected rate in bits/sec */
    void SetRate(double rate_bps);
    /* calculate the time period based on the rate */
    void CalPeriod(unsigned int size);
    /* sleep for an amount of time based the calculated value */
    void Sleep();

private:
    double period;
    double sleeptime;
    double rate;
    /* transmission packet size */
    unsigned int txsize;
    /* transmission start time */
    HRC::time_point start_time;
    /* transmission end time */
    HRC::time_point end_time;
};


#endif /* VCMTP_VCMTPV3_RATESHAPER_H_ */
