#!/bin/sh
#------------------------------------------------------------------------------
# mkverres.sh
# make a windows version resource.
#------------------------------------------------------------------------------

outfile=$1
description=$2

verfile=include/csver.h
vmajor=`sed -e '/#define[ 	][ 	]*CS_VERSION_MAJOR/!d' -e '/#define[ 	][ 	]*CS_VERSION_MAJOR/s/\(#define[ 	][ 	]*CS_VERSION_MAJOR[ 	][ 	]*"\)\([^\"]*\)"\(.*\)/\2/' < ${verfile}`
vminor=`sed -e '/#define[ 	][ 	]*CS_VERSION_MINOR/!d' -e '/#define[ 	][ 	]*CS_VERSION_MINOR/s/\(#define[ 	][ 	]*CS_VERSION_MINOR[ 	][ 	]*"\)\([^\"]*\)"\(.*\)/\2/' < ${verfile}`
vminor=`echo ${vminor} | sed -e 's/[^0-9]//g'`
if [ -z "$vminor" ]; then
    vminor=0
fi
fileversion=`echo "${vmajor},${vminor},0" | sed -e 's/\./,/'`

cat > ${outfile} << EOF
// This file is automatically generated.

#include "volatile.h"
#include "csver.h"

#define CS_VERSION_W_DATE CS_VERSION " (" CS_RELEASE_DATE ")"

1 VERSIONINFO
FILEVERSION ${fileversion}
{
  BLOCK "StringFileInfo"
  {
    BLOCK "040904E4"
    {
      VALUE "ProductName", "CrystalSpace"
      VALUE "ProductVersion", CS_VERSION_W_DATE
      VALUE "FileVersion", CS_VERSION_NUMBER
      VALUE "LegalCopyright", "(C)1998-2002 Jorrit Tyberghein and others."
      VALUE "FileDescription", "${description}"
#ifdef CS_DEBUG
      VALUE "Comments", "Debug build"
#else
      VALUE "Comments", "Release build"
#endif
      VALUE "WWW", "http://crystal.sourceforge.net/"
    }
  }
}
EOF
