dnl Example for use of GNU gettext.
dnl Copyright (C) 2003 Free Software Foundation, Inc.
dnl This file is in the public domain.
dnl
dnl Configuration file - processed by autoconf.

AC_INIT
AC_CONFIG_SRCDIR(hello.cc)
AM_INIT_AUTOMAKE(hello-c++-qt, 0)

AC_PROG_CXX
FUN_TYPE_BOOL
AC_CHECK_HEADERS([unistd.h])
FUN_HEADER_STDCXX
FUN_CHECK_PTHREAD
FUN_CHECK_QT(220)

dnl Support for the po directory.
AM_PO_SUBDIRS

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])
AC_OUTPUT
