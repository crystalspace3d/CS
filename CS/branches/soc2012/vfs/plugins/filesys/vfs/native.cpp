/*

    Crystal Space Virtual File System class
    Copyright (C) 1998,1999,2000 by Andrew Zabolotny <bit@eltech.ru>
    Copyright (C) 2012 by Eunsoo Roh <nes1209@hotmail.com>

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
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

// win32 utime() workaround
#ifdef CS_PLATFORM_WIN32
#include <sys/utime.h>
#define utime(a,b) _utime(a,b)
#define utimbuf _utimbuf
#else
#include <utime.h>
#endif

#include "vfs.h"
#include "csutil/scf_implementation.h"
#include "csutil/sysfunc.h"
#include "csutil/syspath.h"
#include "csutil/util.h"
#include "csutil/platformfile.h"
#include "csutil/vfsplat.h"
#include "csutil/databuf.h"
#include "csutil/filepermission.h"

// VC++ defines _S_IFDIR instead of S_IFDIR
// Define _S_IFDIR for other platform
#ifndef _S_IFDIR
#define _S_IFDIR S_IFDIR
#endif

// local helper functions
namespace
{
  // Replace occurences of 'search' characters in String to 'replace'
  char * ReplaceChar (char * string, char search, char replace)
  {
    if (!string)
      return nullptr;

    // replace all occurences; assume String is null-terminated.
    for (char *curr = string; curr != '\0'; ++curr)
      if (*curr == search)
        *curr = replace;

    return string;
  }

  // Convert errno error status to VFS status code
  int ErrnoToVfsStatus (int error)
  {
    int status = VFS_STATUS_OK;
    // the following code is from old vfs.cpp code
    switch (error)
    {
      case 0:
        // no error; commented out because VFS_STATUS_OK is the default
        //status = VFS_STATUS_OK;
        break;
#ifdef ENOSPC
      case ENOSPC:
        status = VFS_STATUS_NOSPACE;
        break;
#endif
#ifdef EMFILE
      case EMFILE:
#endif
#ifdef ENFILE
      case ENFILE:
#endif
#ifdef ENOMEM
      case ENOMEM:
#endif
#if defined( EMFILE ) || defined( ENFILE ) || defined( ENOMEM )
        status = VFS_STATUS_RESOURCES;
        break;
#endif
#ifdef ETXTBSY
    case ETXTBSY:
#endif
#ifdef EROFS
    case EROFS:
#endif
#ifdef EPERM
     case EPERM:
#endif
#ifdef EACCES
     case EACCES:
#endif
#if defined( ETXTBSY ) || defined( EROFS ) || defined( EPERM ) || \
    defined( EACCES )
        status = VFS_STATUS_ACCESSDENIED;
        break;
#endif
#ifdef EIO
      case EIO:
        status = VFS_STATUS_IOERROR;
        break;
#endif
      default:
        // there was an error, but not recognized
        status = VFS_STATUS_OTHER;
        break;
    }
    return status;
  }
}


// this belongs to VFS namespace, for now.
CS_PLUGIN_NAMESPACE_BEGIN(VFS)
{

// OS Native iFileSystem
class NativeFS : public scfImplementation1<NativeFS, iFileSystem>
{
  // Real-world mountpoint of this instance
  char *mountRoot;
  // Last error status
  int lastError;
  // Allocator heap for this filesystem
  //csRef<HeapRefCounted> heap;

  // Construct a NativeFS instance with 'RealPath' as mount root.
  NativeFS (const char *realPath);
  // Convert relative virtual path to real path
  csString ToRealPath (const char *virtualPath);
  // Set last error to specified value
  bool SetLastError (int errorCode);
  // Update last error code from system call
  void UpdateError ();

public:

  // Destructor
  virtual ~NativeFS ();
  // Open a file of given path and return iFile pointer
  virtual csPtr<iFile> Open (const char *path,
                             const char *pathPrefix,
                             int mode,
                             bool useCaching);
  // Move file from OldPath to NewPath
  virtual bool Move (const char *oldPath, const char *newPath);
  // Get permission set of a file
  virtual bool GetPermission (const char *filename, csFilePermission &oPerm);
  // Set permission set of a file
  virtual bool SetPermission (const char *filename,
                              const csFilePermission &iPerm);
  // Set file time
  virtual bool GetTime (const char *filename, csFileTime &oTime);
  // Get file time
  virtual bool SetTime (const char *filename, const csFileTime &iTime);
  // Get file size
  virtual bool GetSize (const char *filename, uint64_t &oSize);
  // Check for file existence
  virtual bool Exists (const char *filename);
  // Delete a given file
  virtual bool Delete (const char *filename);
  // Get mount root
  virtual csString GetRootRealPath ();
  // Query and reset last error status
  virtual int GetStatus ();
};

// OS Native iFile
class NativeFile : public scfImplementation1<NativeFile, iFile>
{
  class View;

  friend class View;
  friend class NativeFS;

  // Fully-qualified VFS path of file
  char *virtualPath;
  // Real path
  char *realPath;
  // File size
  uint64_t size;
  // Last error
  int lastError;
  // Native file handle
  FILE *handle;
  // Read-only?
  bool readOnly;

  // Constructor
  NativeFile (NativeFS *parent,
              const char *virtualPath,
              const char *realPath,
              int mode);
  // Set last error to specified value
  bool SetLastError (int errorCode);
  // Update last error code from system call
  void UpdateError ();
public:
  // Destructor
  virtual ~NativeFile ();
  // Query VFS filename
  virtual const char *GetName () { return virtualPath; }
  // Query file size
  virtual uint64_t GetSize () { return size; }
  // Query and reset last error status
  virtual int GetStatus ();
  // Read Length bytes into the buffer at which Data points.
  virtual size_t Read (char *data, size_t length);
  // Write Length bytes from the buffer at which Data points.
  virtual size_t Write (const char *data, size_t length);
  // Flush strem
  virtual void Flush ();
  // Check whether pointer is at End of File
  virtual bool AtEOF ();
  // Query file pointer (absolute position)
  virtual uint64_t GetPos ();
  // Set file pointer (relative position; absolute by default)
  virtual bool SetPos (off64_t newPos, int relativeTo = VFS_POS_ABSOLUTE);
  // Get all data into a single buffer.
  virtual csPtr<iDataBuffer> GetAllData (bool nullTerminated = false);
  // Get all data into a single buffer with custom allocator.
  virtual csPtr<iDataBuffer> GetAllData (CS::Memory::iAllocator *allocator);
  // Get subset of file as iFile
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0);
};

// View for NativeFile
class NativeFile::View : public scfImplementation1<View, iFile>
{
  friend class NativeFile;

  // parent NativeFile
  csRef<NativeFile> parent;
  // zero-based offset of parent NativeFile
  uint64_t offset;
  // length of this view
  uint64_t size;
  // zero-based position of this view
  uint64_t pos;
  // last error status of this view
  int lastError;

  // Constructor
  View (NativeFile *parent, uint64_t offset, uint64_t size);
public:
  // Destructor
  virtual ~View ();
  // Query filename
  virtual const char *GetName () { return "#NativeFile::View"; }
  // Query file size
  virtual uint64_t GetSize () { return size; }
  // Query and reset last error status
  virtual int GetStatus ();
  // Read Length bytes into the buffer at which Data points.
  virtual size_t Read (char *data, size_t length);
  // Write Length bytes from the buffer at which Data points.
  virtual size_t Write (const char *data, size_t length);
  // Flush strem
  virtual void Flush ();
  // Check whether pointer is at End of File
  virtual bool AtEOF ();
  // Query file pointer (absolute position)
  virtual uint64_t GetPos ();
  // Set file pointer (relative position; absolute by default)
  virtual bool SetPos (off64_t newPos, int relativeTo = VFS_POS_ABSOLUTE);
  // Get all data into a single buffer.
  virtual csPtr<iDataBuffer> GetAllData (bool nullTerminated = false);
  // Get all data into a single buffer with custom allocator.
  virtual csPtr<iDataBuffer> GetAllData (CS::Memory::iAllocator *allocator);
  // Get subset of file as iFile
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0);
};


// NativeFile methods
/* NativeFile Constructor
 - takes parent      (pointer to filesystem which invoked the constructor)
         virtualPath (full VFS path to file)
         realPath    (full real path to file. must be formatted beforehand)
         mode        (bitflag combination of VFS_FILE constants)
 - Remarks: While mode is bitflag parameter, VFS_FILE_READ, VFS_FILE_APPEND
              and VFS_FILE_WRITE are mutually exclusive.
            This filesystem does not support VFS_FILE_UNCOMPRESSED.
 */
