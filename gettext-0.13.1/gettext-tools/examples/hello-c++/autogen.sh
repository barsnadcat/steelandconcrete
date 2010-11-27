#!/bin/sh
# Example for use of GNU gettext.
# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is in the public domain.
#
# Script for regenerating all autogenerated files.

test -r ../Makefile.am || . ../installpaths

cp -p ${GETTEXTSRCDIR-../../../gettext-tools/lib}/gettext.h gettext.h

autopoint -f # was: gettextize -f -c
rm po/Makevars.template
rm po/Rules-quot
rm po/boldquot.sed
rm po/en@boldquot.header
rm po/en@quot.header
rm po/insert-header.sin
rm po/quot.sed

aclocal -I m4

autoconf

automake -a -c

cd po
for f in *.po; do
  lang=`echo $f | sed -e 's,\.po$,,'`
  msgfmt -c -o $lang.gmo $lang.po
done
cd ..
