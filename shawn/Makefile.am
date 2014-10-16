# Copyright 2014 University Corporation for Atmospheric Research. All rights
# reserved. See the file COPYRIGHT in the top-level source-directory of this
# package for copying and redistribution conditions.
#
# Process this file with automake(1) to produce file Makefile.in

SUBDIRS 		= recv send
noinst_LTLIBRARIES	= lib.la
lib_la_SOURCES		= vcmtpBase.cpp vcmtpBase.h \
			  ReceivingApplicationNotifier.h
lib_la_LIBADD		= recv/lib.la send/lib.la

