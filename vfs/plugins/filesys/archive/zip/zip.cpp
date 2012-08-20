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

#include "cssysdef.h"
#include "csutil/archive.h"
#include "csutil/refcount.h"
#include "csutil/refarr.h"
#include "csutil/scfstringarray.h"
#include "csutil/parasiticdatabuffer.h"
#include "cstool/vfspartialview.h"
#include "cstool/vfspathhelper.h"

#include "zip.h"

CS_PLUGIN_NAMESPACE_BEGIN (VFS_ARCHIVE_ZIP)
{

using CS::Memory::iAllocator;

class ArchiveCache;

// Archive cache object
ArchiveCache *archiveCache = nullptr;

class csZipArchiveFile::View :
  public scfImplementationExt0<View, csVfsPartialView>
{
  friend class csZipArchiveFile;

private:
  View (csZipArchiveFile *parent, size_t offset, size_t size) :
    scfImplementationType (this, parent, offset, size)
  {
  }

public:
  virtual ~View () { }

  virtual const char *GetName () { return "#csZipArchiveFile::View"; }
};

class ArchiveCache : public CS::Memory::CustomAllocated
{
private:
  // list of archives
  csRefArray<ZipArchive> archives;
  // mutex
  CS::Threading::ReadWriteMutex mutex;
public:
  ArchiveCache ()
  {
  }

  // Get archive of given identifier from cache, instantiating if necessary
  //   identifier - fully-qualified identifier of current archive
  //   path       - path of archive file within fs
  //   fs         - (optional) parent filesystem to find archive file
  // - Remarks: if fs is nullptr (or not supplied), path is assumed to be
  //            native path
  csPtr<ZipArchive> GetArchive (const char *identifier,
                                const char *path,
                                iFileSystem *fs = nullptr)
  {
    // Create comparator for lookup
    csArrayCmp<ZipArchive *, const char *> compare (identifier, CompareKey);
    // acquire upgradeable lock before access
    CS::Threading::ScopedUpgradeableLock lock (mutex);
    // perform lookup
    size_t pos = archives.FindKey (compare);
    csRef<ZipArchive> archive;
    if (pos != csArrayItemNotFound)
    {
      // element found; return that element.
      // it is stored in csRef first, so refcount gets properly updated
      archive = archives.Get (pos);
      return csPtr<ZipArchive> (archive);
    }

    // not found in cache... acquire write lock
    mutex.UpgradeUnlockAndWriteLock ();
    // try lookup again, as another thread might already have created one 
    // while we were waiting for write lock
    pos = archives.FindKey (compare);
    if (pos != csArrayItemNotFound)
    {
      // element found; return that element.
      // it is stored in csRef first, so refcount gets properly updated
      archive = archives.Get (pos);
    }
    else
    {
      // element was not found... proceed with instantiation
      archive = csPtr<ZipArchive> (new ZipArchive (identifier, path, fs));
      // insert new instance into list
      archives.Push (archive);
    }
    // unlock
    mutex.WriteUnlock ();
    return csPtr<ZipArchive> (archive);
  }

  // comparator function for csArrayCmp
  static int CompareKey (ZipArchive * const &archive,
                         const char * const &key)
  {
    return archive->identifier.Compare (key);
  }
};

// --- ZipArchive -------------------------------------------------------- //

ZipArchive::ZipArchive (const char *identifier,
                        const char *path,
                        iFileSystem *parentFS/*= nullptr*/)
 : csArchive (path), parent (parentFS), identifier (identifier)
{
}

csPtr<iFile> ZipArchive::OpenBaseFile (int mode)
{
  if (!parent)
    return csArchive::OpenBaseFile (mode); // fallback to old mode
  // open file handle from parent fs
  return csPtr<iFile> (parent->Open (GetName (), "", mode, false));
}

bool ZipArchive::Flush ()
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  // flush cache
  FlushCache ();

  // archive-level flush
  bool result = csArchive::Flush ();

  if (parent) // flush parent filesystem
    result &= parent->Flush ();

  return result;
}

void ZipArchive::FlushCache ()
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  typedef csHash<size_t, csString>::GlobalIterator Iterator;
  Iterator it = cacheLookup.GetIterator ();
  // flush entire cache
  while (it.HasNext ())
  {
    // get key/value pair
    csString path;
    size_t index = it.Next (path);
    // fetch required data
    int flags = cacheFlags[index];
    if (flags & CACHE_UPDATED)
    {
      // there has been updates...
      csRef<iDataBuffer> buffer = cache.Get (index);
      size_t size = cacheSize[index];
      // flush contents
      void *entry = NewFile (path, size,
                            (flags & CACHE_PACK_ON_WRITE) != 0);
      Write (entry, **buffer, size);
    }
    // remove CACHE_UPDATED flag
    cacheFlags[index] &= ~CACHE_UPDATED;
  }
}

