# Copyright 2015 University Corporation for Atmospheric Research
#
# This file is part of the Unidata LDM package.  See the file COPYRIGHT in
# the top-level source-directory of the package for copying and redistribution
# conditions.
#
# Process this file with automake(1) to produce file Makefile.in

SENDER_SRCDIR	= $(top_srcdir)/VCMTPv3/sender
CPPFLAGS	= -I$(SENDER_SRCDIR)
ProdIndexDeleyQueueTest_SOURCES 	= \
        ProdIndexDelayQueueTest.cpp \
        $(SENDER_SRCDIR)/ProdIndexDelayQueue.cpp
LDFLAGS		= -lgtest

check_PROGRAMS	= ProdIndexDeleyQueueTest
TESTS		= $(check_PROGRAMS)
