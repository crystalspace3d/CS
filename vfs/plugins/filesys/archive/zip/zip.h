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
#include "csutil/databuf.h"
#include "iutil/vfs.h"
#include "iutil/comp.h"

CS_PLUGIN_NAMESPACE_BEGIN (VFS_ARCHIVE_ZIP)
{
// forward declarations
class ArchiveCache;
class csZipFS;
class csZipFSHandler;
// constants
const size_t DEFAULT_BUFFER_SIZE = 1024; // 1 KBytes
const int    CACHE_OK            = 0; // no updates, no reference
const int    CACHE_UPDATED       = 1; // cache has been updated
const int    CACHE_PACK_ON_WRITE = 4; // compress on write

// Custom csArchive that uses iFileSystem-based file handling
class ZipArchive : public csArchive, public CS::Utility::AtomicRefCount
{
  friend class ArchiveCache;
  friend class csZipFS;

private:
  iFileSystem *parent;
  csString identifier;

  // cache lookup table
  csHash<size_t, csString> cacheLookup;
  // actual data cache; required to allow partial read/write
  csRefArray<iDataBuffer> cache;
  // flag showing cache status
  csArray<int> cacheFlags;
  // valid size of individual cached files
  csArray<size_t> cacheSize;
  
  // mutex for thread-safe access
  CS::Threading::RecursiveMutex mutex;

  // overridden to support nested archives
  virtual csPtr<iFile> OpenBaseFile (int mode);

  // constructor
  ZipArchive (const char *identifier,
              const char *path,
              iFileSystem *parentFS = nullptr);
public:
  // Flush
  virtual bool Flush ();
  // Get archive identifier
  void GetIdentifier (csString &oIdentifier);
  // Get size of given file
  bool GetSize (const char *path, uint64_t &oSize);
  // Determine whether a file exists
  bool Exists (const char *path);
  // Flush cache
  void FlushCache ();
  // Purge entire cache
  void PurgeCache ();
  // Purge individual entry in cache
  void PurgeCache (const csString &path);
};

// Zip acrhive filesystem class for VFS
class csZipFS : public scfImplementation1<csZipFS, iFileSystem>
{
  // let factory access the constructor
  friend class csZipFSHandler;

  // underlying archive of current filesystem
  csRef<ZipArchive> archive;
  // full path of current archive
  csString archivePath;
  // root path of filesystem, relative to archive root
  csString root;
  // last error status
  int lastError;

  // constructor
  csZipFS (ZipArchive *archive,
           const char *archivePath,
           const char *rootPath);

public:
  /* iFileSystem methods */
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

  virtual bool ChRoot (const char *newRoot, bool mustExist = false);

  virtual bool Flush ();

  /* Local public helpers */
  void GetFullPath (csString &dest, const char *suffix);
  size_t ReadFile (const char *path, size_t offset, 
                   char *data, size_t dataSize);
  size_t WriteFile (const char *path, size_t offset,
                    const char *data, size_t dataSize);
  // returns whole content of file (buffered), with null-terminator
  template <class Allocator>
  csRef<iDataBuffer> GetFileContents (const char *path,
                                      bool fileMustExist,
                                      size_t &oSize,
                                      Allocator &alloc)
  {
    // template function implemented on header
    if (path [strlen (path) - 1] == '/')
    {
      // this won't work with folders
      return csRef<iDataBuffer> ();
    }
    // construct full path from archive root
    // (cache is shared among filesystems sharing single archive)
    csString fullPath (root);
    csVfsPathHelper::AppendPath (fullPath, path);
    // acquire lock on archive
    CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
    // perform cache lookup
    size_t index = archive->cacheLookup.Get (fullPath,
                                             csArrayItemNotFound);
    if (index != csArrayItemNotFound)
    {
      // cache found; return it
      oSize = archive->cacheSize.Get (index);
      return csRef<iDataBuffer> (archive->cache.Get (index));
    }
    // cache was not found; construct it
    csRef<iDataBuffer> buffer;
    size_t size = 0;
    if (GetEntry (path) == nullptr)
    {
      if (fileMustExist)
        return buffer; // noexistent file; halt operation

      // create nonexistent file
      buffer = csPtr<iDataBuffer> (
        new CS::DataBuffer<Allocator> (DEFAULT_BUFFER_SIZE, alloc));
      // zero-fill, in order to mimic POSIX fseek/fread behavior
      memset (**buffer, 0, DEFAULT_BUFFER_SIZE);
    }
    else
    {
      csRef<iDataBuffer> buffer = archive->Read (fullPath, alloc);
      size = buffer->GetSize ();
    }

    // insert into structure
    index = archive->cache.Push (buffer);
    archive->cacheFlags[index] = CACHE_OK; // cache is fresh
    archive->cacheSize[index] = size;
    archive->cacheLookup.Put (fullPath, index);
    // return by reference
    oSize = size;
    return buffer;
  }

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
  // Full path of this file
  csString fullPath;
  // path of this file, relative to root of parent FS
  const char *path;
  // File pointer
  size_t pos;
  // File opening mode
  int mode;
  // constructor
  csZipArchiveFile (csZipFS *parent, const char *path,
                    const char *pathPrefix, int mode);

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
  virtual csPtr<iFileSystem> Create (const char *realPath, int &oStatus);
  // iArchiveHandler methods
  virtual csPtr<iFileSystem> GetFileSystem (iFileSystem *parentFS,
                                            const char *parentFSPath,
                                            const char *archivePath);
};

}
CS_PLUGIN_NAMESPACE_END (VFS_ARCHIVE_ZIP)

#endif // !__CS_VFS_ZIP_H__
