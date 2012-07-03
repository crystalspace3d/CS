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

// Win32 utime() workaround
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

// local helper functions
namespace
{
  // Replace occurences of 'Old' characters in String to 'New'
  char * ReplaceChar (char * String, char Old, char New)
  {
    if (!String)
      return nullptr;

    // replace all occurences; assume String is null-terminated.
    for (char *curr = String; curr != '\0'; ++curr)
      if (*curr == Old)
        *curr = New;

    return String;
  }
}


// this belongs to VFS namespace, for now.
CS_PLUGIN_NAMESPACE_BEGIN(VFS)
{

// Forward declaration
class NativeFS;

// OS Native iFile
class NativeFile : public scfImplementation1<NativeFile, iFile>
{
  class NativeFileView;

  // Fully-qualified VFS path of file
  char *VFSPath;
  // Real path
  char *RealPath;
  // File size
  uint64_t Size;
  // Last error
  int LastError;
  // Native file handle
  FILE *Handle;
  // Read-only?
  bool ReadOnly;

  // Constructor
  NativeFile (const char *VFSPath, const char *RealPath, int Mode);
  // Set last error to specified value
  bool SetLastError (int ErrorCode);
  // Update last error code from system call
  void UpdateError ();
public:
  // Destructor
  virtual ~NativeFile ();
  // Query VFS filename
  virtual const char *GetName () { return VFSPath; }
  // Query file size
  virtual uint64_t GetSize () { return Size; }
  // Query and reset last error status
  virtual int GetStatus ();
  // Read Length bytes into the buffer at which Data points.
  virtual size_t Read (char *Data, size_t Length);
  // Write Length bytes from the buffer at which Data points.
  virtual size_t Write (const char *Data, size_t Length);
  // Flush strem
  virtual void Flush ();
  // Check whether pointer is at End of File
  virtual bool AtEOF ();
  // Query file pointer (absolute position)
  virtual uint64_t GetPos ();
  // Set file pointer (relative position; absolute by default)
  virtual bool SetPos (off64_t NewPos, int RelativeTo = VFS_POS_ABSOLUTE);
  // Get all data into a single buffer.
  virtual csPtr<iDataBuffer> GetAllData (bool NullTerminated = false);
  // Get all data into a single buffer with custom allocator.
  virtual csPtr<iDataBuffer> GetAllData (CS::Memory::iAllocator *allocator);
  // Get subset of file as iFile
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0);

  friend class NativeFileView;
  friend class NativeFS;
};


// OS Native iFileSystem
class NativeFS : public scfImplementation1<NativeFS, iFileSystem>
{
  // Real-world mountpoint of this instance
  char *MountRoot;
  // Last error status
  int LastError;

  // Construct a NativeFS instance with 'RealPath' as mount root.
  NativeFS (const char *RealPath);
  // Convert relative virtual path to real path
  csString ToRealPath (const char *VirtualPath);
  // Set last error to specified value
  bool SetLastError (int ErrorCode);
  // Update last error code from system call
  void UpdateError ();

public:

  // Destructor
  virtual ~NativeFS ();
  // Open a file of given path and return iFile pointer
  virtual csPtr<iFile> Open (const char *Path,
                             const char *PathPrefix,
                             int Mode,
                             bool UseCaching);
  // Move file from OldPath to NewPath
  virtual bool Move (const char *OldPath, const char *NewPath);
  // Get permission set of a file
  virtual bool GetPermission (const char *FileName, csFilePermission &oPerm);
  // Set permission set of a file
  virtual bool SetPermission (const char *FileName,
                              const csFilePermission &iPerm);
  // Set file time
  virtual bool GetTime (const char *FileName, csFileTime &oTime);
  // Get file time
  virtual bool SetTime (const char *FileName, const csFileTime &iTime);
  // Get file size
  virtual bool GetSize (const char *FileName, uint64_t &oSize);
  // Query and reset last error status
  virtual int GetStatus ();
};

class NativeFile::NativeFileView
{
protected:

public:

};

