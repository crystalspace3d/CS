/*
    Copyright (C) 2002 by Norman Kr�mer
  
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

#include "cssysdef.h"
#include "winthread.h"
#include <process.h>

#ifndef _UINTPTR_T_DEFINED
typedef ptr_uint uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

#include "cssys/win32/wintools.h"

#ifdef CS_DEBUG
#define CS_SHOW_ERROR if (lasterr) printf ("%s\n",lasterr)
#else
#define CS_SHOW_ERROR
#endif

#define CS_GET_SYSERROR() \
if (lasterr){delete[] lasterr; lasterr = 0;}\
  lasterr = cswinGetErrorMessage (::GetLastError ())

#define CS_TEST(x) if(!(x)) {CS_GET_SYSERROR(); CS_SHOW_ERROR;}

// ignore recursive switch... windows mutexes are always recursive.
csRef<csMutex> csMutex::Create (bool )
{
  return csPtr<csMutex>(new csWinMutex ());
}

csWinMutex::csWinMutex ()
{
  lasterr = 0;
  mutex = CreateMutex (0, false, 0);
  CS_TEST (mutex != 0);
}

csWinMutex::~csWinMutex ()
{
#ifdef CS_DEBUG
  //  CS_ASSERT (Destroy ());
  Destroy ();
#else
  Destroy ();
#endif
  if (lasterr) {LocalFree (lasterr); lasterr = 0;}
}

bool csWinMutex::Destroy ()
{
  bool rc = CloseHandle (mutex);
  CS_TEST (rc);
  return rc;
}

bool csWinMutex::LockWait()
{
  bool rc = (WaitForSingleObject (mutex, INFINITE) != WAIT_FAILED);
  CS_TEST (rc);
  return rc;
}

bool csWinMutex::LockTry ()
{
  bool rc = (WaitForSingleObject (mutex, 0) != WAIT_FAILED);
  CS_TEST (rc);
  return rc;
}

bool csWinMutex::Release ()
{
  bool rc = ReleaseMutex (mutex);
  CS_TEST (rc);
  return rc;
}

char const* csWinMutex::GetLastError ()
{
  return (char const*)lasterr;
}


csRef<csSemaphore> csSemaphore::Create (uint32 value)
{
  return csPtr<csSemaphore>(new csWinSemaphore (value));
}

csWinSemaphore::csWinSemaphore (uint32 v)
{
  lasterr = 0;
  value = v;
  sem = CreateSemaphore (0, (LONG)value, (LONG)value, 0);
  if (sem == 0)
    value = 0;
  CS_TEST (sem != 0);
}

csWinSemaphore::~csWinSemaphore ()
{
  Destroy ();
}

bool csWinSemaphore::LockWait ()
{
  bool rc = (WaitForSingleObject (sem, INFINITE) != WAIT_FAILED);
  if (rc)
    value--;
  CS_TEST (rc);
  return rc;
}

bool csWinSemaphore::LockTry ()
{
  bool rc = (WaitForSingleObject (sem, 0) != WAIT_FAILED);
  if (rc)
    value--;
  CS_TEST (rc);
  return rc;
}

bool csWinSemaphore::Release ()
{
  bool rc = ReleaseSemaphore (sem, 1, &value);
  if (rc)
    value++;
  CS_TEST (rc);
  return rc;
}

uint32 csWinSemaphore::Value ()
{
  return (uint32)value;
}

bool csWinSemaphore::Destroy ()
{
  bool rc = CloseHandle (sem);
  CS_TEST (rc);
  return rc;
}

char const* csWinSemaphore::GetLastError ()
{
  return (char const*)lasterr;
}


csRef<csCondition> csCondition::Create (uint32 conditionAttributes)
{
  return csPtr<csCondition>(new csWinCondition (conditionAttributes));
}

csWinCondition::csWinCondition (uint32 /*conditionAttributes*/)
{
  lasterr = 0;
  cond = CreateEvent (0, false, false, 0); // auto-reset
  CS_TEST (cond != 0);
}

csWinCondition::~csWinCondition ()
{
  Destroy ();
}

void csWinCondition::Signal (bool /*WakeAll*/)
{
  // only releases one waiting thread, coz its auto-reset
  bool rc = PulseEvent (cond);
  CS_TEST (rc);
}

bool csWinCondition::Wait (csMutex* mutex, csTicks timeout)
{
  // SignalObjectAndWait() is only available in WinNT 4.0 and above
  // so we use the potentially dangerous version below
  if (mutex->Release () && LockWait ((DWORD)timeout))
    return mutex->LockWait ();
  return false;
}

bool csWinCondition::LockWait (DWORD nMilliSec)
{
  bool rc = (WaitForSingleObject (cond, nMilliSec) != WAIT_FAILED);
  CS_TEST (rc);
  return rc;
}

bool csWinCondition::Destroy ()
{
  bool rc = CloseHandle (cond);
  CS_TEST (rc);
  return rc;
}

char const* csWinCondition::GetLastError ()
{
  return (char const*)lasterr;
}


csRef<csThread> csThread::Create (csRunnable* r, uint32 options)
{
  return csPtr<csThread>(new csWinThread (r, options));
}

csWinThread::csWinThread (csRunnable* r, uint32 /*options*/)
{
  runnable = r;
  running = false;
  lasterr = 0;
}

csWinThread::~csWinThread ()
{
  if (running)
    Stop ();
}

bool csWinThread::Start ()
{
  uintptr_t thdl = _beginthread (ThreadRun, 0, (void*)this);
  running = (thdl != (uintptr_t)-1);
  CS_TEST (running);
  thread = (HANDLE)thdl;
  return running;
}

bool csWinThread::Wait ()
{
  if (running)
  {
    bool rc = (WaitForSingleObject (thread, INFINITE) != WAIT_FAILED);
    CS_TEST (rc);
    return rc;
  }
  return true;
}

bool csWinThread::Stop ()
{
  if (running)
  {
    running = !TerminateThread (thread, ~0);
    CS_TEST (!running);
  }
  return !running;
}

char const* csWinThread::GetLastError ()
{
  return (char const*)lasterr;
}

void csWinThread::ThreadRun (void* param)
{
  csWinThread *thread = (csWinThread*)param;
  thread->runnable->Run ();
  thread->running = false;
  _endthread();
}

#undef CS_TEST
#undef CS_SHOW_ERROR