void ZipArchive::PurgeCache ()
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  
  // flush changes before purging
  FlushCache ();
  // at this point, all changes has been written
  // purge entire cache
  cacheLookup.DeleteAll ();
  cache.DeleteAll ();
  cacheFlags.DeleteAll ();
  cacheSize.DeleteAll ();
}

void ZipArchive::PurgeCache (const csString &path)
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  // purge cache of specific file
  size_t index = cacheLookup.Get (path, csArrayItemNotFound);
  if (index != csArrayItemNotFound)
  {
    int flags = cacheFlags[index];
    if (flags & CACHE_UPDATED)
    {
      // there has been updates...
      csRef<iDataBuffer> buffer = cache.Get (index);
      size_t size = cacheSize[index];
      // flush contents
      void *entry = NewFile (path, size,
                            (flags & CACHE_PACK_ON_WRITE) != 0);
      Write (entry, **buffer, size);
    }
    size_t lastIndex = cache.GetSize () - 1;
    // O(n) search to find the key
    csHash<size_t, csString>::GlobalIterator it;
    it = cacheLookup.GetIterator ();
    csString key;
    size_t pos = csArrayItemNotFound;
    while (it.HasNext ())
    {
      pos = it.Next (key);
      if (pos == lastIndex)
        // match found;
        break;
    }

    if (pos == lastIndex && index != lastIndex)
    {
      // overwrite index
      *cacheLookup[key] = index;
      // overwrite with last item
      cache[index] = cache[lastIndex];
      cacheFlags[index] = cacheFlags[lastIndex];
      cacheSize[index] = cacheSize[lastIndex];
    }
    // Remove old key from lookup table
    cacheLookup.DeleteAll (path);
    cache.DeleteIndex (lastIndex);
    cacheFlags.DeleteIndex (lastIndex);
    cacheSize.DeleteIndex (lastIndex);
  }
}

// Get archive identifier
void ZipArchive::GetIdentifier (csString &oIdentifier)
{
  oIdentifier = identifier;
}

// --- csZipArchiveFile -------------------------------------------------- //

// constructor
csZipArchiveFile::csZipArchiveFile (csZipFS *parent,
                                    const char *path,
                                    const char *pathPrefix,
                                    int mode) :
  scfImplementationType (this),
  archive (parent),
  fullPath (pathPrefix), // put prefix part first
  pos (0),
  mode (mode)
{
  size_t prefixLength = fullPath.Length ();
  // append suffix portion
  csVfsPathHelper::AppendPath (fullPath, path);
  // setup path variable
  this->path = ((const char *)fullPath) + prefixLength;
  if (*this->path == VFS_PATH_SEPARATOR)
    ++this->path;
}

// destructor
csZipArchiveFile::~csZipArchiveFile ()
{
}

// query filename
const char *csZipArchiveFile::GetName ()
{
  return fullPath;
}

// get file size
uint64_t csZipArchiveFile::GetSize ()
{
  uint64_t size;
  if (archive->GetSize (path, size))
    return size;
  lastError = archive->GetStatus ();
  return 0;
}

size_t csZipArchiveFile::Read (char *data, size_t dataSize)
{
  if (mode != VFS_FILE_READ)
    return 0; // cannot read file
  // lock on current file (to protect concurrent access from view)
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  size_t read = archive->ReadFile (path, pos, data, dataSize);
  pos += read;
  return read;
}

size_t csZipArchiveFile::Write (const char *data, size_t dataSize)
{
  if (mode == VFS_FILE_READ)
    return 0; // cannot write read-only file

  // lock on current file (to protect concurrent access from view)
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  // if append mode, you can only write past end of file
  size_t size = GetSize ();
  if (mode == VFS_FILE_APPEND && pos < size)
  {
    pos = size;
  }
  size_t written = archive->WriteFile (path, pos, data, dataSize);
  pos += written;
  return written;
}

void csZipArchiveFile::Flush ()
{
  archive->Flush ();
}

bool csZipArchiveFile::AtEOF ()
{
  return (pos >= GetSize ());
}

