/*
Copyright (C) 2007 by Michael Gist

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

#ifndef __SIMD_TYPES_H__
#define __SIMD_TYPES_H__

// TODO: Make compatible with non-supporting compiler (i.e. checks for xmmintrin.h)

#include <xmmintrin.h>

namespace CS
{
    namespace SIMD
    {
        // TODO: Make PPC versions and check compat with gcc/*nix.
        typedef __m128 SIMDVector4;

        static inline __m128 csMulSIMD(__m128 a, __m128 b)
        {
            return _mm_mul_ps(a, b);
        }

        static inline __m128 csAddSIMD(__m128 a, __m128 b)
        {
            return _mm_add_ps(a, b);
        }

        static inline __m128 csSqrtSIMD(__m128 a)
        {
            return _mm_sqrt_ps(a);
        }
    }
}

#endif // __SIMD_TYPES_H__
