# Copyright 2014 University Corporation for Atmospheric Research. All rights
# reserved. See the file COPYRIGHT in the top-level source-directory of this
# package for copying and redistribution conditions.
#
# Process this file with automake(1) to produce file Makefile.in

EXTRA_DIST		= vcmtpBase.cpp vcmtpBase.h
SUBDIRS 		= receiver sender SilenceSuppressor
noinst_LTLIBRARIES	= lib.la
lib_la_SOURCES		= vcmtpBase.cpp vcmtpBase.h TcpBase.cpp TcpBase.h
lib_la_LIBADD		= receiver/lib.la sender/lib.la \
			  SilenceSuppressor/lib.la
