dnl Process this file with automake to produce a configure script
dnl Requires ./aclocal.m4
dnl
AC_PREREQ(2.59)
m4_include([version.m4])
AC_INIT([VCMTP], VERSION_ID, [support-vcmtp@unidata.ucar.edu],,
    [http://www.unidata.ucar.edu/software/vcmtp])

AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])

LT_INIT

AC_PROG_LIBTOOL

AC_PROG_CXX
#if test "$GXX" = "yes"; then
#    AC_SUBST([AM_CXXFLAGS], [-std=c++11])
#fi

# Check for the CUNIT unit-testing package
AC_MSG_NOTICE([checking for the CUnit unit-testing package.])
AC_CHECK_FILE([/opt/include/CUnit/CUnit.h],
    [CPPFLAGS_CUNIT=-I/opt/include
    AC_SUBST(CPPFLAGS_CUNIT)
    AC_CHECK_FILE([/opt/lib/libcunit.so],
        [LIBS_CUNIT='-L/opt/lib -lcunit'
        AC_SUBST(LIBS_CUNIT)],
        [AC_CHECK_FILE([/opt/lib/libcunit.a],
            [LIBS_CUNIT='-L/opt/lib -lcunit'
            AC_SUBST(LIBS_CUNIT)])])],
    [AC_CHECK_FILE([/usr/local/include/CUnit/CUnit.h],
        [CPPFLAGS_CUNIT=-I/usr/local/include
        AC_SUBST(CPPFLAGS_CUNIT)
        AC_CHECK_FILE([/usr/local/lib/libcunit.so],
            [LIBS_CUNIT='-L/usr/local/lib -lcunit'
            AC_SUBST(LIBS_CUNIT)],
            [AC_CHECK_FILE([/usr/local/lib/libcunit.a],
                [LIBS_CUNIT='-L/usr/local/lib -lcunit'
                AC_SUBST(LIBS_CUNIT)])])],
        [AC_CHECK_FILE([/usr/include/CUnit/CUnit.h],
            [CPPFLAGS_CUNIT=-I/usr/include
            AC_SUBST(CPPFLAGS_CUNIT)
            AC_CHECK_FILE([/usr/lib/libcunit.so],
                [LIBS_CUNIT='-L/usr/lib -lcunit'
                AC_SUBST(LIBS_CUNIT)],
                [AC_CHECK_FILE([/usr/lib/libcunit.a],
                    [LIBS_CUNIT='-L/usr/lib -lcunit'
                    AC_SUBST(LIBS_CUNIT)])])],
            [AC_CHECK_FILE([/usr/lib64/libcunit.so],
                [LIBS_CUNIT='-L/usr/lib64 -lcunit'
                AC_SUBST(LIBS_CUNIT)],
                [AC_CHECK_FILE([/usr/lib64/libcunit.a],
                    [LIBS_CUNIT='-L/usr/lib64 -lcunit'
                    AC_SUBST(LIBS_CUNIT)])])])])])
AM_CONDITIONAL([HAVE_CUNIT], [test "$CPPFLAGS_CUNIT" && test "$LIBS_CUNIT"])
AM_COND_IF([HAVE_CUNIT], 
    [AC_MSG_NOTICE([CUnit found.  Enabling unit-tests.])],
    [AC_MSG_NOTICE([CUnit not found.  Disabling unit-tests.])])

# Check for the Opmock unit-testing and mocking package
AC_MSG_NOTICE([checking for the Opmock mocking package.])
AC_CHECK_FILE(
    [/usr/local/include/opmock.h],
    [   OPMOCK_CPPFLAGS=-I/usr/local/include
        AC_SUBST([OPMOCK_CPPFLAGS])
        libs_prev=$LIBS
        LIBS="-L/usr/local/lib64 -lopmock${LIBS:+ $LIBS}"
        AC_LINK_IFELSE(
            [   AC_LANG_PROGRAM(
                    [void opmock_test_suite_reset();],
                    [opmock_test_suite_reset();]) ],
            [   OPMOCK_LDFLAGS='-rpath /usr/local/lib64'
                OPMOCK_LDADD='-L/usr/local/lib64 -lopmock'
                AC_SUBST([OPMOCK_LDFLAGS])
                AC_SUBST([OPMOCK_LDADD]) ])
        LIBS=$libs_prev])
AM_CONDITIONAL([HAVE_OPMOCK], [test "$OPMOCK_LDADD"])
AM_COND_IF([HAVE_OPMOCK], 
    [AC_MSG_NOTICE([Opmock found. Enabling mocking-tests.])],
    [AC_MSG_NOTICE([Opmock not found. Disabling mocking-tests.])])

AC_CONFIG_FILES([
    Makefile
    CommUtil/Makefile
    protocol/Makefile
    fatma/Makefile
    shawn/recv/Makefile
    test/protocol/Executor/Makefile
])

AC_OUTPUT
