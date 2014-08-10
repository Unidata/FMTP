#!/bin/sh

g++ -o UnitTest BofResponseTest.cpp BofResponse.cpp -I../../../protocol/ -lcppunit