NativeFile::NativeFile (NativeFS *parent,
                        const char *virtualPath,
                        const char *realPath, int mode) :
 scfImplementationType (this)
{
  // TODO: Add debug messages

  // Measure required length
  size_t virtualPathLen = strlen (virtualPath) + 1;
  size_t realPathLen    = strlen (realPath) + 1;
  // Allocate buffer
  this->virtualPath = (char *)cs_malloc (virtualPathLen);
  this->realPath    = (char *)cs_malloc (realPathLen);
  // Copy string content
  memcpy (this->virtualPath, virtualPath, virtualPathLen);
  memcpy (this->realPath,    realPath,    realPathLen);

  bool retry = false;
  int fileMode = mode & VFS_FILE_MODE;
  readOnly = (fileMode == VFS_FILE_READ);
  do
  {
    // Open file handle with specified mode
    switch (fileMode)
    {
      case VFS_FILE_WRITE:
        // write only
        handle = CS::Platform::File::Open (realPath, "wb");
        break;
      case VFS_FILE_APPEND:
        // append mode
        handle = CS::Platform::File::Open (realPath, "ab");
        break;
      case VFS_FILE_READ:
      default:
        // read only or unknown mode
        handle = CS::Platform::File::Open (realPath, "rb");
        break;
    }

    // if this is a retry, exit the loop
    if (retry)
      break;

    if (!handle)
    {
      // somehow failed to open; try creating directory and try again
      if (fileMode != VFS_FILE_READ)
      {
        // TODO: create missing directory tree
        retry = true;
      }
    }
  } while (retry);

  // is this a regular file? (i.e. not fifo, etc.)
  if (handle && !CS::Platform::IsRegularFile (realPath))
  {
    // close the handle
    fclose (handle);
    handle = nullptr;
  }

  if (!handle)
  {
    // Somehow failed to open file
    // TODO: error handling code
  }
  else
  {
    // Store the size
    // TODO: implement 64-bit fail-proof handling
    if (fseek (handle, 0, SEEK_END) == 0)
    {
      // fseek succeeded;
      size = ftell (handle);
      if (size == (uint64_t)-1)
      {
        // cannot possibly have this size
        size = 0;
        // update error
        UpdateError ();
      }
    }
    else
      // update error from errno
      UpdateError ();

    // for read or write mode, reposition file pointer
    if (fileMode != VFS_FILE_APPEND)
      rewind (handle);
  }
}