uint64_t csZipArchiveFile::GetPos ()
{
  return pos;
}

bool csZipArchiveFile::SetPos (off64_t newPos, int relativeTo)
{
  // is newPos negative (backwards)
  bool negative = newPos < 0;
  // take absolute value
  uint64_t distance = negative ? -newPos : newPos;
  // get file size
  size_t size = GetSize ();
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
      // absolute mode requested
      pos = ((uint64_t)newPos > size) ? size : newPos;
      break;
    default:
      return false;
  }

  return true;
}

csPtr<iDataBuffer> csZipArchiveFile::GetAllData (bool nullterm)
{
  // can we read this file?
  if (mode != VFS_FILE_READ)
    return csPtr<iDataBuffer> (nullptr); // operation failed

  CS::Memory::AllocatorMalloc alloc;
  size_t size;
  csRef<iDataBuffer> buffer = archive->GetFileContents (path, true,
                                                        size, alloc);

  if (!buffer.IsValid ())
    return csPtr<iDataBuffer> (nullptr); // operation failed

  if (size < buffer->GetSize ())
  {
    // buffer is bigger than actual file size
    return new csParasiticDataBuffer (buffer, 0, nullterm ? size + 1
                                                          : size);
  }

  // buffer is equal to actual file size
  if (nullterm)
  {
    // null-terminator is required.
    return csPtr<iDataBuffer> (new CS::DataBuffer<> (buffer, true));
  }

  return csPtr<iDataBuffer> (buffer);
}


csPtr<iDataBuffer> csZipArchiveFile::GetAllData (iAllocator *_alloc)
{
  // can we read this file?
  if (mode != VFS_FILE_READ)
    return csPtr<iDataBuffer> (nullptr); // operation failed

  CS::Memory::AllocatorInterface alloc (_alloc);
  size_t size = 0;
  csRef<iDataBuffer> buffer = archive->GetFileContents (path, true,
                                                        size, alloc);
  if (!buffer.IsValid ())
    return csPtr<iDataBuffer> (nullptr); // operation failed

  if (size < buffer->GetSize ())
  {
    // buffer is bigger than actual file size
    return new csParasiticDataBuffer (buffer, 0, size);
  }

  // buffer is equal to actual file size
  return csPtr<iDataBuffer> (buffer);
}

// Get partial view of current file
csPtr<iFile> csZipArchiveFile::GetPartialView (uint64_t offset,
                                               uint64_t size)
{
  return csPtr<iFile> (new View (this, offset, size));
}

// --- csZipFS------- ---------------------------------------------------- //

csZipFS::csZipFS (ZipArchive *archive,
                  const char *archivePath,
                  const char *suffix) :
  scfImplementationType (this),
  archive (csPtr<ZipArchive> (archive)),
  archivePath (archivePath),
  root (suffix),
  lastError (VFS_STATUS_OK)
{
}

csZipFS::~csZipFS ()
{
  Flush ();
  archive.Invalidate ();
}

// open file
csPtr<iFile> csZipFS::Open (const char *path, // relative vfs path
                            const char *pathPrefix, // vfs path prefix
                            int mode,
                            const csVfsOptionList &options)
{
  // setup parameters
  csZipArchiveFile *file
    = new csZipArchiveFile (this, path, pathPrefix, mode);

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

bool csZipFS::Move (const char *oldPath, const char *newPath)
{
  // sorry, this feature is not here yet
  lastError = VFS_STATUS_NOTIMPLEMENTED;
  return false;
}

bool csZipFS::GetPermission (const char *filename, csFilePermission &oPerm)
{
  // operation not supported (zip archive does not support permissions)
  lastError = VFS_STATUS_UNSUPPORTED;
  return false;
}

bool csZipFS::SetPermission (const char *filename,
                             const csFilePermission &iPerm)
{
  // operation not supported (zip archive does not support permissions)
  lastError = VFS_STATUS_UNSUPPORTED;
  return false;
}

void *csZipFS::GetEntry (const char *vfsPath)
{
  csString path (root);
  csVfsPathHelper::AppendPath (path, vfsPath);
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
  return archive->FindName (path);
}

bool csZipFS::GetTime (const char *filename, csFileTime &oTime)
{
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
  // try finding given name from archive
  void *handle = GetEntry (filename);
  if (!handle)
    return false;

  // query file time
  archive->GetFileTime (handle, oTime);

  return true;
}

bool csZipFS::SetTime (const char *filename, const csFileTime &iTime)
{
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
  // try finding given name from archive
  void *handle = GetEntry (filename);
  if (!handle)
    return false;

  // set file time
  archive->SetFileTime (handle, iTime);

  return true;
}

bool csZipFS::GetSize (const char *filename, uint64_t &oSize)
{
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
  // try finding given name from archive
  void *handle = GetEntry (filename);
  if (!handle)
    return false;

  // get file size
  oSize = archive->GetFileSize (handle);

  return true;
}

bool csZipFS::Exists (const char *filename)
{
  // true if entry handle is nonzero
  return GetEntry (filename) != 0;
}

bool csZipFS::Delete (const char *filename)
{
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);

  csString path (root);
  csVfsPathHelper::AppendPath (path, filename);
  
  return archive->DeleteFile (path);
}

