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
#include "csutil/scfstringarray.h"
#include "cstool/vfspartialview.h"

#include "zip.h"

CS_PLUGIN_NAMESPACE_BEGIN (VFS_ARCHIVE_ZIP)
{

// Custom csArchive that uses iFileSystem-based file handling
class ZipArchive : public csArchive, public CS::Utility::AtomicRefCount
{
private:
  iFileSystem *parent;

protected:
  // overridden to support nested archives
  virtual csPtr<iFile> OpenBaseFile (int mode);

public:
  // constructor
  ZipArchive (const char *path, iFileSystem *parentFS = nullptr);
  // flush
  virtual bool Flush ();
};

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

// --- ZipArchive -------------------------------------------------------- //

ZipArchive::ZipArchive (const char *path, iFileSystem *parentFS/*= nullptr*/)
 : csArchive (path), parent (parentFS)
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
  // archive-level flush
  bool result = csArchive::Flush ();

  if (parent) // flush parent filesystem
    result &= parent->Flush ();

  return result;
}

// --- csZipArchiveFile -------------------------------------------------- //

// constructor
csZipArchiveFile::csZipArchiveFile (csZipFS *parent) :
  scfImplementationType (this),
  archive (parent)
{
}

// destructor
csZipArchiveFile::~csZipArchiveFile ()
{
}

// query filename
const char *csZipArchiveFile::GetName ()
{
}

uint64_t csZipArchiveFile::GetSize ()
{
}

size_t csZipArchiveFile::Read (char *data, size_t dataSize)
{
}

size_t csZipArchiveFile::Write (const char *data, size_t dataSize)
{
}

void csZipArchiveFile::Flush ()
{
}

bool csZipArchiveFile::AtEOF ()
{
}

uint64_t csZipArchiveFile::GetPos ()
{
}

bool csZipArchiveFile::SetPos (off64_t newPos, int relativeTo)
{
}

csPtr<iDataBuffer> csZipArchiveFile::GetAllData (bool nullterm)
{
}

csPtr<iDataBuffer> csZipArchiveFile::GetAllData (CS::Memory::iAllocator *alloc)
{
}

csPtr<iFile> csZipArchiveFile::GetPartialView (uint64_t offset,
                                               uint64_t size)
{
}

// --- csZipArchiveFile::View -------------------------------------------- //

// --- csZipFS------- ---------------------------------------------------- //

csZipFS::csZipFS (ZipArchive *archive, const char *suffix) :
  scfImplementationType (this),
  archive (csPtr<ZipArchive> (archive)),
  root (suffix),
  lastError (VFS_STATUS_OK)
{
}

csZipFS::~csZipFS ()
{
  Flush ();
  delete archive;
}

csPtr<iFile> csZipFS::Open (const char *path,
                            const char *pathPrefix,
                            int mode,
                            const csVfsOptionList &options)
{
  // @@@TODO: implement feature
  return csPtr<iFile> (nullptr);
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
  // @@@TODO: make that helper available here...
  // AppendVfsPath (path, vfsPath);
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  return archive->FindName (path);
}

bool csZipFS::GetTime (const char *filename, csFileTime &oTime)
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
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
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
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
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
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
  CS::Threading::RecursiveMutexScopedLock lock (mutex);

  csString path (root);
  // @@@TODO: make that helper available here...
  // AppendVfsPath (path, filename);
  
  return archive->DeleteFile (path);
}

csPtr<iStringArray> csZipFS::List (const char *path)
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);

  size_t index = 0;
  void *handle;

  csString basePath (root);
  // AppendVfsPath (basePath, path);
  if (basePath[basePath.Length () - 1] != VFS_PATH_SEPARATOR)
  {
    // AppendVfsPath (basePath, "");
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
  csString rp;

  return rp;
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
    // AppendVfsPath (pathNormalized, "");
  }

  // try getting entry handle
  if (GetEntry (path) != 0) // if entry exists
    return true;

  return false;
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
}

csZipFSHandler::~csZipFSHandler ()
{
  // perform any cleanup
}

bool csZipFSHandler::Initialize (iObjectRegistry *objRegistry)
{
  // currently object registry is not required
  return true;
}

// iFileSystemFactory::Create()
// Try creating filesystem from given parameters
iFileSystem *csZipFSHandler::Create (const char *realPath, int &oStatus)
{
  if (!realPath)
  {
    // invalid argument
    oStatus = VFS_STATUS_INVALIDARGS;
    return nullptr;
  }

  // this results in using old-fashioned csArchive
  ZipArchive *archive = new ZipArchive (realPath);
  iFileSystem *fs = new csZipFS (archive, "/");

  if (!fs) // reason of failure: unknown...
    oStatus = VFS_STATUS_OTHER;
  else // use status from filesystem
    oStatus = fs->GetStatus ();

  // stop the whole process if operation fails
  if (oStatus != VFS_STATUS_OK)
  {
    delete fs;
    return nullptr;
  }

  return fs;
}

// iArchiveHandler::GetFileSystem()
// Try getting archive filesystem from given iFileSystem and path
iFileSystem *csZipFSHandler::GetFileSystem (iFileSystem *parentFS,
                                            const char *archivePath,
                                            const char *suffix)
{
  // create csArchive from filesystem/path
  ZipArchive *archive = new ZipArchive (archivePath, parentFS);
  iFileSystem *fs = new csZipFS (archive, suffix);

  if (fs && fs->GetStatus () != VFS_STATUS_OK)
  {
    delete fs;
    return nullptr;
  }

  return fs;
}

}
CS_PLUGIN_NAMESPACE_END (VFS_ARCHIVE_ZIP)
