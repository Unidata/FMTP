# Copyright 2014 University Corporation for Atmospheric Research
#
# This file is part of the Unidata LDM package.  See the file COPYRIGHT in
# the top-level source-directory of the package for copying and redistribution
# conditions.
#
# Process this file with automake(1) to produce file Makefile.in

EXTRA_DIST		= main.cpp Makefile_recv
noinst_LTLIBRARIES	= lib.la
lib_la_SOURCES		= TcpRecv.cpp TcpRecv.h vcmtpRecvv3.cpp vcmtpRecvv3.h \
			  ReceivingApplicationNotifier.h
lib_la_CPPFLAGS		= -I$(srcdir)/..
