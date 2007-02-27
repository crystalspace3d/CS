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

#include "cssysdef.h"
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "filesystem.h"
#include "csutil/databuf.h"
#include "csutil/scfstringarray.h"
#include "iutil/objreg.h"
#include "csutil/sysfunc.h"

CS_PLUGIN_NAMESPACE_BEGIN(vfs)
{
// ----------------------------------------------------- csFile ---------- //
csFileSystem::csFile::csFile (const char *Name) :
  scfImplementationType(this, 0)
{
  this->Name = new char[strlen(Name) +1];
  strcpy(this->Name, Name);
  this->Name[strlen(Name)] = 0;

  Size = 0;
  Error = VFS_STATUS_OK;
}

csFileSystem::csFile::~csFile ()
{
  delete [] Name;
}

int csFileSystem::csFile::GetStatus ()
{
  int rc = Error;
  Error = VFS_STATUS_OK;
  return rc;
}
// ------------------------------------------------------ csFileSystem --- //
csFileSystem::csFileSystem(iBase *iParent) :
  scfImplementationType(this, iParent),
  object_reg(0)
{
 
}

csFileSystem::~csFileSystem ()
{
 
}

bool csFileSystem::Initialize (iObjectRegistry* r)
{
  object_reg = r;
  return true;
}

csString csFileSystem::ExtractParentName(const char *path) const
{
  csString strPath = path;
  csString strFileName;

    // Remove trailing slash
  if (strPath.GetAt(strPath.Length() - 1) == CS_PATH_SEPARATOR)
    strPath.Truncate(strPath.Length() - 1);

  size_t newlen = strPath.FindLast(CS_PATH_SEPARATOR);


  strPath.SubString(strFileName, 0, newlen);
  return strFileName;
}

csString csFileSystem::ExtractFileName(const char *path) const
{
  csString strPath = path;
  csString strFileName;
   
  // Remove trailing slash
  if (strPath.GetAt(strPath.Length() - 1) == CS_PATH_SEPARATOR)
    strPath.Truncate(strPath.Length() - 1);

  size_t newlen = strPath.FindLast(CS_PATH_SEPARATOR);

  strPath.SubString(strFileName, newlen+1);
  return strFileName;
}

// ------------------------------------------------ csNativeFileSystem --- //

SCF_IMPLEMENT_FACTORY (csNativeFileSystem)

csNativeFileSystem::csNativeFileSystem(iBase *iParent) :
	scfImplementationType(this, iParent)
{

}

csNativeFileSystem::~csNativeFileSystem()
{

}

iFile* csNativeFileSystem::Open(const char * FileName, int mode)
{
  csPhysicalFile* file = 0;

  if ((mode & VFS_FILE_MODE) == VFS_FILE_READ)
  {
     file = new csPhysicalFile(FileName, "rb");
  }
  else if ((mode & VFS_FILE_MODE) == VFS_FILE_WRITE)
  {
      file = new csPhysicalFile(FileName, "wb");
      if (file->GetStatus() != VFS_STATUS_OK)
      {
        delete file;
        if (MakeDirectory(ExtractParentName(FileName)))
        {
          file = new csPhysicalFile(FileName, "wb");
        }
      }
  }
  else if ((mode & VFS_FILE_MODE) == VFS_FILE_APPEND)
  {
      file = new csPhysicalFile(FileName, "ab");
      if (file->GetStatus() != VFS_STATUS_OK)
      {
        delete file;
        if (MakeDirectory(ExtractParentName(FileName)))
        {
          file = new csPhysicalFile(FileName, "ab");
        }
      }
  }

  if (!file)
    return 0;

  if (file->GetStatus() != VFS_STATUS_OK)
  {
    delete file;
    return 0;
  }


	return file;
}

bool csNativeFileSystem::MakeDirectory(const char *DirName)
{
  if (!DirName)
    return false;

  csString NewDirectory;
  csStringArray DirectoryTree;
  int result = 0;

  // Split the path into individual directories
  static char path_separator [] = {CS_PATH_SEPARATOR, 0};
  DirectoryTree.SplitString(DirName, path_separator);

  if (DirectoryTree.GetSize() == 0)
    return false;

  // Check if the path start with the separator
  if (DirName[0] != CS_PATH_SEPARATOR)
  {
    NewDirectory.Append(DirectoryTree[0]);
  }

  // Go through all the directories
  for (size_t i = 1; i < DirectoryTree.GetSize(); i++)
  {
    // Make sure it is not null
    if (!DirectoryTree[i])
        continue;

    // Add the path separator
    NewDirectory.Append(CS_PATH_SEPARATOR);

    // Add the directory name
    NewDirectory.Append(DirectoryTree[i]);

    // Try and create the directory
    result = CS_MKDIR(NewDirectory);
  }

  // Return the result
  return (result == 0);
}

bool csNativeFileSystem::Delete(const char * FileName)
{
  return (unlink (FileName) == 0);
}

#ifndef _S_IFDIR
#define _S_IFDIR S_IFDIR
#endif

csVFSFileKind csNativeFileSystem::Exists(const char * FileName)
{
  struct stat stats;
  if (stat (FileName, &stats) != 0)
    return fkDoesNotExist;


  if ((stats.st_mode & _S_IFDIR) != 0)
    return fkFile;
  else
    return fkDirectory;
}

bool csNativeFileSystem::CanHandleMount(const char *FileName)
{
  struct stat stats;
  if (stat (FileName, &stats) != 0)
    return false;

  if ((stats.st_mode & _S_IFDIR) != 0)
    return false;

  return true;
}

void csNativeFileSystem::GetFilenames(const char *Path, const char *Mask,
                                      iStringArray *Names)
{
  csString vpath;
  DIR *dh;
  struct dirent *de;

  csString dpath = Path;

  // Make sure drive letter slash is correct on windows
#if defined (CS_PLATFORM_DOS) || defined (CS_PLATFORM_WIN32)
  if (strlen(Path) > 3)
    if ((Path[1] == ':') && (Path[2] != '\\'))
      dpath.Insert(2, '\\');
#endif

  dh = opendir ((const char *) dpath);

  if (dh == 0)
  {
    return;
  }
  while ((de = readdir (dh)) != 0)
  {
    if ((strcmp (de->d_name, ".") == 0) || (strcmp (de->d_name, "..") == 0))
          continue;
    if (!csGlobMatches (de->d_name, Mask))
          continue;

    if (Names->Find(de->d_name) == csArrayItemNotFound)
      Names->Push(de->d_name);
  }
  closedir (dh);
}

// Query file date/time.
bool csNativeFileSystem::GetFileTime (const char *FileName, 
                                      csFileTime &oTime) const
{
  struct stat st;

  if (stat (FileName, &st) != 0)
    return false;

  const time_t mtime = st.st_mtime;
  struct tm *curtm = localtime (&mtime);

  ASSIGN_FILETIME (oTime, *curtm);

  return true;
}
  
// Set file date/time.
bool csNativeFileSystem::SetFileTime (const char *FileName, 
                                      const csFileTime &iTime)
{
  // Cannot do this in platform independant way
  return true;
}

// Query file size (without opening it).
bool csNativeFileSystem::GetFileSize (const char *FileName, size_t &oSize)
{
  struct stat st;
  if (stat (FileName, &st) != 0)
      return false;
  oSize = st.st_size;
  return true;
}

// Sync
bool csNativeFileSystem::Sync ()
{
  return true;
}

// ----------------------------------------------- csArchiveFileSystem --- //
class VfsArchiveCache
{
private:
  csPDelArray<csArchiveFileSystem::VfsArchive> array;

public:
  VfsArchiveCache () : array (8, 8)
  {
  }
  virtual ~VfsArchiveCache ()
  {
    array.DeleteAll ();
  }

  /// Find a given archive file.
  size_t FindKey (const char* Key) const
  {
    size_t i;
    for (i = 0; i < array.GetSize (); i++)
      if (strcmp (array[i]->GetName (), Key) == 0)
        return i;
    return (size_t)-1;
  }

  csArchiveFileSystem::VfsArchive *Get (size_t iIndex)
  {
    return array.Get (iIndex);
  }

  size_t GetSize () const
  {
    return array.GetSize ();
  }

  void Push (csArchiveFileSystem::VfsArchive* ar)
  {
    array.Push (ar);
  }

  void DeleteAll ()
  {
    array.DeleteAll ();
  }

  void FlushAll ()
  {
    size_t i = 0;
    while (i < array.GetSize ())
    {
      array[i]->Flush ();
      if (array[i]->RefCount == 0)
      {
	array.DeleteIndex (i);
      }
      else
      {
	i++;
      }
    }
  }

  void CheckUp ()
  {
    size_t i = array.GetSize ();
    while (i > 0)
    {
      i--;
      csArchiveFileSystem::VfsArchive *a = array.Get (i);
      if (a->CheckUp ())
        array.DeleteIndex (i);
    }
  }
};

// The global archive cache
static VfsArchiveCache *ArchiveCache = 0;

// ----------------------------------------------- csArchiveFileSystem --- //
SCF_IMPLEMENT_FACTORY (csArchiveFileSystem)

csArchiveFileSystem::csArchiveFileSystem(iBase *iParent) :
	scfImplementationType (this, iParent)
{
  ArchiveCache = new VfsArchiveCache ();
}

csArchiveFileSystem::~csArchiveFileSystem()
{
  CS_ASSERT (ArchiveCache);
  delete ArchiveCache;
  ArchiveCache = 0;
}

csArchiveFileSystem::VfsArchive * csArchiveFileSystem::FindFile(
  const char *FileName) const
{
  csString archiveName = ExtractArchiveName(FileName);

  size_t idx = ArchiveCache->FindKey (archiveName);
  // archive not in cache?
  if (idx == csArrayItemNotFound)
  {
     // does file rpath exist?
     if (access (archiveName, F_OK) != 0)
       return 0;

     idx = ArchiveCache->GetSize ();
     ArchiveCache->Push (new VfsArchive (archiveName));
  }

  VfsArchive *a = ArchiveCache->Get (idx);

  a->UpdateTime ();

  if (a->FileExists (ExtractArchiveFileName(FileName), 0))
  {
    ArchiveCache->CheckUp ();
    return a;
  }

  ArchiveCache->CheckUp ();
	return 0;
}

iFile* csArchiveFileSystem::Open(const char * FileName, int mode)
{
  csString strPath = ExtractArchiveName(FileName);

  // rpath is an archive
  size_t idx = ArchiveCache->FindKey(strPath);
  
  // archive not in cache?
  if (idx == csArrayItemNotFound)
  {
    if ((mode & VFS_FILE_MODE) != VFS_FILE_WRITE)
	  {
        // does file rpath exist?
	     if (access (strPath, F_OK) != 0)
            return 0;
	  }

    idx = ArchiveCache->GetSize ();
    ArchiveCache->Push (new VfsArchive (strPath));
  }

  csFile *f = new csArchiveFile(mode, ExtractArchiveFileName(FileName), ArchiveCache->Get(idx));
  if (f->GetStatus () != VFS_STATUS_OK)
  {
    delete f;
	  f = 0;
  }

  ArchiveCache->CheckUp ();
	return f;
}

bool csArchiveFileSystem::Delete(const char * FileName)
{
  csString strPath = FileName;

  VfsArchive *a = FindFile(strPath);

  if (a)
  {
    return a->DeleteFile(ExtractArchiveFileName(FileName));
  }

  return false;
}

csVFSFileKind csArchiveFileSystem::Exists(const char * FileName)
{
  if (FindFile(FileName))
  {
    return fkArchiveFile;
  }

	return fkDoesNotExist;
}

bool csArchiveFileSystem::CanHandleMount(const char *FileName)
{
  FILE* f = fopen (FileName, "rb");
  if (!f) return false;

  char header[4];
  bool ret = ((fread (header, sizeof(header), 1, f) == 1)
    && (header[0] == 'P') && (header[1] == 'K')
    && (header[2] ==   3) && (header[3] ==   4));
  fclose (f);

  return ret;
}

void csArchiveFileSystem::GetFilenames(const char *Path, const char *Mask, 
                                       iStringArray *Names)
{
  csString strPath = Path;

  size_t idx = ArchiveCache->FindKey(strPath);
  
  // archive not in cache?
  if (idx == (size_t)-1)
  {
    // does file rpath exist?
    if (access ((const char *) strPath, F_OK) != 0)
      return;

    idx = ArchiveCache->GetSize ();
    ArchiveCache->Push (new VfsArchive (strPath));
  }

  VfsArchive *a = ArchiveCache->Get (idx);
  // Flush all pending operations
  a->UpdateTime ();
  if (a->Writing == 0)
     a->Flush ();
  void *iterator;
  int no = 0;

  while ((iterator = a->GetFile(no++)))
  {
    char *fname = a->GetFileName(iterator);
    if (csGlobMatches (fname, Mask))
    {
      if (Names->Find(fname) == csArrayItemNotFound)
        Names->Push(fname);
    }
  }

  ArchiveCache->CheckUp ();
}

// Query file date/time.
bool csArchiveFileSystem::GetFileTime (const char *FileName, 
                                       csFileTime &oTime) const
{
  csString strPath = FileName;
  VfsArchive *a = FindFile(strPath);
 
  if (!a)
    return false;

  void *e = a->FindName (ExtractArchiveFileName(strPath));
  if (!e)
      return false;
    
  a->GetFileTime(e, oTime);

  ArchiveCache->CheckUp ();
  return true;
}
  
// Set file date/time.
bool csArchiveFileSystem::SetFileTime (const char *FileName, 
                                       const csFileTime &iTime)
{
  csString strPath = FileName;

  VfsArchive *a = FindFile(strPath);
 
  if (!a)
    return false;

  
  void *e = a->FindName (ExtractArchiveFileName(strPath));
  if (!e)
      return false;
    
  a->SetFileTime(e, iTime);

  ArchiveCache->CheckUp ();
  return true;
}

// Query file size (without opening it).
bool csArchiveFileSystem::GetFileSize (const char *FileName, size_t &oSize)
{
  csString strPath = FileName;
  VfsArchive *a = FindFile(strPath);
 
  if (!a)
    return false;

  
  void *e = a->FindName (ExtractArchiveFileName(strPath));
  if (!e)
      return false;
    
  oSize = a->GetFileSize(e);

  ArchiveCache->CheckUp ();
  return true;
}

// Sync
bool csArchiveFileSystem::Sync ()
{
  ArchiveCache->FlushAll ();
  return true;
}

csString csArchiveFileSystem::ExtractArchiveName(const char *path) const
{
  csString result = path;

  static char path_sep [] = {CS_PATH_SEPARATOR, 0};

  size_t period = result.Find(".");
  size_t end = result.Find(path_sep, period);

  result.Truncate(end);

  return result;
}

csString csArchiveFileSystem::ExtractArchiveFileName(const char *path) const
{
  csString result = path;

  static char path_sep [] = {CS_PATH_SEPARATOR, 0};

  size_t period = result.Find(".");
  size_t end = result.Find(path_sep, period);

  return result.Slice(end + 1);
}

// ------------------------------------------------ VfsArchive------------ //
void csArchiveFileSystem::VfsArchive::UpdateTime ()
{
    LastUseTime = csGetTicks ();
}

void csArchiveFileSystem::VfsArchive::IncRef ()
{
    RefCount++;
    UpdateTime ();
}
 
void csArchiveFileSystem::VfsArchive::DecRef ()
{
  if (RefCount)
    RefCount--;
  UpdateTime ();
}

bool csArchiveFileSystem::VfsArchive::CheckUp ()
{
  return (RefCount == 0) &&
      (csGetTicks () - LastUseTime > VFS_KEEP_UNUSED_ARCHIVE_TIME);
}

csArchiveFileSystem::VfsArchive::VfsArchive (const char *filename) 
   : csArchive (filename)
{
  RefCount = 0;
  Writing = 0;
  VfsArchive::object_reg = object_reg;

  UpdateTime ();
}

csArchiveFileSystem::VfsArchive::~VfsArchive ()
{
  CS_ASSERT (RefCount == 0);
  Flush ();
}

// ----------------------------------------------------- csArchiveFile --- //

csArchiveFileSystem::csArchiveFile::csArchiveFile (int Mode, 
  const char *Name, VfsArchive *ParentArchive) :
  scfImplementationType(this, Name)
{
  Archive = ParentArchive;
  Error = VFS_STATUS_OTHER;
  Size = 0;
  FileHandle = 0;
  FileData = 0;
  fpos = 0;

  CS::Threading::RecursiveMutexScopedLock lock (Archive->archive_mutex);

  Archive->UpdateTime ();

  ArchiveCache->CheckUp ();

  if ((Mode & VFS_FILE_MODE) == VFS_FILE_READ)
  {
    // If reading a file, flush all pending operations
    if (Archive->Writing == 0)
      Archive->Flush ();
    if ((FileData = Archive->Read (Name, &Size)))
    {
      Error = VFS_STATUS_OK;
      DataBuffer = csPtr<iDataBuffer> (new csDataBuffer (FileData, Size));
    }
  }
  else if ((Mode & VFS_FILE_MODE) == VFS_FILE_WRITE)
  {
    if ((FileHandle = Archive->NewFile(Name,0,!(
      Mode & VFS_FILE_UNCOMPRESSED))))
    {
      Error = VFS_STATUS_OK;
      Archive->Writing++;
    }
  }
  Archive->IncRef ();
}

csArchiveFileSystem::csArchiveFile::~csArchiveFile ()
{
  CS::Threading::RecursiveMutexScopedLock lock (Archive->archive_mutex);
  if (FileHandle)
    Archive->Writing--;
  Archive->DecRef ();
}

size_t csArchiveFileSystem::csArchiveFile::Read (char *Data, size_t DataSize)
{
  if (FileData)
  {
    size_t sz = DataSize;
    if (fpos + sz > Size)
      sz = Size - fpos;
    memcpy (Data, FileData + fpos, sz);
    fpos += sz;
    return sz;
  }
  else
  {
    Error = VFS_STATUS_ACCESSDENIED;
    return 0;
  }
}

size_t csArchiveFileSystem::csArchiveFile::Write (const char *Data, 
                                                  size_t DataSize)
{
  if (FileData)
  {
    Error = VFS_STATUS_ACCESSDENIED;
    return 0;
  }
  CS::Threading::RecursiveMutexScopedLock lock (Archive->archive_mutex);
  if (!Archive->Write (FileHandle, Data, DataSize))
  {
    Error = VFS_STATUS_NOSPACE;
    return 0;
  }
  return DataSize;
}

void csArchiveFileSystem::csArchiveFile::Flush ()
{
  if (Archive)
  {
    CS::Threading::RecursiveMutexScopedLock lock (Archive->archive_mutex);
    Archive->Flush ();
  }
}

bool csArchiveFileSystem::csArchiveFile::AtEOF ()
{
  if (FileData)
    return fpos + 1 >= Size;
  else
    return true;
}

size_t csArchiveFileSystem::csArchiveFile::GetPos ()
{
  return fpos;
}

bool csArchiveFileSystem::csArchiveFile::SetPos (size_t newpos)
{
  if (FileData)
  {
    fpos = (newpos > Size) ? Size : newpos;
    return true;
  }
  else
  {
    return false;
  }
}

csPtr<iDataBuffer> csArchiveFileSystem::csArchiveFile::GetAllData(bool 
                                                                  nullterm)
{
  if (FileData)
  {
    return csPtr<iDataBuffer> (DataBuffer);
  }
  else
  {
    return 0;
  }
}

} CS_PLUGIN_NAMESPACE_END(vfs)