// NativeFile destructor
NativeFile::~NativeFile ()
{
  // Free memory and resources
  cs_free(virtualPath);
  cs_free(realPath);
}

/* Set last error to specified value
 - takes errorCode (any constant in VFS_STATUS_* enumeration)
 - returns true if error code is updated, or stayed VFS_STATUS_OK
           false if status unchanged due to lastError wasn't VFS_STATUS_OK
 */
bool NativeFile::SetLastError (int errorCode)
{
  // If new status is VFS_STATUS_OK, do it anyway.
  // Otherwise, update error code if and only if previous error is cleared.
  if (errorCode == VFS_STATUS_OK || lastError == VFS_STATUS_OK)
  {
    lastError = errorCode;
    return true;
  }
  // previous error was not cleared
  return false;
}

// Update last error code from system call
void NativeFile::UpdateError ()
{
  // Update error code if and only if previous error is cleared.
  if (lastError != VFS_STATUS_OK)
    return;

  // Skip if handle is invalid
  if (!handle)
    return;

  // call our helper function to determine status
  lastError = ErrnoToVfsStatus (errno);
}

// Reset and return last error status
int NativeFile::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

// Read DataSize bytes from file
size_t NativeFile::Read (char *data, size_t dataSize)
{
  if (!readOnly)
  {
    // cannot read a file opened with write permission; thus access denied
    lastError = VFS_STATUS_ACCESSDENIED;
    return 0;
  }

  if (!handle)
  {
    // TODO: double check for handling this case
    return 0;
  }
  // try reading...
  size_t bytesRead = fread (data, 1, dataSize, handle);
  if (bytesRead < dataSize)
  {
    // read less... what happened?
    UpdateError ();
  }
  return bytesRead;
}

