#!/bin/awk -f
#
# This script is intended to extract all useful information from output of
#
# nm -f sysv --numeric-sort --debug-syms
#
# command. This is known to work with a.out format.

# Function that converts from hexadecimal to decimal
function hexval (str)
{
  gsub (/^[ 	]*/, "", str)
  str = tolower (str)
  len = length (str);
  val = 0
  for (idx = 1; idx <= len; idx++)
  {
    dig = index ("0123456789abcdef", substr (str, idx, 1)) - 1
    if (dig < 0)
      break
    val = (val * 16) + dig
  }
  return val
}

# Function that removes all spaces at the end of string
function striplastspc (str)
{
  gsub (/[ 	]*$/, "", str)
  return str
}

# Function that removes all spaces at the start and end of string
function stripspc (str)
{
  gsub (/^[ 	]*/, "", str)
  gsub (/[ 	]*$/, "", str)
  return str
}

# The startup function that performs initialization
BEGIN {
  FS = "|";
  modname = ""; modaddr = -1;
  funname = ""; funaddr = -1;
  labname = ""; labaddr = -1;
}

# Lines with Type == "SO" contain information about source files
/\| *SO *\|/{
  ma = hexval($2)
  mn = striplastspc($1)
  if (ma == modaddr)
    modname = modname mn
  else
    modname = mn
  modaddr = ma
}

# The lines with Type == "FUN" contain information about functions
/\| *FUN *\|/{
  if (modaddr != -1)
  {
    printf "S %x %s\n", modaddr, modname
    modaddr = -1
  }

  funaddr = hexval($2)
  if (labaddr == funaddr)
    funname = labname
  else
  {
    funname = striplastspc($1)
    if (colidx = index (funname, ":"))
      funname = substr (funname, 1, colidx - 1)
  }
}

# The lines with Type == "SLINE" contain information
# about source code line addresses
/\| *SLINE *\|/{
  if (funaddr != -1)
  {
    printf "F %x %s\n", funaddr, funname
    funaddr = -1
  }

  addr = hexval($2)
  line = hexval($5)
  printf "L %x %d\n", addr, line
}