// NativeFile methods
// NativeFile Constructor
NativeFile::NativeFile (const char *VFSPath, const char *RealPath, int Mode) :
 scfImplementationType(this)
{
  // TODO: Add debug messages

  // Measure required length
  size_t VFSPathLen  = strlen (VFSPath) + 1;
  size_t RealPathLen = strlen (RealPath) + 1;
  // Allocate buffer
  this->VFSPath  = (char *)cs_malloc (VFSPathLen);
  this->RealPath = (char *)cs_malloc (RealPathLen);
  // Copy string content
  memcpy (this->VFSPath,  VFSPath,  VFSPathLen);
  memcpy (this->RealPath, RealPath, RealPathLen);

  bool retry = false;
  int filemode = Mode & VFS_FILE_MODE;
  ReadOnly = (filemode == VFS_FILE_READ);
  do
  {
    // Open file handle with specified mode
    switch (filemode)
    {
      case VFS_FILE_WRITE:
        // write only
        Handle = CS::Platform::File::Open (RealPath, "wb");
        break;
      case VFS_FILE_APPEND:
        // append mode
        Handle = CS::Platform::File::Open (RealPath, "ab");
        break;
      case VFS_FILE_READ:
      default:
        // read only or unknown mode
        Handle = CS::Platform::File::Open (RealPath, "rb");
        break;
    }

    // if this is a retry, exit the loop
    if (retry)
      break;

    if (!Handle)
    {
      // somehow failed to open; try creating directory and try again
      if (filemode != VFS_FILE_READ)
      {
        // TODO: create missing directory tree
        retry = true;
      }
    }
  } while (retry);

  // is this a regular file? (i.e. not fifo, etc.)
  if (Handle && !CS::Platform::IsRegularFile (RealPath))
  {
    // close the handle
    fclose(Handle);
    Handle = nullptr;
  }

  if (!Handle)
  {
    // Somehow failed to open file
    // TODO: error handling code
  }
  else
  {
    // Store the size
    // TODO: implement 64-bit fail-proof handling
    if (fseek (Handle, 0, SEEK_END) == 0)
    {
      // fseek succeeded
      Size = ftell (Handle);
      if (Size == (uint64_t)-1)
      {
        // cannot possibly have this size
        Size = 0;
        // TODO: handle errors
      }
    }
    else
      ; // TODO: handle errors

    // for read or write mode, reposition file pointer
    if (filemode != VFS_FILE_APPEND)
      rewind (Handle);
  }
}

// NativeFile destructor
NativeFile::~NativeFile ()
{
  // Free memory and resources
  cs_free(VFSPath);
  cs_free(RealPath);
}

// Set last error to specified value
bool NativeFile::SetLastError (int ErrorCode)
{
  // If new status is VFS_STATUS_OK, do it anyway.
  // Otherwise, update error code if and only if previous error is cleared.
  if (ErrorCode == VFS_STATUS_OK || LastError == VFS_STATUS_OK)
  {
    LastError = ErrorCode;
    return true;
  }
  // previous error was not cleared
  return false;
}

// Update last error code from system call
void NativeFile::UpdateError ()
{
  // Update error code if and only if previous error is cleared.
  if (LastError == VFS_STATUS_OK)
    return;

  // Skip if handle is invalid
  if (!Handle)
    return;

  switch (errno)
  {
  default:
    break;
  }
}

// Reset and return last error status
int NativeFile::GetStatus ()
{
  int status = LastError;
  LastError = VFS_STATUS_OK;
  return status;
}

// Read DataSize bytes from file
size_t NativeFile::Read (char *Data, size_t DataSize)
{
  if (!ReadOnly)
  {
    // cannot read a file opened with write permission; thus access denied
    LastError = VFS_STATUS_ACCESSDENIED;
  }

  return 0;
}

// Write DataSize bytes to file
size_t NativeFile::Write (const char *Data, size_t DataSize)
{
  if (ReadOnly)
  {
    // cannot write to a read-only file: access denied.
    LastError = VFS_STATUS_ACCESSDENIED;
  }

  return 0;
}

// Flush strem
void NativeFile::Flush ()
{
  if (Handle)
    fflush (Handle);
}

// Check whether pointer is at End of File
bool NativeFile::AtEOF ()
{
  // if feof returns nonzero, file is at EOF
  if (Handle)
    return feof (Handle) != 0;

  return true;
}

// Query file pointer (absolute position)
uint64_t NativeFile::GetPos ()
{
  // TODO: ensure large file compatibility
  if (Handle)
    return ftell (Handle);

  return (uint64_t)-1;
}

// Set file pointer (relative position; absolute by default)
bool NativeFile::SetPos (off64_t NewPos, int RelativeTo)
{
  // currently, null handle will simply fail...
  if (!Handle)
    return false;

  int mode; // seek mode for fseek ()

  switch (RelativeTo)
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
  if (fseek (Handle, NewPos, mode) != 0)
  {
    UpdateError ();
    return false;
  }

  return true;
}

// Get all data into a single buffer.
csPtr<iDataBuffer> NativeFile::GetAllData (bool NullTerminated)
{
  // TODO: this is currently a stub; implement feature
  iDataBuffer *buffer = nullptr;

  return csPtr<iDataBuffer> (buffer);
}

// Get all data into a single buffer with custom allocator.
csPtr<iDataBuffer> NativeFile::GetAllData (CS::Memory::iAllocator *allocator)
{
  // TODO: this is currently a stub; implement feature
  iDataBuffer *buffer = nullptr;

  return csPtr<iDataBuffer> (buffer);
}