// Write DataSize bytes to file
size_t NativeFile::Write (const char *data, size_t dataSize)
{
  if (readOnly)
  {
    // cannot write to a read-only file: access denied.
    lastError = VFS_STATUS_ACCESSDENIED;
    return 0;
  }

  if (!handle)
  {
    // TODO: double check for handling this case
    return 0;
  }
  // try writing
  size_t bytesWritten = fwrite (data, 1, dataSize, handle);
  if (bytesWritten < dataSize)
  {
    // written less... what happened?
    UpdateError ();
  }
  return bytesWritten;
}

// Flush strem
void NativeFile::Flush ()
{
  if (handle)
    fflush (handle);
}

// Check whether pointer is at End of File
bool NativeFile::AtEOF ()
{
  // if feof returns nonzero, file is at EOF
  if (handle)
    return feof (handle) != 0;

  return true;
}

// Query file pointer (absolute position)
uint64_t NativeFile::GetPos ()
{
  // TODO: ensure large file compatibility
  if (handle)
    return ftell (handle);

  return (uint64_t)-1;
}

// Set file pointer (relative position; absolute by default)
bool NativeFile::SetPos (off64_t newPos, int relativeTo)
{
  // currently, null handle will simply fail...
  if (!handle)
    return false;

  int mode; // seek mode for fseek ()

  switch (relativeTo)
  {
    case VFS_POS_CURRENT:
      mode = SEEK_CUR;
      break;
    case VFS_POS_END:
      mode = SEEK_END;
      break;
    case VFS_POS_ABSOLUTE:
    default:
      // absolute mode requested, or unknown constant
      mode = SEEK_SET;
      break;
  }

  // TODO: ensure large file compatibility
  // try seek
  if (fseek (handle, newPos, mode) != 0)
  {
    UpdateError ();
    return false;
  }

  return true;
}

// Get all data into a single buffer.
csPtr<iDataBuffer> NativeFile::GetAllData (bool nullTerminated)
{
  // we need read permission... check it
  if (!readOnly)
  {
    // update error code
    lastError = VFS_STATUS_ACCESSDENIED;
    return csPtr<iDataBuffer> (nullptr);
  }

  // TODO: implement mmap () feature
  // TODO: check how csVFS::heap works and fix if necessary.
  // it uses malloc () for now

  // Since client code doesn't know about buffer length, nullTerminated is
  // useless for now.

  char *data = (char *)cs_malloc(size + 1);
  if (!data)
  {
    // TODO: update error code
    return csPtr<iDataBuffer> (nullptr);
  }

  // backup pointer
  uint64_t oldPos = GetPos ();
  // set pointer at beginning
  SetPos (0);
  // read!
  size_t bytesRead = Read (data, size);
  data[bytesRead] = 0; // null-terminated.

  // TODO: add handling if bytesRead < size ?

  // restore pointer
  SetPos (oldPos);

  // increment the size to include null-terminator if required
  if (nullTerminated)
    ++bytesRead;

  return csPtr<iDataBuffer> (
    new CS::DataBuffer<Memory::AllocatorMalloc> (data, bytesRead));
}

