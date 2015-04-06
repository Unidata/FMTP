/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      randgen.cpp
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Apr. 3, 2015
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
 * @brief     Generates random sized file with random content.
 */


#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <random>


int main(int argc, const char *argv[])
{
    std::random_device rd;
    unsigned int rand = rd() % 100 + 1;
    rand = rand * 1024;

    /* maximum 100KB */
    char* data = new char[rand];
    std::ifstream fp("/dev/urandom", std::ios::binary);
    if (fp.is_open()) {
        fp.read(data, rand);
    }

    std::ofstream rdfile("test.dat");
    if (rdfile.is_open()) {
        rdfile.write(data, rand);
    }
    fp.close();
    rdfile.close();
    delete[] data;

    return 0;
}
