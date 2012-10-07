/*
  Copyright (C) 2012 by Frank Richter

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
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
#ifndef __CS_CSUTIL_ATOMICOPS_SIMPLE_H__
#define __CS_CSUTIL_ATOMICOPS_SIMPLE_H__

/**\file
 * Implement some simple atomic operations using other operations
 */

#ifndef DOXYGEN_RUN

namespace CS
{
  namespace Threading
  {
    /**
     * Implement some simple atomic operations using other operations
     * Specifically, implements Write() using Swap() and Read() using CompareAndSet().
     */
    template<typename Actual>
    class AtomicOperationsSimple
    {
    public:
      /**
      * Atomically read value pointed to by target.
      */
      CS_FORCEINLINE_TEMPLATEMETHOD
      static int32 Read (const int32* target)
      {
        return Actual::CompareAndSet (const_cast<int32*> (target), 0, 0);
      }

      /**
      * Atomically read pointer pointed to by target.
      */
      CS_FORCEINLINE_TEMPLATEMETHOD
      static void* Read (void* const* target)
      {
        return Actual::CompareAndSet (
          const_cast<void**> (target), (void*)0, (void*)0);
      }
        
      /**
      * Atomically set value pointed to by target to value
      */
      CS_FORCEINLINE_TEMPLATEMETHOD
      static void Write (int32* target, int32 value)
      {
        Actual::Swap (target, value);
      }

      /**
      * Atomically set value pointed to by target to value
      */
      CS_FORCEINLINE_TEMPLATEMETHOD
      static void Write ( void** target, void* value)
      {
        Actual::Swap (target, value);
      }
    };
  } // namespace Threading
} // namespace CS

#endif // DOXYGEN_RUN

#endif // __CS_CSUTIL_ATOMICOPS_SIMPLE_H__