// Get all data into a single buffer with custom allocator.
csPtr<iDataBuffer> NativeFile::GetAllData (CS::Memory::iAllocator *allocator)
{
  using CS::Memory::AllocatorInterface;

  // create iDataBuffer instance with custom allocator
  iDataBuffer *buffer =
    new CS::DataBuffer<AllocatorInterface> (size,
                                            AllocatorInterface (allocator));

  char *data = buffer->GetData();
  if (!data)
  {
    // TODO: update error code
    return csPtr<iDataBuffer> (nullptr);
  }

  // backup pointer
  uint64_t oldPos = GetPos ();
  // set pointer at beginning
  SetPos (0);
  // read!
  size_t bytesRead = Read (data, size);

  // TODO: add handling if bytesRead < size ?

  // restore pointer
  SetPos (oldPos);

  return csPtr<iDataBuffer> (buffer);
}

// Get subset of file as iFile
csPtr<iFile> NativeFile::GetPartialView (uint64_t offset, uint64_t size)
{
  // this doesn't really work with iFile not opened for read access
  if (!readOnly)
  {
    lastError = VFS_STATUS_ACCESSDENIED;
    return csPtr<iFile> (nullptr);
  }

  // TODO: check boundary conditions more thoroughly
  // make sure offset is within [0, file size]
  if (offset > this->size)
    offset = this->size;

  // make sure offset+size is within [offset, file size]
  // this is safe since offset <= this->size
  if (size > this->size - offset)
    size = this->size - offset;

  // only pass safe values to View constructor
  return csPtr<iFile> (new View (this, offset, size));
}

// NativeFile::View methods
/* View Constructor
 - takes parent (underlying NativeFile instance)
         offset (offset in parent NativeFile where this view starts)
         size   (size of this view)
  All parameters must be valid. The constructor assumes they are, and
  bypasses any checks boundary conditions.
 */ 
NativeFile::View::View (NativeFile *parent,
                        uint64_t offset,
                        uint64_t size) :
 scfImplementationType (this, parent),
 pos (0),
 lastError (VFS_STATUS_OK)
{
  // assign members as appropriate
  this->parent = parent;
  this->offset = offset;
  this->size   = size;
}

// NativeFile::View destructor
NativeFile::View::~View ()
{
  // Free memory and resources
  parent = 0;
}

// Reset and return last error status
int NativeFile::View::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

// Read DataSize bytes from file
size_t NativeFile::View::Read (char *data, size_t dataSize)
{
  // Save last error status
  int parentError = parent->lastError;
  // Save last position
  uint64_t lastPos = parent->GetPos ();

  // Apply offset of this view
  parent->SetPos (offset + pos);
  // Reset parent error so we can collect status info
  parent->lastError = VFS_STATUS_OK;
  // make sure read doesn't exceed view boundary
  size_t toRead = (size - pos < dataSize) ? size - pos : dataSize;

  // try reading...
  size_t bytesRead = Read (data, toRead);
  // update last error of this view..
  if (lastError == VFS_STATUS_OK)
    lastError = parent->lastError;

  // restore parent's last position
  parent->SetPos (lastPos);
  // restore parent's last error
  parent->lastError = parentError;

  return bytesRead;
}

// Write DataSize bytes to file
size_t NativeFile::View::Write (const char *data, size_t dataSize)
{
  // partial view is read-only; access denied.
  lastError = VFS_STATUS_ACCESSDENIED;
  return 0;
}

