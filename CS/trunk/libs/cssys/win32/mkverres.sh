#!/bin/sh
#------------------------------------------------------------------------------
# mkverres.sh
# make a windows version resource.
#------------------------------------------------------------------------------

outfile=$1
description=$2

verfile=include/csver.h
vmajor=`sed -e '/#define[ 	][ 	]*CS_VERSION_MAJOR/!d' -e 's/#define[ 	][ 	]*CS_VERSION_MAJOR[ 	][ 	]*CS_VER_QUOTE(\(..*\)).*/\1/' < ${verfile}`
vminor=`sed -e '/#define[ 	][ 	]*CS_VERSION_MINOR/!d' -e 's/#define[ 	][ 	]*CS_VERSION_MINOR[ 	][ 	]*CS_VER_QUOTE(\(..*\)).*/\1/' < ${verfile}`
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
PRODUCTVERSION ${fileversion}
#ifdef CS_DEBUG
FILEFLAGS 0x1
#else
FILEFLAGS 0x0
#endif
{
  BLOCK "StringFileInfo"
  {
    BLOCK "040904E4"
    {
      VALUE "ProductName", "CrystalSpace"
      VALUE "ProductVersion", CS_VERSION_W_DATE
      VALUE "FileVersion", CS_VERSION_NUMBER
      VALUE "LegalCopyright", "(C)1998-2003 Jorrit Tyberghein and others."
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
