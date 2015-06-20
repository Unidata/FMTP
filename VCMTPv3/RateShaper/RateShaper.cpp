/**
 * Copyright (C) 2015 University of Virginia. All rights reserved.
 *
 * @file      RateShaper.cpp
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
 * @brief     Rate shaper to limit rate in application layer.
 */


#include "RateShaper.h"

#include <thread>


/**
 * Constructor of RateShaper.
 */
RateShaper::RateShaper()
{
    period    = 0;
    sleeptime = 0;
    rate      = 0;
    txsize    = 0;
}


/**
 * Destructor of RateShaper.
 */
RateShaper::~RateShaper()
{
    // TODO Auto-generated destructor stub
}


/**
 * Sets the sending rate to a given value.
 *
 * @param[in] rate_bps Sending rate in bits per second.
 * @return    None
 */
void RateShaper::SetRate(double rate_bps)
{
    rate = rate_bps;
}


/**
 * Calculates the time period based the pre-set rate, and starts to time.
 *
 * @param[in] size     Size of the packet to be sent.
 * @return    None
 */
void RateShaper::CalPeriod(unsigned int size)
{
    txsize = size;
    /* compute time period in seconds */
    period = (size * 8) / rate;
    start_time  = HRC::now();
}


/**
 * Stops the clock, calculates the transmission time, and sleeps for the
 * rest of the time period.
 *
 * @param[in] None
 * @return    None
 */
void RateShaper::Sleep()
{
    end_time = HRC::now();
    std::chrono::duration<double> txtime = end_time - start_time;
    std::chrono::duration<double> p(period);
    /* sleep for the computed time */
    std::this_thread::sleep_for(p - txtime);
}
