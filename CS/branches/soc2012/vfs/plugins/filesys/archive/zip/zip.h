/*
    Crystal Space Virtual File System Zip Archive Support
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

#ifndef __CS_VFS_ZIP_H__
#define __CS_VFS_ZIP_H__

#include "csutil/scf_implementation.h"
#include "csutil/threading/rwmutex.h"
#include "cstool/vfsfilebase.h"
#include "iutil/vfs.h"
#include "iutil/comp.h"

CS_PLUGIN_NAMESPACE_BEGIN (VFS_ARCHIVE_ZIP)
{

class ZipArchive;
class csZipFSHandler;

// Zip acrhive filesystem class for VFS
class csZipFS : public scfImplementation1<csZipFS, iFileSystem>
{
  // let factory access the constructor
  friend class csZipFSHandler;

  // mutex for thread-safe access
  CS::Threading::RecursiveMutex mutex;

  // underlying archive of current filesystem
  csRef<ZipArchive> archive;
  // root path of filesystem, relative to archive root
  csString root;
  // last error status
  int lastError;

  // constructor
  csZipFS (ZipArchive *archive, const char *rootPath);

public:
  // destructor
  virtual ~csZipFS ();

  virtual csPtr<iFile> Open (const char *path,
                             const char *pathPrefix,
                             int mode,
                             const csVfsOptionList &options);

  virtual bool Move (const char *oldPath, const char *newPath);

  virtual bool GetPermission (const char *filename,
                              csFilePermission &oPerm);

  virtual bool SetPermission (const char *filename,
                              const csFilePermission &iPerm);

  virtual bool GetTime (const char *filename, csFileTime &oTime);

  virtual bool SetTime (const char *filename, const csFileTime &iTime);

  virtual bool GetSize (const char *filename, uint64_t &oSize);

  virtual bool Exists (const char *filename);

  virtual bool Delete (const char *filename);

  virtual csPtr<iStringArray> List (const char *path);

  virtual csString GetRootRealPath ();

  virtual int GetStatus ();

  virtual bool IsDir (const char *path);

  virtual bool Flush ();

private:
  // Convenience method; try getting archive entry handle of given vfs path
  void *GetEntry (const char *vfsPath);
};

// Virtual file (iFile) for files within zip archive
class csZipArchiveFile :
  public scfImplementationExt0<csZipArchiveFile, csVfsFileBase>
{
  friend class csZipFS;

  // internal class for partial view support
  class View;

  // parent archive FS of this file
  csRef<csZipFS> archive;

  // constructor
  csZipArchiveFile (csZipFS *parent);

public:
  // destructor
  virtual ~csZipArchiveFile ();
  // query file name
  virtual const char *GetName ();
  // query file size
  virtual uint64_t GetSize ();
  // Read from file
  virtual size_t Read (char *data, size_t dataSize);
  // Write to file
  virtual size_t Write (const char *data, size_t dataSize);
  // Flush changes of file
  virtual void Flush ();
  // Query whether pointer is at end of file
  virtual bool AtEOF ();
  // Query current file pointer
  virtual uint64_t GetPos ();
  // Set new position of file pointer
  virtual bool SetPos (off64_t newpos, int ref = VFS_POS_ABSOLUTE);
  // Request whole content of the file as a single buffer.
  virtual csPtr<iDataBuffer> GetAllData (bool nullterm = false);
  // Request whole content of the file as a single buffer.
  // Uses custom allocator.
  virtual csPtr<iDataBuffer> GetAllData (CS::Memory::iAllocator *allocator);
  // Request part of the file as an iFile
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0);
};

// Factory/archive handler for zip filesystem
class csZipFSHandler : public scfImplementation3<csZipFSHandler,
                                                 iFileSystemFactory,
                                                 iArchiveHandler,
                                                 iComponent>
{
public:
  // constructor
  csZipFSHandler (iBase *parent);
  // destructor
  virtual ~csZipFSHandler ();
  // iComponent initializer
  virtual bool Initialize (iObjectRegistry *objRegistry);

  // iFileSystemFactory methods
  virtual iFileSystem *Create (const char *realPath, int &oStatus);
  // iArchiveHandler methods
  virtual iFileSystem *GetFileSystem (iFileSystem *parentFS,
                                      const char *archivePath,
                                      const char *suffix);
};

}
CS_PLUGIN_NAMESPACE_END (VFS_ARCHIVE_ZIP)

#endif // !__CS_VFS_ZIP_H__
