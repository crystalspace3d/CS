/*
    Copyright (C) 1999 by Andrew Zabolotny <bit@eltech.ru>

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

#ifndef __ILIGHT_H__
#define __ILIGHT_H__

#include "csutil/scf.h"

SCF_VERSION (iLight, 0, 0, 1);

/**
 * The iLight interface is the SCF interface for the csLight class. 
 */
struct iLight : public iBase
{
};

#endif // __ILIGHT_H__
