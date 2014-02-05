# Copyright 2014 University Corporation for Atmospheric Research. All rights
# reserved. See the file COPYRIGHT in the top-level source-directory of this
# package for copying and redistribution conditions.
#
# Process this file with automake(1) to produce file Makefile.in

SUBDIRS 		= CommUtil protocol
ACLOCAL_AMFLAGS         = -I m4
lib_LTLIBRARIES		= libvcmtp.la
libvcmtp_la_SOURCES	= dummy.c
libvcmtp_la_LIBADD	= CommUtil/lib.la protocol/lib.la
dist_doc_data		= CHANGE_LOG COPYRIGHT README

version.m4:             CHANGE_LOG
	versionId=`awk '{print $1;exit}' $?`; \
	test "$versionId"; \
	echo "m4_define([VERSION_ID], [$$versionId])" >$@