// Get subset of file as iFile
csPtr<iFile> NativeFile::GetPartialView (uint64_t offset, uint64_t size)
{
  // TODO: this is currently a stub; implement feature
  iFile *file = nullptr;

  return csPtr<iFile> (file);
}

// NativeFS methods

// NativeFS constructor
NativeFS::NativeFS (const char *RealPath) : scfImplementationType(this)
{
  // Store mount root (real path) of this instance
  size_t MountRootLen = strlen(RealPath) + 1; // +1 for null-terminator
  MountRoot = (char *)cs_malloc(MountRootLen);
  memcpy(MountRoot, RealPath, MountRootLen);
  // Set last error to VFS_STATUS_OK
  LastError = VFS_STATUS_OK;
}

// NativeFS destructor
NativeFS::~NativeFS ()
{
  // clean up resources
  cs_free(MountRoot);
}

// Convert virtual path to real path
csString NativeFS::ToRealPath (const char *VirtualPath)
{
  // string constants to use
  static const char search[] = { VFS_PATH_SEPARATOR, 0 };
  static const char replace[] = { CS_PATH_SEPARATOR, 0 };

  csString path (MountRoot);
  csString suffix (VirtualPath);
  // replace virtual path separators to platform specific ones
  suffix.ReplaceAll (search, replace);
  // append rest of the path to the base
  path << suffix;

  return path;
}

// Set last error status
bool NativeFS::SetLastError (int ErrorCode)
{
  // If new status is VFS_STATUS_OK, do it anyway.
  // Otherwise, update error code if and only if previous error is cleared.
  if (ErrorCode == VFS_STATUS_OK || LastError == VFS_STATUS_OK)
  {
    LastError = ErrorCode;
    return true;
  }
  // previous error was not cleared
  return false;
}

// Update error code from last system call
void NativeFS::UpdateError ()
{  
  // Update error code if and only if previous error is cleared.
  if (LastError == VFS_STATUS_OK)
    return;

  switch (errno)
  {
  default:
    break;
  }
}

// Open a particular file
csPtr<iFile> NativeFS::Open (const char *Path,
                             const char *PathPrefix,
                             int Mode,
                             bool UseCaching)
{
  // TODO: implement checks for boundary cases


  // setup required parameters;
  csString RealPath (ToRealPath (Path));
  csString VFSPath (PathPrefix);

  // append VFS path separator if needed
  if ((VFSPath[VFSPath.Length ()-1] != VFS_PATH_SEPARATOR)
      && (Path[0] != VFS_PATH_SEPARATOR))
    VFSPath << VFS_PATH_SEPARATOR;

  // append rest of the path to the base
  VFSPath << Path;

  // try creating an instance
  iFile *file = new NativeFile (VFSPath, RealPath, Mode);

  // did it go well?
  switch (LastError = file->GetStatus ())
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
bool NativeFS::Move (const char *OldPath, const char *NewPath)
{
  // This only works within current file system.
  SetLastError (VFS_STATUS_NOTIMPLEMENTED);
  return false;
}

// Get permission set of a file
bool NativeFS::GetPermission (const char *FileName, csFilePermission &oPerm)
{
  csString path (ToRealPath (FileName));
  struct stat info;

#if defined(WIN32) && !defined(__CYGWIN__) && !defined(__CYGWIN32__)
  // TODO: implement Win32 emulated permission
  SetLastError (VFS_STATUS_UNSUPPORTED);
  return false;
#endif
  if (stat (path, &info) != 0)
  {
    // stat() failed... update error status
    UpdateError ();
    return false;
  }

  // Using obtained information, fill in the structure
  csFilePermission permission (info.st_mode);
  // done; copy contents
  oPerm = permission;

  return true;
}

// Set permission set of a file
bool NativeFS::SetPermission (const char *FileName,
                              const csFilePermission &iPerm)
{
  // TODO: implement feature
  SetLastError (VFS_STATUS_NOTIMPLEMENTED);
  return false;
}

// Set file time
bool NativeFS::GetTime (const char *FileName, csFileTime &oTime)
{
  csString path (ToRealPath (FileName));
  struct stat info;

  // retrieve file information with stat()
  int result = stat (path, &info);

  if (result != 0)
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
bool NativeFS::SetTime (const char *FileName, const csFileTime &iTime)
{
  // TODO: implement feature
  csString path (ToRealPath (FileName));
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
bool NativeFS::GetSize (const char *FileName, uint64_t &oSize)
{
  csString path (ToRealPath (FileName));
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

// Query and reset last error status
int NativeFS::GetStatus ()
{
  int status = LastError;
  LastError = VFS_STATUS_OK;
  return status;
}

}
CS_PLUGIN_NAMESPACE_END(VFS)
