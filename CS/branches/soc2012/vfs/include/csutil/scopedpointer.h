/*
  Copyright (C) 2007-2011 by Frank Richter

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
#ifndef __CSUTIL_SCOPEDPOINTER_H__
#define __CSUTIL_SCOPEDPOINTER_H__

/**\file
 * Pointer valid in a scope
 */

#include "csutil/noncopyable.h"

namespace CS
{
namespace Utility
{

  /**
   * Smart pointer that deletes the contained pointer when the scope is exited.
   * \tparam T is the type pointed to.
   * \tparam IsArray true if pointer was allocated with new [] operator;
   *   false if new was used.
   */
  template<class T, bool IsArray = false>
  class ScopedPointer : private NonCopyable
  {
    T* ptr;
  public:
    /**
     * Construct from given pointer.
     * \remarks Takes ownership of \a ptr!
     */
    ScopedPointer (T* ptr = nullptr) : ptr (ptr) {}
    /// Destruct. Deletes the given pointer!
    ~ScopedPointer() { if (IsArray) delete [] ptr; else delete ptr; }

    /**
     * Replace the contained pointer with another.
     * \remarks Takes ownership of \a ptr!
     */
    void Reset (T* ptr = nullptr)
    {
      if (IsArray)
        delete [] this->ptr;
      else
        delete this->ptr;
      this->ptr = ptr;
    }
    /// Invalidate (delete) the contained pointer
    void Invalidate() { Reset(); }
    /// Check if the contained pointer is valid.
    bool IsValid() const { return ptr != (T*)nullptr; }
  
    /// Dereference underlying pointer.
    T* operator -> () const
    { return ptr; }
    
    /// Cast to a pointer.
    operator T* () const
    { return ptr; }
    
    /// Dereference underlying pointer.
    T& operator* () const
    { return *ptr; }
  
  };
} // namespace Utility
} // namespace CS

#endif // __CSUTIL_SCOPEDPOINTER_H__
