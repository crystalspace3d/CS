/*
    Crystal Space Virtual File System Plugin classes
    Copyright (C) 2006 by Brandon Hamilton <brandon.hamilton@gmail.com>

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

#ifndef __CS_FILESYSTEM_H__
#define __CS_FILESYSTEM_H__

#include "csutil/scf_implementation.h"
#include "csutil/physfile.h"
#include "iutil/vfs.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"

CS_PLUGIN_NAMESPACE_BEGIN(vfs)
{

class csVFS;
class VfsNode;

/**
 * TODO: write class description
 */
class csFileSystem 
: public scfImplementation2<csFileSystem, iFileSystem, iComponent>
{
public:

  /**
   * TODO: write class description
   */
  class csFile : public scfImplementation1<csFile, iFile>
  {
    protected:
	  // File size (initialized in constructor)
      size_t Size;
      // Filename in VFS
      char *Name;
	  // Error status
	  int Error;
	  // The constructor for csFile
      csFile (const char *Name);
    public:
      /// Instead of fclose() do "delete file" or file->DecRef ()
      virtual ~csFile ();
	  /// Query file name (in VFS)
      virtual const char *GetName () { return Name; }
      /// Query file size
      virtual size_t GetSize () { return Size; }
      /// Check (and clear) file last error status
      virtual int GetStatus ();
	  /// Replacement for standard fread()
      virtual size_t Read (char *Data, size_t DataSize) = 0;
      /// Replacement for standard fwrite()
      virtual size_t Write (const char *Data, size_t DataSize) = 0;
      /// Flush stream.
      virtual void Flush () = 0;
      /// Replacement for standard feof()
      virtual bool AtEOF () = 0;
      /// Query current file pointer
      virtual size_t GetPos () = 0;
      /// Get entire file data at once, if possible, or 0
      virtual csPtr<iDataBuffer> GetAllData () = 0;
      /// Set new file pointer
      virtual bool SetPos (size_t newpos) = 0;
  };

  /// Vritual Destructor
  virtual ~csFileSystem ();

  /// Initialize the File System Plugin
  virtual bool Initialize (iObjectRegistry *object_reg);

  /// Replacement for standard fopen()
  virtual csPtr<iFile> Open(const char * FileName, int mode) = 0;

  /**
   * Get an entire file at once. You should delete[] returned data
   * after usage. This is more effective than opening files and reading
   * the file in blocks.  Note that the returned buffer is always null-
   * terminated (so that it can be conveniently used with string functions)
   * but the extra null-terminator is not counted as part of the returned 
   * size.
   */
  virtual csPtr<iDataBuffer> ReadFile(const char * FileName) = 0;

  /// Write an entire file in one pass.
  virtual bool WriteFile(const char * Name, const char * Data, size_t Size) 
	  = 0;

  /// Delete a file on VFS
  virtual bool DeleteFile(const char * FileName) = 0;

  // Check if must be abstract
  virtual csVFSFileKind Exists(const char * FileName)  = 0;


protected:
  /// The constructor for csFileSystem
  csFileSystem(iBase *iParent = 0);

  // Reference to the object registry.
  iObjectRegistry *object_reg;

  friend class csVFS;
};

/**
 * TODO: write class description
 * The native file system plugin will use csPhysicalFile for access to the physical file system
 */
class csNativeFileSystem 
	: public scfImplementationExt0<csNativeFileSystem, csFileSystem>
{
public:
  /// The constructor for csFileSystem
  csNativeFileSystem(iBase *iParent = 0);

  /// Virtual destructor
  virtual ~csNativeFileSystem();

	/// Replacement for standard fopen()
  virtual csPtr<iFile> Open(const char * FileName, int mode);

  /**
   * Get an entire file at once. You should delete[] returned data
   * after usage. This is more effective than opening files and reading
   * the file in blocks.  Note that the returned buffer is always null-
   * terminated (so that it can be conveniently used with string functions)
   * but the extra null-terminator is not counted as part of the returned size.
   */
  virtual csPtr<iDataBuffer> ReadFile(const char * FileName);

  /// Write an entire file in one pass.
  virtual bool WriteFile(const char * Name, const char * Data, size_t Size);

  /// Delete a file on VFS
  virtual bool DeleteFile(const char * FileName);

  // Check if must be abstract
  virtual csVFSFileKind Exists(const char * FileName);
};


/**
 * TODO: write class description
 */
class csArchiveFileSystem 
	: public scfImplementationExt0<csArchiveFileSystem, csFileSystem>
{
public:
  /// The constructor for csFileSystem
  csArchiveFileSystem(iBase *iParent = 0);

  /// Virtual destructor
  virtual ~csArchiveFileSystem();

	/// Replacement for standard fopen()
  virtual csPtr<iFile> Open(const char * FileName, int mode);

  /**
   * Get an entire file at once. You should delete[] returned data
   * after usage. This is more effective than opening files and reading
   * the file in blocks.  Note that the returned buffer is always null-
   * terminated (so that it can be conveniently used with string functions)
   * but the extra null-terminator is not counted as part of the returned size.
   */
  virtual csPtr<iDataBuffer> ReadFile(const char * FileName);

  /// Write an entire file in one pass.
  virtual bool WriteFile(const char * Name, const char * Data, size_t Size);

  /// Delete a file on VFS
  virtual bool DeleteFile(const char * FileName);

  // Check if must be abstract
  virtual csVFSFileKind Exists(const char * FileName);
};

} CS_PLUGIN_NAMESPACE_END(vfs)

#endif  // __CS_FILESYSTEM_H__