csPtr<iStringArray> csZipFS::List (const char *path)
{
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);

  size_t index = 0;
  void *handle;

  csString basePath (root);
  csVfsPathHelper::AppendPath (basePath, path);
  if (basePath[basePath.Length () - 1] != VFS_PATH_SEPARATOR)
  {
    csVfsPathHelper::AppendPath (basePath, "");
  }

  iStringArray *list = new scfStringArray;

  // use GetFile to iterate
  while ((handle = archive->GetFile (index++)) != 0)
  {
    // before adding each filename to the list,
    // check the following:
    // 1. base path matches given path
    // 2. filename is not base path itself
    // 3. filename must not go one extra level deeper
    csString filename = archive->GetFileName (handle);

    if (!filename.StartsWith (basePath))
      continue; // skip if this file is not within base path

    if (filename.Length () == basePath.Length ())
      continue; // this is base path itself; skip

    // see whether the path doesn't go extra level deeper
    // (either no further slashes, or having slash as the last character)
    size_t pos = filename.FindFirst ('/', basePath.Length ());
    if (pos == (size_t)-1 || pos == filename.Length () - 1)
    {
      // insert it without base path portion
      list->Push ((const char *)filename + basePath.Length ());
    }
  }
  return csPtr<iStringArray> (list);
}

csString csZipFS::GetRootRealPath ()
{
  return csVfsPathHelper::ComposePath (archivePath, root);
}

int csZipFS::GetStatus ()
{
  // reset current error status, then return old error
  int error = lastError;
  lastError = VFS_STATUS_OK;
  return error;
}

bool csZipFS::IsDir (const char *path)
{
  // normalize path so it ends with slash
  // (directory names always end with slash in zip archive)
  csString pathNormalized (path);
  if (path[strlen (path) - 1] == '/')
  {
    csVfsPathHelper::AppendPath (pathNormalized, "");
  }

  // try getting entry handle
  if (GetEntry (path) != 0) // if entry exists
    return true;

  return false;
}

bool csZipFS::ChRoot (const char *newRoot, bool mustExist/*= false*/)
{
  if (mustExist && !IsDir (newRoot))
  {
    // given directory doesn't exist
    return false;
  }

  // append newRoot
  csVfsPathHelper::AppendPath (root, newRoot);

  return true;
}

size_t csZipFS::ReadFile (const char *path, size_t offset, 
                          char *data, size_t dataSize)
{
  // acquire lock on archive
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
  CS::Memory::AllocatorMalloc alloc;
  // get file contents (from cache)
  size_t size;
  csRef<iDataBuffer> buffer = GetFileContents (path, true,
                                               size, alloc);
  if (!buffer.IsValid ())
    return 0; // invalid buffer

  // set bounds of parameters
  if (offset > size)
    offset = size;
  if (offset + dataSize > size)
    dataSize = size - offset; // works: offset <= size

  // copy data
  memcpy (data, **buffer + offset, dataSize);

  return dataSize;
}