// Flush strem
void NativeFile::View::Flush ()
{
  // you can't flush a read-only view
  // therefore do nothing.
}

// Check whether pointer is at End of File
bool NativeFile::View::AtEOF ()
{
  return pos >= size;
}

// Query file pointer (absolute position)
uint64_t NativeFile::View::GetPos ()
{
  return pos;
}

// Set file pointer (relative position; absolute by default)
bool NativeFile::View::SetPos (off64_t newPos, int relativeTo)
{
  // is newPos negative (backwards)
  bool negative = newPos < 0;
  // take absolute value
  uint64_t distance = negative ? -newPos : newPos;
  // only virtual pointer is moved
  switch (relativeTo)
  {
    case VFS_POS_CURRENT:
      // relative to current position
      if (negative) // remember, this is unsigned arithmetic
        pos = (pos < distance) ? 0 : pos - distance;
      else
        pos += distance;
      break;
    case VFS_POS_END:
      // relative to end of view
      if (negative)
        pos = (size < distance) ? 0 : size - distance;
      else
        pos = size;
      break;
    case VFS_POS_ABSOLUTE:
    default:
      // absolute mode requested, or unknown constant
      pos = ((uint64_t)newPos > size) ? size : newPos;
      break;
  }

  return true;
}

// Get all data into a single buffer.
csPtr<iDataBuffer> NativeFile::View::GetAllData (bool nullTerminated)
{
  // TODO: implement mmap () feature
  // TODO: use private heap for allocation
  // it uses malloc () for now

  // Since client code doesn't know about buffer length, nullTerminated is
  // useless for now.

  char *data = (char *)cs_malloc(size + 1);
  if (!data)
  {
    // TODO: update error code
    return csPtr<iDataBuffer> (nullptr);
  }

  // backup pointer
  uint64_t oldPos = GetPos ();
  // set pointer at beginning
  SetPos (0);
  // read!
  size_t bytesRead = Read (data, size);
  data[bytesRead] = 0; // null-terminated.

  // TODO: add handling if bytesRead < size ?

  // restore pointer
  SetPos (oldPos);

  if (nullTerminated)
    ++bytesRead;

  return csPtr<iDataBuffer> (
    new CS::DataBuffer<Memory::AllocatorMalloc> (data, bytesRead));
}

// Get all data into a single buffer with custom allocator.
// - takes pointer of iAllocator interface to be used for memory allocation
// - returns smart pointer to iDataBuffer containing requested data
csPtr<iDataBuffer> 
NativeFile::View::GetAllData (CS::Memory::iAllocator *allocator)
{

  using CS::Memory::AllocatorInterface;

  // create iDataBuffer instance with custom allocator
  iDataBuffer *buffer =
    new CS::DataBuffer<AllocatorInterface> (size,
                                            AllocatorInterface (allocator));

  char *data = buffer->GetData();
  if (!data)
  {
    // TODO: update error code
    return csPtr<iDataBuffer> (nullptr);
  }

  // backup pointer
  uint64_t oldPos = GetPos ();
  // set pointer at beginning
  SetPos (0);
  // read!
  size_t bytesRead = Read (data, size);

  // TODO: add handling if bytesRead < size ?

  // restore pointer
  SetPos (oldPos);

  return csPtr<iDataBuffer> (buffer);
}

// Get subset of file as iFile
csPtr<iFile> NativeFile::View::GetPartialView (uint64_t offset,
                                               uint64_t size)
{
  // TODO: check boundary conditions more thoroughly
  // make sure offset is within [0, current view size]
  if (offset > this->size)
    offset = this->size;

  // make sure offset+size is within [offset, current view size]
  // this is safe since offset <= this->size
  if (size > this->size - offset)
    size = this->size - offset;

  // only pass safe values to View constructor
  return csPtr<iFile> (new View (parent, this->offset + offset, size));
}

