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

#ifndef __CS_POSIX_THREAD_H__
#define __CS_POSIX_THREAD_H__

#include <pthread.h>
#include <semaphore.h>
#include "cssys/thread.h"

// Avoid PTHREAD_MUTEX_HAS_RECURSIVE_NP because the existing check/code breaks
// compilation on some platforms (such as Debian).  This experimental addition
// should not have been added to the stable R0_96 branch in the first place.
#undef PTHREAD_MUTEX_HAS_RECURSIVE_NP

class csPosixThread : public csThread
{
 public:
  /**
   * Construct a new thread.
   * The thread does not run yet, you have to call Start () upon it
   */
  csPosixThread (csRunnable*, uint32 options);
  virtual ~csPosixThread ();

  /**
   * This actually starts the thread.
   * If something gone wrong false is returned.
   */
  virtual bool Start ();

  /**
   * Unmercifully stop the thread as soon as possible.
   * This method performs a dirty shutdown of the thread.  The thread is not
   * given a chance to exit normally.  Do not invoke this method unless you
   * have a very good reason for doing so.  In general, it is best to implement
   * some sort of communication with threads so that you can ask them to
   * terminate in an orderly fashion.  Returns true if the thread was killed.
   */
  virtual bool Stop ();

  /**
   * Wait for the thread to die.  Only returns once the thread has terminated.
   */
  virtual bool Wait ();

  /**
   * Return the last eror description and NULL if there was none.
   */
  virtual char const* GetLastError ();

 protected:
  static void* ThreadRun (void* param);

 protected:
  pthread_t thread;
  csRef<csRunnable> runnable;
  const char *lasterr;
  bool running;
  bool created;
};

class csPosixMutex : public csMutex
{
 public:
  csPosixMutex (bool needrecursive);
  virtual ~csPosixMutex ();

  virtual bool LockWait ();
  virtual bool LockTry  ();
  virtual bool Release  ();
  virtual char const* GetLastError ();

 protected:
  static void Cleanup (void* arg);
 private:
  bool Destroy ();
 protected:
  pthread_mutex_t mutex;
#ifndef PTHREAD_HAS_RECURSIVE_NP
  int count;
  pthread_t owner;
#endif
  
  char const* lasterr;
  friend class csPosixCondition;
};

class csPosixSemaphore : public csSemaphore
{
 public:
  csPosixSemaphore (uint32 value);
  virtual ~csPosixSemaphore ();

  virtual bool LockWait ();
  virtual bool LockTry ();
  virtual bool Release ();
  virtual uint32 Value ();
  virtual char const* GetLastError ();

 protected:
  char const *lasterr;
  sem_t sem;
 private:
  bool Destroy ();
};

class csPosixCondition : public csCondition
{
 public:
  csPosixCondition (uint32 conditionAttributes);
  virtual ~csPosixCondition ();

  virtual void Signal (bool WakeAll = false);
  virtual bool Wait (csMutex*, csTicks timeout = 0);
  virtual char const* GetLastError ();

 private:
  bool Destroy ();
 private:
  pthread_cond_t cond;
  char const* lasterr;
};

#endif // __CS_POSIX_THREAD_H__
