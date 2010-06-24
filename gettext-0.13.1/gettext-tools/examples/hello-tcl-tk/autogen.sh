#!/bin/sh
# Example for use of GNU gettext.
# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is in the public domain.
#
# Script for regenerating all autogenerated files.

test -r ../Makefile.am || . ../installpaths

cp -p ${ACLOCALDIR-../../../gettext-runtime/m4}/nls.m4 m4/nls.m4
cp -p ${ACLOCALDIR-../../../gettext-runtime/m4}/po.m4 m4/po.m4
cp -p ${GETTEXTSRCPODIR-../../../gettext-runtime/po}/remove-potcdate.sin po/remove-potcdate.sin

aclocal -I m4

autoconf

automake -a -c

cd po
for f in *.po; do
  if test -r "$f"; then
    lang=`echo $f | sed -e 's,\.po$,,'`
    msgfmt -c --tcl -d . -l $lang $lang.po
  fi
done
cd ..