// NativeFS methods
/* NativeFS constructor
 - takes realPath (real path as mount root of this particular instance)
 - Remarks: realPath is assumed to be non-null and end with trailing path
     separator (e.g. slash, backslash) before null-terminator
 */
NativeFS::NativeFS (const char *realPath) : scfImplementationType (this)
{
  // Store mount root (real path) of this instance
  size_t mountRootLen = strlen(realPath) + 1; // +1 for null-terminator
  mountRoot = (char *)cs_malloc(mountRootLen);
  memcpy(mountRoot, realPath, mountRootLen);
  // Set last error to VFS_STATUS_OK
  lastError = VFS_STATUS_OK;
}

// NativeFS destructor
NativeFS::~NativeFS ()
{
  // clean up resources
  cs_free(mountRoot);
}

/* Convert virtual path to real path
 - takes virtualPath (virtual path to convert)
 - returns csString containing full native path corresponding to given
           virtual path
 */
csString NativeFS::ToRealPath (const char *virtualPath)
{
  // string constants to use
  static const char search[] = { VFS_PATH_SEPARATOR, 0 };
  static const char replace[] = { CS_PATH_SEPARATOR, 0 };

  csString path (mountRoot);
  csString suffix (virtualPath);
  // replace virtual path separators to platform specific ones
  suffix.ReplaceAll (search, replace);
  // append rest of the path to the base
  path << suffix;

  return path;
}

// Set last error status
bool NativeFS::SetLastError (int errorCode)
{
  // If new status is VFS_STATUS_OK, do it anyway.
  // Otherwise, update error code if and only if previous error is cleared.
  if (errorCode == VFS_STATUS_OK || lastError == VFS_STATUS_OK)
  {
    lastError = errorCode;
    return true;
  }
  // previous error was not cleared
  return false;
}

// Update error code from last system call
void NativeFS::UpdateError ()
{  
  // Update error code if and only if previous error is cleared.
  if (lastError != VFS_STATUS_OK)
    return;

  // use helper function to get corresponding VFS status
  lastError = ErrnoToVfsStatus (errno);
}

// Open a particular file
csPtr<iFile> NativeFS::Open (const char *path,
                             const char *pathPrefix,
                             int mode,
                             bool /*useCaching*/)
{
  // TODO: implement checks for boundary cases


  // setup required parameters;
  csString realPath (ToRealPath (path));
  csString vfsPath (pathPrefix);

  // append VFS path separator if needed
  if ((vfsPath[vfsPath.Length ()-1] != VFS_PATH_SEPARATOR)
      && (path[0] != VFS_PATH_SEPARATOR))
    vfsPath << VFS_PATH_SEPARATOR;

  // append rest of the path to the base
  vfsPath << path;

  // try creating an instance
  iFile *file = new NativeFile (this, vfsPath, realPath, mode);

  // did it go well?
  switch (lastError = file->GetStatus ())
  {
    case VFS_STATUS_OK:
      // ok!
      break;
    default:
      // error happened; invalidate csPtr
      delete file;
      file = nullptr;
      break;
  }

  return csPtr<iFile> (file);
}

// Move file from OldPath to NewPath
bool NativeFS::Move (const char *oldPath, const char *newPath)
{
  // This only works within current file system.
  SetLastError (VFS_STATUS_NOTIMPLEMENTED);
  return false;
}

// Get permission set of a file
bool NativeFS::GetPermission (const char *filename, csFilePermission &oPerm)
{
  csString path (ToRealPath (filename));

  uint32 permission;
  int error = CS::Platform::GetFilePermission (path, permission);

  // use GetFilePermission () to get permission
  if (error != 0)
  {
    // GetFilePermission() failed; update error status
    SetLastError (ErrnoToVfsStatus (error));
    return false;
  }

  // Using obtained information, fill in the structure using constructor
  // st_mode is bitwise compatible with traditional octal representation
  oPerm = permission;

  return true;
}