size_t csZipFS::WriteFile (const char *path, size_t offset,
                           const char *data, size_t dataSize)
{
  // acquire lock on archive
  CS::Threading::RecursiveMutexScopedLock lock (archive->mutex);
  CS::Memory::AllocatorMalloc alloc;
  // get file contents (from cache)
  size_t size;
  csRef<iDataBuffer> buffer = GetFileContents (path, false,
                                               size, alloc);
  if (!buffer.IsValid ())
    return 0; // invalid buffer

  // actual size of buffer
  size_t bufferSize = buffer->GetSize ();
  // minimum required buffer size for this operation
  size_t requiredSize = offset + dataSize;

  // get cache index
  size_t index = archive->cacheLookup.Get (path, csArrayItemNotFound);
  CS_ASSERT (index != csArrayItemNotFound);
  if (bufferSize < requiredSize)
  {
    // buffer needs resizing
    while (bufferSize < requiredSize)
      bufferSize += DEFAULT_BUFFER_SIZE; // increase by default size
    iDataBuffer *newBuffer = new CS::DataBuffer<> (bufferSize);
    // copy old data
    memcpy (**newBuffer, **buffer, size);
    // zero-fill the rest
    memset (**newBuffer + size, 0, bufferSize - size);
    buffer.AttachNew (newBuffer);
    // insert into cache structure
    archive->cache.Put (index, buffer);
  }
  // if there are referenced ones, create a new copy before writing
  // GetRefCount() is not 100% reliable, but guaranteed to work here as
  // refcount must be at least 2 (cache and local);
  // in the worst scenario, it will only result in unnecessary
  // buffer duplication.
  else if (buffer->GetRefCount () > 2)
  {
    // 1. no resizing is required
    // 2. there might be more references outside here
    // make a copy
    buffer.AttachNew (new CS::DataBuffer<> (buffer, false));
    // insert into cache
    archive->cache.Put (index, buffer);
  }
  // at this point, we have buffer of
  // 1. sufficient size
  // 2. safe to work with
  // proceed to writing
  memcpy (**buffer + offset, data, dataSize);

  // set CACHE_UPDATED flag (needs to be flushed)
  archive->cacheFlags[index] |= CACHE_UPDATED;
  // update cache size if necessary
  if (requiredSize > size)
    archive->cacheSize[index] = requiredSize;

  // return # of bytes written
  return dataSize;
}

bool csZipFS::Flush ()
{
  // flush archive
  archive->Flush ();
  return true;
}

// --- csZipFSHandler ---------------------------------------------------- //

SCF_IMPLEMENT_FACTORY (csZipFSHandler)

csZipFSHandler::csZipFSHandler (iBase *parent) :
  scfImplementationType (this, parent) 
{
  archiveCache = new ArchiveCache;
}

csZipFSHandler::~csZipFSHandler ()
{
  // perform any cleanup
  ArchiveCache *temp = archiveCache;
  archiveCache = nullptr;
  delete temp;
}

bool csZipFSHandler::Initialize (iObjectRegistry *objRegistry)
{
  // currently object registry is not required
  return true;
}

// iFileSystemFactory::Create()
// Try creating filesystem from given parameters
csPtr<iFileSystem> csZipFSHandler::Create (const char *realPath, int &oStatus)
{
  if (!realPath)
  {
    // invalid argument
    oStatus = VFS_STATUS_INVALIDARGS;
    return csPtr<iFileSystem> (nullptr);
  }

  // this results in using old-fashioned csArchive way
  csString identifier;

  // if there is no protocol identifier, add it
  if (strstr (realPath, "://") == nullptr)
    identifier << "zip://" << realPath;
  else
    identifier << realPath;

  csRef<ZipArchive> archive =
    archiveCache->GetArchive (identifier, realPath);
  iFileSystem *fs = new csZipFS (archive, realPath, "/");

  if (!fs) // reason of failure: unknown...
    oStatus = VFS_STATUS_OTHER;
  else // use status from filesystem
    oStatus = fs->GetStatus ();

  // stop the whole process if operation fails
  if (oStatus != VFS_STATUS_OK)
  {
    delete fs;
    fs = nullptr;
  }

  return csPtr<iFileSystem> (fs);
}

// iArchiveHandler::GetFileSystem()
// Try getting archive filesystem from given iFileSystem and path
csPtr<iFileSystem> csZipFSHandler::GetFileSystem (iFileSystem *parentFS,
                                                  const char *parentFSPath,
                                                  const char *archivePath)
{
  // prepare identifier first
  csString identifier (parentFSPath);
  csVfsPathHelper::AppendPath (identifier, archivePath);
  // create csArchive from filesystem/path
  csRef<ZipArchive> archive = archiveCache->GetArchive (identifier,
                                                        archivePath,
                                                        parentFS);
  iFileSystem *fs = new csZipFS (archive, identifier, "/");

  if (fs && fs->GetStatus () != VFS_STATUS_OK)
  {
    delete fs;
    fs = nullptr;
  }

  return csPtr<iFileSystem> (fs);
}

}
CS_PLUGIN_NAMESPACE_END (VFS_ARCHIVE_ZIP)
