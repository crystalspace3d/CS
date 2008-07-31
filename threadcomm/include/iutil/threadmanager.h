/*
  Copyright (C) 2008 by Michael Gist

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

#ifndef __CS_IUTIL_THREADMANAGER_H__
#define __CS_IUTIL_THREADMANAGER_H__

struct iJob;

struct iThreadReturn : public csRefCount
{
  virtual bool IsFinished() = 0;
  virtual bool WasSuccessful() = 0;
  virtual void* GetResultPtr() = 0;
  virtual csRef<iBase> GetResultRefPtr() = 0;

  virtual void MarkFinished() = 0;
  virtual void MarkSuccessful() = 0;
  virtual void SetResult(void* result) = 0;
  virtual void SetResult(csRef<iBase> result) = 0;

  virtual void Copy(iThreadReturn* other) = 0;

  virtual void Wait() = 0;
};

/**
 * This is the thread manager.
 *
 * Main creators of instances implementing this interface:
 * - csInitializer::CreateEnvironment()
 * - csInitializer::CreateThreadManager()
 * 
 * Main ways to get pointers to this interface:
 * - csQueryRegistry()
 */

struct iThreadManager : public virtual iBase
{
  SCF_INTERFACE(iThreadManager, 1, 0, 0);

  virtual void Process(uint num = 1) = 0;
  virtual void PushToQueue(bool useThreadQueue, iJob* job) = 0;
  virtual void Wait(csRef<iThreadReturn> result) = 0;
};

// Interface macros
#define THREADED_INTERFACE(function) \
  virtual iThreadReturn* function() = 0;

#define THREADED_INTERFACE1(function, arg1) \
  virtual iThreadReturn* function(arg1) = 0;

#define THREADED_INTERFACE2(function, arg1, arg2) \
  virtual iThreadReturn* function(arg1, arg2) = 0;

#define THREADED_INTERFACE3(function, arg1, arg2, arg3) \
  virtual iThreadReturn* function(arg1, arg2, arg3) = 0;

#define THREADED_INTERFACE4(function, arg1, arg2, arg3, arg4) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4) = 0;

#define THREADED_INTERFACE5(function, arg1, arg2, arg3, arg4, arg5) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5) = 0;

#define THREADED_INTERFACE6(function, arg1, arg2, arg3, arg4, arg5, arg6) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6) = 0;

#define THREADED_INTERFACE7(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7) = 0;

#define THREADED_INTERFACE8(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) = 0;

#define THREADED_INTERFACE9(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) = 0;

#define THREADED_INTERFACE10(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) = 0;

#define THREADED_INTERFACE11(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) = 0;

#define THREADED_INTERFACE12(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) = 0;

#define THREADED_INTERFACE13(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) = 0;

#define THREADED_INTERFACE14(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) = 0;

#define THREADED_INTERFACE15(function, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) \
  virtual iThreadReturn* function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) = 0;

#endif // __CS_IUTIL_THREADMANAGER_H__
