/*
    Copyright (C) 1998 by Jorrit Tyberghein
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __FPU80X86_H__
#define __FPU80X86_H__

#if defined(PROC_INTEL) && defined(COMP_GCC) && !defined(OS_NEXT) && !defined(OS_BE)

/*
     ---- ---- --XX XXXX = MCW_EM - exception masks (1=handle exception internally, 0=fault)
     ---- ---- ---- ---X = EM_INVALID - invalid operation
     ---- ---- ---- --X- = EM_DENORMAL - denormal operand
     ---- ---- ---- -X-- = EM_ZERODIVIDE - divide by zero
     ---- ---- ---- X--- = EM_OVERFLOW - overflow
     ---- ---- ---X ---- = EM_UNDERFLOW - underflow
     ---- ---- --X- ---- = EM_INEXACT - rounding was required
     ---- --XX ---- ---- = MCW_PC - precision control
     ---- --00 ---- ---- = PC_24 - single precision
     ---- --10 ---- ---- = PC_53 - double precision
     ---- --11 ---- ---- = PC_64 - extended precision
     ---- XX-- ---- ---- = MCW_RC - rounding control
     ---- 00-- ---- ---- = RC_NEAR - round to nearest
     ---- 01-- ---- ---- = RC_DOWN - round towards -Inf
     ---- 10-- ---- ---- = RC_UP - round towards +Inf
     ---- 11-- ---- ---- = RC_CHOP - round towards zero
     ---X ---- ---- ---- = MCW_IC - infinity control (obsolete, always affine)
     ---0 ---- ---- ---- = IC_AFFINE - -Inf < +Inf
     ---1 ---- ---- ---- = IC_PROJECTIVE - -Inf == +Inf
*/

static inline unsigned int _control87(unsigned int newcw, unsigned int mask)
{
  int oldcw;
  int tmpcw;
  asm __volatile__
  (
        "       fclex                   \n"     // clear exceptions
        "       fstcw   %0              \n"     // oldcw = FPU control word
	"       movl    %%ebx, %%eax    \n"
        "       movl    %%ecx, %%edx    \n"
        "       andl    %0,%%ebx        \n"     // eax &= oldcw;
        "       andl    %1,%%edx        \n"     // ecx &= newcw
        "       orl     %%edx,%%ebx     \n"     // eax |= ecx
        "       movl    %%ebx,%4        \n"     // tmpcw = eax
        "       fldcw   %4              \n"     // load FPU control word
        : "=m" (oldcw)
	: "g" (newcw), "a" (~mask), "c" (mask), "m" (tmpcw)
	: "ebx", "edx", "memory"
  );
  return oldcw & 0xffff;
}

#else

static inline unsigned int _control87(unsigned int, unsigned int) { return 0; }

#endif

#endif // __FPU80X86_H__