// Set permission set of a file
bool NativeFS::SetPermission (const char *filename,
                              const csFilePermission &iPerm)
{
  // get real path from given virtual filename
  csString path (ToRealPath (filename));

  // FIXME: currently it only supports user/group/others r/w/x permissions.
  // what about S_ISUID, S_ISGID, S_ISVTX and such special fields?

  // generate numeric mode_t parameter from csFilePermission
  // multiplication is used to reduce branching
  mode_t mode = (S_IRUSR * (iPerm.user_read      != 0))
             || (S_IWUSR * (iPerm.user_write     != 0))
             || (S_IXUSR * (iPerm.user_execute   != 0))
             || (S_IRGRP * (iPerm.group_read     != 0))
             || (S_IWGRP * (iPerm.group_write    != 0))
             || (S_IXGRP * (iPerm.group_execute  != 0))
             || (S_IROTH * (iPerm.others_read    != 0))
             || (S_IWOTH * (iPerm.others_write   != 0))
             || (S_IXOTH * (iPerm.others_execute != 0));

  // use SetFilePermission () to apply desired permission
  int error = CS::Platform::SetFilePermission (path, mode);
  if (error != 0)
  {
    // SetFilePermission () failed...
    SetLastError (ErrnoToVfsStatus (error));
    return false;
  }

  return true;
}

// Set file time
bool NativeFS::GetTime (const char *filename, csFileTime &oTime)
{
  // get real path from given virtual filename
  csString path (ToRealPath (filename));
  struct stat info; // struct to store file information

  // retrieve file information with stat()
  if (stat (path, &info) != 0)
  {
    // stat() failed... update error status
    UpdateError ();
    return false;
  }

  // Using obtained information, fill in the structure
  const time_t mtime = info.st_mtime;
  struct tm *localtm = localtime (&mtime);
  oTime = *localtm;

  return true;
}

// Get file time
bool NativeFS::SetTime (const char *filename, const csFileTime &iTime)
{
  csString path (ToRealPath (filename)); // obtain real path from fileName
  struct tm curtm = iTime;
  struct utimbuf times;

  times.actime = mktime (&curtm); // access time
  times.modtime = times.actime;  // modification time

  // try updating time...
  int result = utime (path, &times);

  if (result != 0)
  {
    // operation failed.
    UpdateError ();
    return false;
  }
  return true;
}

// Get file size
bool NativeFS::GetSize (const char *filename, uint64_t &oSize)
{
  csString path (ToRealPath (filename));
  struct stat info;
  // TODO: ensure large file compatibility
  if (stat (path, &info) != 0)
  {
    // failure; update error code
    UpdateError ();
    return false;
  }
  // copy the information
  oSize = info.st_size;

  return true;
}

// Determines whether a file exists
bool NativeFS::Exists (const char *filename)
{
  // find corresponding real path
  csString path (ToRealPath (filename));

  // use access () with F_OK to test file existence
  return (access (path, F_OK) == 0);
}

bool NativeFS::Delete (const char *filename)
{  
  // find corresponding real path
  csString path (ToRealPath (filename));

  // remove trailing path separator
  if (path[path.Length ()-1] == CS_PATH_SEPARATOR)
    path.Truncate (path.Length () - 1);

  struct stat info;

  // retrieve file information
  if (stat (path, &info) != 0)
  {
    // stat() failed for whatever reason
    UpdateError ();
    return false;
  }


  if (info.st_mode & _S_IFDIR)
  {
    // file is a directory
    if (rmdir (path) != 0)
    {
      // rmdir failed for whatever reason
      UpdateError ();
      return false;
    }
  }
  else
  {
    if (unlink (path) != 0)
    {
      // unlink () failed for whatever reason
      UpdateError ();
      return false;
    }
  }

  return true;
}

// Query real path of filesystem root
csString NativeFS::GetRootRealPath ()
{
  return csString (mountRoot);
}

// Query and reset last error status
int NativeFS::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

}
CS_PLUGIN_NAMESPACE_END(VFS)
