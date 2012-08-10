/*
    Crystal Space Virtual File System class
    Copyright (C) 1998,1999,2000 by Andrew Zabolotny <bit@eltech.ru>

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

//MSVC and MINGW define utime as _utime
//and utimbuf as _utimbuf. plus their headers
//are in a different place than unix like os.
#ifdef CS_PLATFORM_WIN32
#include <sys/utime.h>
#define utime(a,b) _utime(a,b)
#define utimbuf _utimbuf
#else
#include <utime.h>
#endif

#include "vfs.h"
#include "csgeom/math.h"
#include "csutil/archive.h"
#include "csutil/cmdline.h"
#include "csutil/csstring.h"
#include "csutil/databuf.h"
#include "csutil/memfile.h"
#include "csutil/mmapio.h"
#include "csutil/refarr.h"
#include "csutil/parray.h"
#include "csutil/parasiticdatabuffer.h"
#include "csutil/platformfile.h"
#include "csutil/scf_implementation.h"
#include "csutil/scfstringarray.h"
#include "csutil/stringquote.h"
#include "csutil/strset.h"
#include "csutil/sysfunc.h"
#include "csutil/syspath.h"
#include "csutil/util.h"
#include "csutil/vfsplat.h"
#include "csutil/fifo.h"
#include "iutil/databuff.h"
#include "iutil/objreg.h"
#include "iutil/verbositymanager.h"

#define NEW_CONFIG_SCANNING

// anonymous namespace; contains helpers local to this file
namespace
{
  // CS path separator ('/' or '\'), in string format
  const char CS_PATH_SEPARATOR_STRING  [] = { CS_PATH_SEPARATOR,  '\0' };
  // VFS path separator ('/'), in string format
  const char VFS_PATH_SEPARATOR_STRING [] = { VFS_PATH_SEPARATOR, '\0' };
  // VFS root path
  const char *VFS_ROOT_PATH = VFS_PATH_SEPARATOR_STRING;
  // whitespace characters (subject to trimming) in VFS
  const char *CS_VFSSPACE = " \t";

  // threshold in # of filesystems to use hashtable in mount/unmount process
  const size_t USE_HASHTABLE_THRESHOLD = 32;

  // Split a list of multiple paths delimited by VFS_PATH_DIVIDER.
  // pathList must be already expanded with ExpandVars().
  // the callee is responsible for freeing the returned pointer via cs_free().
  // Returns: NULL-terminated list of pointers to null-terminated C-string
  // Remarks: each string entries need not be freed separately.
  const char **SplitRealPath (const char *pathList);

  csString ComposeVfsPath (const char *base, const char *suffix);
  csString &AppendVfsPath (csString &base, const char *suffix);

  // Transform a path so that every \ or / is replaced with $/.
  // If 'add_end' is true there will also be a $/ at the end if there
  // is not already one there.
  // The result of this function must be deleted with cs_free().
  char *TransformPath (const char *path, bool add_end);

  char *AllocNormalizedPath (const char *s);
  bool LoadVfsConfig (csConfigFile& cfg, const char *dir,
                      csStringSet& seen, bool verbose);
} // end of anonymous namespace



CS_PLUGIN_NAMESPACE_BEGIN(VFS)
{

typedef csStringFast<CS_MAXPATHLEN> PathString;

//***********************************************************
// NOTE on naming convention: public classes begin with "cs"
// while private (local) classes do not.
//***********************************************************

// Private structure used to keep a "node" in virtual filesystem tree.
// The nodes are not refcounted objects. Any access on this structure
// must be protected by locks on parent VFS object.
class VfsNode : public CS::Memory::CustomAllocated
{
public:
  // Parent node
  VfsNode *parent;
  // Child nodes
  csArray<VfsNode *> children;
  // The virtual path
  char *vfsPath;
  // Configuration section key
  char *configKey;

  // number of static mounts
  size_t staticMounts;
  // The array of filesystems bound to this virtual path node
  csRefArray<iFileSystem> fileSystems;
  // The array of real paths that haven't been platform expanded
  // (e.g. Cygwin paths before they get expanded to Win32 paths)
  csStringArray realPaths;
  // Parent VFS object
  csVFS* vfs;
  // Verbosity flags.
  unsigned int verbosity;

  // Initialize the object
  VfsNode (const char *iPath, const char *iConfigKey, csVFS* vfs, 
	   unsigned int verbosity);
  // Destroy the object
  virtual ~VfsNode ();

  // Mount a single filesystem
  bool MountFileSystem (const char *realPath, iFileSystem *fs);
  // Mount a list of filesystems
  bool MountFileSystem (const char **realPath,
                        const csRefArray<iFileSystem> &fs);
  // Unmount a filesystem
  bool UnmountFileSystem (const char *realPath);
  // Unmount filesystems from a list of real-world path
  bool UnmountFileSystem (const char **realPath);
  // Find all files in a subpath
  void FindFiles (const char *suffix, const char *mask, iStringArray *FileList);
  // Find a file and return the appropiate csFile object
  iFile *Open (int mode, const char *suffix);
  // Delete a file
  bool Delete (const char *suffix);
  // Does file exists?
  bool Exists (const char *suffix);
  // Query date/time
  bool GetFileTime (const char *suffix, csFileTime &oTime);
  // Set date/time
  bool SetFileTime (const char *suffix, const csFileTime &iTime);
  // Query permission
  bool GetFilePermission (const char *suffix, csFilePermission &oPerm);
  // Set permission
  bool SetFilePermission (const char *suffix, const csFilePermission &iPerm);
  // Get file size
  bool GetFileSize (const char *suffix, uint64_t &oSize);
  // Query whether a given virtual path refers to a valid directory
  bool IsDir (const char *vfsPath);
  // Query whether current node has no children nor filesystems.
  bool IsEmpty ();
  // Return the list of mounted real path in a single string.
  // Each entry is delimited by VFS_PATH_DIVIDER plus an extra whitespace
  csString GetMountListString ();
private:
  // Find a file either on disk or in archive - in this node only
  // - takes suffix (rest of the path, deeper in the tree)
  //         rank   (0-based rank of file; denotes rank'th file)
  // returns smart pointer of belonging iFileSystem
  csRef<iFileSystem> FindFile (const char *suffix, size_t rank = 0);
  // Remount filesystem at given index.
  // index must be in range [staticMounts, fileSystems.GetSize ())
  void RemountFileSystem (size_t index);

  // Mutex on this node.
  CS::Threading::ReadWriteMutex mutex;
};

// ------------------------------------------------------------- VfsNode --- //

VfsNode::VfsNode (const char *iPath, const char *iConfigKey,
		  csVFS* vfs, unsigned int verbosity)
 : staticMounts (0), vfs (vfs)
{
  vfsPath = CS::StrDup (iPath);
  configKey = CS::StrDup (iConfigKey);
  VfsNode::verbosity = verbosity;
}

VfsNode::~VfsNode ()
{
  cs_free (configKey);
  cs_free (vfsPath);
}

// Re-mounts filesystem at given index, giving it highest priority.
// use this function only while the thread is write protected
void VfsNode::RemountFileSystem (size_t index)
{
  // basic assertions
  CS_ASSERT (index < fileSystems.GetSize ());
  CS_ASSERT (realPaths.GetSize () == fileSystems.GetSize ());
  CS_ASSERT (index < staticMounts);

  // store new entries from index
  fileSystems.Push (fileSystems.Get (index));
  realPaths.Push (realPaths.Get (index));
  // remove old entry
  fileSystems.DeleteIndex (index);
  realPaths.DeleteIndex (index);
}

bool VfsNode::MountFileSystem (const char *realPath, iFileSystem *fs)
{
  // realPath is assumed to be already expanded.

  if (!fs)
    return false;

  if (!realPath)
    realPath = fs->GetRootRealPath (); // fallback... shouldn't happen

  // no valid real path available
  if (!realPath)
    return false;

  // acquire upgradeable lock
  CS::Threading::ScopedUpgradeableLock lock (mutex);

  // assertions: both arrays must be in sync
  CS_ASSERT (fileSystems.GetSize () >= staticMounts);
  CS_ASSERT (fileSystems.GetSize () == realPaths.GetSize ());

  // check for existing entry
  for (size_t i = staticMounts; i < fileSystems.GetSize (); ++i)
  {
    if (strcmp (realPaths.Get (i), realPath) == 0)
    {
      // acquire write lock
      mutex.UpgradeUnlockAndWriteLock ();
      // duplicate entry found; reuse that filesystem
      // (new fs instance will be automatically discarded)
      RemountFileSystem (i);
      mutex.WriteUnlock ();
      return true;
    }
  }

  // acquire write lock, then insert
  mutex.UpgradeUnlockAndWriteLock ();
  realPaths.Push (realPath);
  fileSystems.Push (fs);
  mutex.WriteUnlock ();

  return true;
}

bool VfsNode::MountFileSystem (const char **realPath,
                               const csRefArray<iFileSystem> &fsList)
{
  // realPath is assumed to be already expanded.

  // acquire write lock
  CS::Threading::ScopedWriteLock lock (mutex);

  CS_ASSERT (fileSystems.GetSize () >= staticMounts);
  CS_ASSERT (fileSystems.GetSize () == realPaths.GetSize ());

  bool result = false;

  // if there are less than USE_HASHTABLE_THRESHOLD entries, do it by loop
  // otherwise, use hashtable
  if (fileSystems.GetSize () - staticMounts < USE_HASHTABLE_THRESHOLD)
  {
    for (size_t i = 0; i < fsList.GetSize (); ++i)
    {
      // get filesystem
      iFileSystem *fs = fsList.Get (i);
      // get reliable path
      csString rPath;
      // if realPath points to valid string, use it and increment the pointer
      if (realPath && *realPath)
      {
        // make sure string is not empty
        if (**realPath)
          rPath = *realPath;
        // increment it
        ++realPath;
      }
      else
        rPath = fs->GetRootRealPath ();

      // flag to use for skipping
      bool skip = false;
      // check for existing entries first
      for (size_t index = staticMounts; index < fileSystems.GetSize (); ++index)
      {
        if (strcmp (realPaths.Get (index), rPath) == 0)
        {
          // duplicate found; remount that filesystem
          RemountFileSystem (index);
          // done with this entry; set the flag
          skip = true;
        }
      }

      // if done with this entry; proceed to next one
      if (!skip)
      {
        // otherwise, insert
        realPaths.Push (rPath);
        fileSystems.Push (fsList.Get (i));
      }
      result = true;
    }
  }
  else
  {
    // use hashtable for faster lookup
    // 1. setup hashtable
    csHash<size_t, const char *> entries;
    for (size_t i = staticMounts; i < fileSystems.GetSize (); ++i)
      entries.Put (realPaths.Get (i), i);
    // 2. perform lookup
    for (size_t i = 0; i < fsList.GetSize (); ++i)
    {
      // get filesystem
      iFileSystem *fs = fsList.Get (i);
      // get reliable path
      csString rPath;
      // if realPath points to valid string, use it and increment the pointer
      if (realPath && *realPath)
      {
        // make sure string is not empty
        if (**realPath)
          rPath = *realPath;
        // increment it
        ++realPath;
      }
      else
        rPath = fs->GetRootRealPath ();
      // try finding the entry
      size_t pos = entries.Get (rPath, (size_t)-1);
      // was the entry found?
      if (pos != (size_t)-1)
      {
        // existing entry found. remount
        RemountFileSystem (pos);
        // remove from lookup table
        entries.DeleteAll (rPath);
      }
      else
      {
        // otherwise, insert as supplied
        realPaths.Push (rPath);
        fileSystems.Push (fs);
      }
      result = true;
    } // !for
  } // !else

  return result;
}

bool VfsNode::UnmountFileSystem (const char *realPath)
{
  // Remove all entries if realPath is NULL
  if (!realPath)
  {
    CS::Threading::ScopedWriteLock lock (mutex);
    // note that even after unmount, filesystem will stay alive until all
    // references are cleared
    realPaths.DeleteAll ();
    fileSystems.DeleteAll ();
    return true;
  }

  // realPath must be already expanded..

  {
    CS::Threading::ScopedUpgradeableLock lock (mutex);
    // iterate over existing filesystem paths
    for (size_t i = staticMounts; i < realPaths.GetSize (); ++i)
    {
      const char *fsPath = realPaths.Get (i);
      // TODO: fix potential issues
      if (strcmp (fsPath, realPath) == 0)
      {
        // upgrade to write lock
        mutex.UpgradeUnlockAndWriteLock ();
        // unmount filesystem
        fileSystems.DeleteIndex (i);
        // unlock
        mutex.WriteUnlock ();
        return true;
      }
    }
  }

  // requested filesystem entry is not found
  return false;
}

bool VfsNode::UnmountFileSystem (const char **realPath)
{
  // Remove all entries if realPath is NULL
  if (!realPath)
  {
    CS::Threading::ScopedWriteLock lock (mutex);
    // note that even after unmount, filesystem will stay alive until all
    // references are cleared
    realPaths.DeleteAll ();
    fileSystems.DeleteAll ();
    return true;
  }

  // if not null, realPath must be already expanded..
  // acquire upgradable lock
  CS::Threading::ScopedUpgradeableLock lock (mutex);

  CS_ASSERT (fileSystems.GetSize () >= staticMounts);
  CS_ASSERT (fileSystems.GetSize () == realPaths.GetSize ());

  bool result = false;

  // if there are less than USE_HASHTABLE_THRESHOLD entries, do it by loop
  // otherwise, use hashtable
  if (fileSystems.GetSize () - staticMounts < USE_HASHTABLE_THRESHOLD)
  {
    // loop until we reach end of the list (null pointer)
    while (*realPath)
    {
      for (size_t i = staticMounts; i < fileSystems.GetSize (); ++i)
      {
        // get path of existing entry for comparison
        const char *fsPath = realPaths.Get (i);
        // TODO: fix potential issues
        if (strcmp (fsPath, *realPath) == 0)
        {
          // upgrade to write lock
          mutex.UpgradeUnlockAndWriteLock ();
          // unmount filesystem
          fileSystems.DeleteIndex (i);
          // unlock
          mutex.WriteUnlockAndUpgradeLock ();
          result = true;
          break; // exit inner loop
        }
      }
      // proceed to next entry
      ++realPath;
    }
  }
  else
  {
    // use hashtable for faster lookup
    // 1. setup hashtable
    csHash<size_t, const char *> entries;
    for (size_t i = staticMounts; i < fileSystems.GetSize (); ++i)
      entries.Put (realPaths.Get (i), i);
    // 2. perform lookup
    // acquire write lock
    mutex.UpgradeUnlockAndWriteLock ();
    // loop until we reach end of the list (null pointer)
    while (*realPath)
    {
      size_t pos = entries.Get (*realPath, (size_t)-1);
      if (pos != (size_t)-1)
      {
        // entry found. remove from the actual list
        // no duplicates are allowed (except static ones)
        // so everything should be fine
        realPaths.DeleteIndex (pos);
        fileSystems.DeleteIndex (pos);
        // remove from lookup table
        entries.DeleteAll (*realPath);
        result = true;
      }
      ++realPath;
    } // !while
    mutex.WriteUnlock ();
  } // !else

  // requested filesystem entry was not found
  return result;
}

// returns a list of real paths mounted on current node as a single string
csString VfsNode::GetMountListString ()
{
  // delimiter : ", "
  static const char delimiter[] = { VFS_PATH_DIVIDER, ' ', '\0' };
  csString result;

  // acquire read lock
  CS::Threading::ScopedReadLock lock (mutex);

  size_t i, rpSize = realPaths.GetSize (),
            fsSize = fileSystems.GetSize ();

  // if real paths are stored, put them sequentially
  for (i = 0; i < rpSize; ++i)
  {
    result << realPaths.Get (i);
    result << delimiter;
  }

  for (; i < fsSize; ++i)
  {
    // in this section, it is assumed that rpSize < fsSize
    result << fileSystems.Get (i)->GetRootRealPath ();
    result << delimiter;
  }

  // truncate last delimiter
  result.Truncate (result.Length () - strlen (delimiter));
  return result;
}

void VfsNode::FindFiles (const char *suffix, const char *mask,
  iStringArray *fileList)
{

  size_t i;
  csString vpath; // temporary buffer
  CS::Threading::ScopedReadLock lock (mutex);

  // prepare base path
  vpath << vfsPath << suffix;
  const size_t baseLen = vpath.Length ();

  // this function must do the following steps:
  // 1. open filesystem
  // 2. get directory listing
  // 3. pattern match with given mask
  // 4. add the file to the list
  // 5. continue with the rest
  for (i = 0; i < fileSystems.GetSize (); ++i)
  {
    iFileSystem *fs = fileSystems.Get (i);
    // retrieve directory listing
    csRef<iStringArray> list = fs->List (suffix);
    if (!list.IsValid ())
      continue;

    size_t size = list->GetSize ();
    for (size_t i = 0; i < size; ++i)
    { 
      // clean the buffer
      vpath.Truncate (baseLen);
      vpath << list->Get (i);
      bool addSlash = false;
      size_t lastChar = vpath.Length () - 1;
      if (vpath[lastChar] == VFS_PATH_SEPARATOR)
      {
        addSlash = true;
        // remove slash, for now
        vpath.Truncate (lastChar);
      }
      // perform pattern matching; we ignore base portion of path
      if (!csGlobMatches (((const char *)vpath)+baseLen, mask))
        continue;

      // add slash for directories
      if (addSlash)
        vpath << VFS_PATH_SEPARATOR;

      // TODO: check for duplicates
      // insert
      fileList->Push (vpath);
    }
  }
}

bool VfsNode::IsDir (const char *vfsPath)
{
  // TODO: implement feature
  return true;
}

bool VfsNode::IsEmpty ()
{
  // at least read lock on parent object is assumed
  // acquire read lock
  CS::Threading::ScopedReadLock lock (mutex);
  // if there are no filesystems nor children, the node is empty;
  return fileSystems.IsEmpty () && children.IsEmpty ();
}

iFile* VfsNode::Open (int mode, const char *filename)
{
  csRef<iFile> f;

  // Look through all mounted filesystems in order
  CS::Threading::ScopedReadLock lock (mutex);
  for (size_t i = 0; i < fileSystems.GetSize (); ++i)
  {
    // read lock is set; at least 1 refcount guaranteed for filesystems
    // no need to use smart pointers here
    iFileSystem *fs = fileSystems.Get (i);

    f = fs->Open (filename, vfsPath, mode, false);
    if (f->GetStatus () == VFS_STATUS_OK)
      break; // done
    else
    {
      // failed; try next filesystem
      f.Invalidate ();
    }
  }
  return f;
}


csRef<iFileSystem> VfsNode::FindFile (const char *suffix, size_t rank)
{
  // Look through all filesystems
  CS::Threading::ScopedReadLock lock (mutex);
  size_t count = 0; // rank counter
  for (size_t i = 0; i < fileSystems.GetSize (); i++)
  {
    csRef<iFileSystem> fs = fileSystems.Get (i);

    if (fs->Exists (suffix) && rank == count++)
    {
      return fs;
    }
  }
  return csRef<iFileSystem> ();
}

bool VfsNode::Delete (const char *suffix)
{
  // make sure no one messes up with current node
  CS::Threading::ScopedReadLock lock (mutex);
  // iterate through every single filesystem
  for (size_t i = 0; i < fileSystems.GetSize (); ++i)
  {
    iFileSystem *fs = fileSystems.Get (i);
    // iFileSystem is responsible for concurrency issues
    if (fs->Delete (suffix))
      return true;
  }

  return false;
}

bool VfsNode::Exists (const char *suffix)
{
  // make sure no one messes up with current node
  CS::Threading::ScopedReadLock lock (mutex);
  // check through every single filesystem mounted
  for (size_t i = 0; i < fileSystems.GetSize (); ++i)
  {
    // if file is found in one of filesystems, return true
    if (fileSystems.Get (i)->Exists (suffix))
      return true;
  }
  return false;
}

bool VfsNode::GetFileTime (const char *suffix, csFileTime &oTime)
{
  // find 1st iFileSystem containing file 'suffix'
  csRef<iFileSystem> fs = FindFile (suffix);
  if (!fs.IsValid ())
  {
    // file not found
    return false;
  }

  // use iFileSystem method to retrieve file time
  if (fs->GetTime (suffix, oTime))
    return true;

  return false;
}

bool VfsNode::SetFileTime (const char *suffix, const csFileTime &iTime)
{
  // find 1st iFileSystem containing file 'suffix'
  csRef<iFileSystem> fs = FindFile (suffix);
  if (!fs.IsValid ())
  {
    // file not found
    return false;
  }
  
  // use iFileSystem method to set file time
  if (fs->SetTime (suffix, iTime))
    return true;

  return false;
}

bool VfsNode::GetFileSize (const char *suffix, uint64_t &oSize)
{
  // find 1st iFileSystem containing file 'suffix'
  csRef<iFileSystem> fs = FindFile (suffix);
  if (!fs.IsValid ())
  {
    // file not found
    return false;
  }
  
  // use iFileSystem method to get file size
  if (fs->GetSize (suffix, oSize))
    return true;

  return false;
}

bool VfsNode::GetFilePermission (const char *suffix, csFilePermission &oPerm)
{
  // find 1st iFileSystem containing file 'suffix'
  csRef<iFileSystem> fs = FindFile (suffix);
  if (!fs.IsValid ())
  {
    // file not found
    return false;
  }
  
  // use iFileSystem method to get permission
  if (fs->GetPermission (suffix, oPerm))
    return true;

  return false;
}

bool VfsNode::SetFilePermission (const char *suffix,
                                 const csFilePermission &iPerm)
{
  // find 1st iFileSystem containing file 'suffix'
  csRef<iFileSystem> fs = FindFile (suffix);
  if (!fs.IsValid ())
  {
    // file not found
    return false;
  }
  
  // use iFileSystem method to set permission
  if (fs->SetPermission (suffix, iPerm))
    return true;

  return false;
}

// --------------------------------------------------------------- csVFS --- //

SCF_IMPLEMENT_FACTORY (csVFS)

csVFS::VfsTls::VfsTls() : dirstack (8, 8)
{
  cwd = VFS_ROOT_PATH;
}

csVFS::csVFS (iBase *iParent) :
  scfImplementationType(this, iParent),
  root (new VfsNode (VFS_ROOT_PATH, nullptr, this, 0)), // root node
  basedir (0),
  resdir (0),
  appdir (0),
  object_reg (0),
  auto_name_counter (0),
  verbosity (VERBOSITY_NONE)
{
  heap.AttachNew (new HeapRefCounted);
  // insert root node manually
  nodeTable.Put (VFS_ROOT_PATH, root);
  // initialize compile-time static nodes other than root nodes
  //InitStaticNodes ();
}

csVFS::~csVFS ()
{
  // clean up all existing nodes
  // first step: get iterator
  typedef csHash<VfsNode *, const char *>::GlobalIterator NodeIterator;
  NodeIterator iterator = nodeTable.GetIterator ();
  
  while (iterator.HasNext ())
  {
    // get pointer
    VfsNode *node = iterator.Next ();
    // delete pointer
    delete node;
  }

  // free other resources
  cs_free (basedir);
  cs_free (resdir);
  cs_free (appdir);
}

bool csVFS::Initialize (iObjectRegistry* r)
{
  object_reg = r;
#ifdef NEW_CONFIG_SCANNING
  static const char *vfsSubdirs[] = {
    "etc/" CS_PACKAGE_NAME,
    "etc", 
    "",
    0};

  csPathsList configPaths;
  const char *crystalconfig = getenv ("CRYSTAL_CONFIG");
  if (crystalconfig)
    configPaths.AddUniqueExpanded (crystalconfig);
  
  csPathsList* basedirs = 
    csInstallationPathsHelper::GetPlatformInstallationPaths();
  configPaths.AddUniqueExpanded (*basedirs * csPathsList  (vfsSubdirs));
  delete basedirs;

  configPaths.AddUniqueExpanded (".");
#ifdef CS_CONFIGDIR
  configPaths.AddUniqueExpanded (CS_CONFIGDIR);
#endif

  configPaths = csPathsUtilities::LocateFile (configPaths, "vfs.cfg", true);
  if (configPaths.GetSize () > 0)
  {
    basedir = AllocNormalizedPath (configPaths[0].path);
  }
#else
  basedir = AllocNormalizedPath (csGetConfigPath ());
#endif

  csRef<iVerbosityManager> vm (
    csQueryRegistry<iVerbosityManager> (object_reg));
  if (vm.IsValid()) 
  {
    verbosity = VERBOSITY_NONE;
    if (vm->Enabled ("vfs.debug", false)) verbosity |= VERBOSITY_DEBUG;
    if (vm->Enabled ("vfs.scan",  true )) verbosity |= VERBOSITY_SCAN;
    if (vm->Enabled ("vfs.mount", true )) verbosity |= VERBOSITY_MOUNT;
  }

  csRef<iCommandLineParser> cmdline =
    csQueryRegistry<iCommandLineParser> (object_reg);
  if (cmdline)
  {
    resdir = AllocNormalizedPath (cmdline->GetResourceDir ());
    appdir = AllocNormalizedPath (cmdline->GetAppDir ());
  }
  
  // Order-sensitive: Mounts in first-loaded configuration file take precedence
  // over conflicting mounts in files loaded later.
  csStringSet seen;
  bool const verbose_scan = IsVerbose (VERBOSITY_SCAN);
  LoadVfsConfig(config, resdir, seen, verbose_scan);
  LoadVfsConfig(config, appdir, seen, verbose_scan);
#ifdef NEW_CONFIG_SCANNING
  bool result =	LoadVfsConfig (config, resdir, seen, verbose_scan);
  if (result && (basedir == 0))
    basedir = AllocNormalizedPath (resdir);
  result = LoadVfsConfig(config, appdir, seen, verbose_scan);
  if (result && (basedir == 0))
    basedir = AllocNormalizedPath (appdir);
  for (size_t i = 0; i < configPaths.GetSize (); i++)
  {
    LoadVfsConfig (config, configPaths[i].path,  seen, verbose_scan);
  }
#else
  LoadVfsConfig (config, basedir, seen, verbose_scan);
#endif

  return ReadConfig ();
}

bool csVFS::ReadConfig ()
{
  csRef<iConfigIterator> iterator (config.Enumerate ("VFS.Mount."));
  while (iterator->HasNext ())
  {
    iterator->Next();
    AddLink (iterator->GetKey (true), iterator->GetStr ());
  }
  return true;
}

// Expand VFS variables
csString csVFS::ExpandVars (const char *source)
{
  csString dst;
  char *src_start = CS::StrDup (source);
  char *src = src_start;
  while (*src != '\0')
  {
    // Is this a variable reference?
    if (*src == '$')
    {
      // Parse the name of variable
      src++;
      char *var = src;
      char one_letter_varname [2];
      if (*src == '(' || *src == '{')
      {
        // Parse until the end of variable, skipping pairs of '(' and ')'
        int level = 1;
        src++; var++;
        while (level > 0 && *src != '\0')
        {
          if (*src == '(' || *src == '{')
	  {
            level++;
	  }
          else if (*src == ')' || *src == '}')
	  {
            level--;
	  }
	  if (level > 0)
	    src++; // don't skip over the last parenthesis
        } /* endwhile */
        // Replace closing parenthesis with \0
        *src++ = '\0';
      }
      else
      {
        var = one_letter_varname;
        var [0] = *src++;
        var [1] = 0;
      }

      char *alternative = strchr (var, ':');
      if (alternative)
        *alternative++ = '\0';
      else
        alternative = strchr (var, '\0');

      const char *value = GetValue (var);
      if (!value)
      {
        if (*alternative)
          dst << ExpandVars (alternative);
      }
      else
      {
	// @@@ FIXME: protect against circular references
        dst << ExpandVars (value);
      }
    } /* endif */
    else
      dst << *src++;
  } /* endif */
  cs_free (src_start);
  return dst;
}

const char *csVFS::GetValue (const char *varName)
{
  // Look in environment first
  const char *value = getenv (varName);
  if (value)
    return value;

  iConfigFile *vfsConfig = &config;

  // Now look in "VFS.Unix" section, for example
  csString keyName;
  keyName << "VFS." CS_PLATFORM_NAME "." << varName;
  value = vfsConfig->GetStr (keyName, 0);
  if (value)
    return value;

  // Now look in "VFS.Alias" section for alias section name
  const char *alias = vfsConfig->GetStr ("VFS.Alias." CS_PLATFORM_NAME, 0);
  // If there is one, look into that section too
  if (alias)
  {
    keyName.Clear();
    keyName << alias << '.' << varName;
    value = vfsConfig->GetStr (keyName, 0);
  }
  if (value)
    return value;

  // Handle predefined variables here so that user
  // can override them in config file or environment

  // check for OS-specific predefined variables
  value = csCheckPlatformVFSVar (varName);
  if (value)
    return value;

  // Path separator variable?
  if (strcmp (varName, VFS_PATH_SEPARATOR_STRING) == 0)
  {
    return CS_PATH_SEPARATOR_STRING;
  }

  if (strcmp (varName, "*") == 0) // Resource directory?
    return resdir;
    
  if (strcmp (varName, "^") == 0) // Application or Cocoa wrapper directory?
    return appdir;
    
  if (strcmp (varName, "@") == 0) // Installation directory?
    return basedir;

  return 0;
}

// Retrieve a node for given vfs path, creating a path of nodes if necessary.
// Remarks: This function is not thread safe. It is callee's responsibility to
//     protect the call by using write lock appropriately.
VfsNode *csVFS::CreateNodePath (const char *vfsPath) /* expanded vfs path */
{
  CS_ASSERT (vfsPath);

  size_t length = strlen (vfsPath);
  // make sure path is already expanded
  CS_ASSERT (length && vfsPath[length - 1] == VFS_PATH_SEPARATOR);
  // allocate work buffer
  CS_ALLOC_STACK_ARRAY (char, path, length + 1);
  memcpy (path, vfsPath, length + 1);
  char *pathEnd = path + (length - 1); // points to last path separator

  VfsNode *node, *prev = nullptr, *leaf = nullptr;
  // repeat this until existing node comes up
  while ((node = nodeTable.Get (path, nullptr)) == nullptr)
  {
    // assertion;
    // if pathEnd == path, it means root directory '/'
    // it must be inside the node table.
    CS_ASSERT (pathEnd != path);

    // requested node does not exist; create one
    // TODO: check for parameter definition
    node = new VfsNode (path, vfsPath, this, GetVerbosity ());
    if (prev) // insert into the children list
      node->children.Push (prev);
    else // first iteration; this is the leaf node
      leaf = node;

    // insert into node table
    nodeTable.Put (path, node);

    // move to the upper level
    --pathEnd;
    while (pathEnd != path && *pathEnd != VFS_PATH_SEPARATOR)
      --pathEnd;
    // put null-terminator, right after path separator
    *(pathEnd + 1) = '\0';
    // store node from current iteration (for use in next iteration)
    prev = node;
  }

  // node points to pre-existing node
  // prev points to last created node
  if (prev && node != prev)
    node->children.Push (prev); // add to the children

  if (leaf) // at least 1 node has been created; return leaf node
    return leaf;

  // return pre-existing node
  return node;
}
/*
// this method is not thread safe. use appropriate locking if required
void csVFS::DestroySubtree (VfsNode *& subtreeRoot)
{
  if (subtreeRoot)
  {
    // postorder traversal; destroy all children then destroy itself
    for (size_t i = 0; i < subtreeRoot->children.Size (); ++i)
      DestroySubtree (subtreeRoot->children.Get (i));
    delete subtreeRoot;
    subtreeRoot = nullptr;
  }
}
*/

csPtr<iFileSystem> csVFS::CreateFileSystem (const char *realPath)
{
  // protocol delimiter
  const char *pDelimiter = "://";
  // protocol delimiter length
  const size_t pDelimiterLen = strlen (pDelimiter);
  // string length of path
  size_t length = strlen (realPath);
  // buffer to hold protocol portion of path (without delimiter)
  CS_ALLOC_STACK_ARRAY (char, protocol, length);

  // split protocol information
  *protocol = '\0'; // empty string to begin with
  // extract protocol specifier
  size_t match = 0; // # of consecutive match
  for (const char *pEnd = realPath; *pEnd != '\0'; ++pEnd)
  {
    if (*pEnd == pDelimiter[match])
    {
      // character match!
      if (++match == pDelimiterLen)
      {
        // delimiter found;
        // pEnd points at the last character of delimiter
        // so (pEnd - realPath) - pDelimiterLen is 1 character short
        size_t pLen = ((size_t)(pEnd - realPath)) - pDelimiterLen + 1;
        memcpy (protocol, realPath, pLen);  // copy contents
        protocol [pLen] = '\0'; // null-terminate
        break;
      }
    }
    else
      match = 0; // reset
  }

  // was it successful?
  if (*protocol == '\0')
    strcpy (protocol, "file"); // default

  // TODO: filesystem instantiation logic here
  csPtr<iFileSystem> fs = csPtr<iFileSystem> (nullptr);

  return fs;
}

bool csVFS::AddLink (const char *virtualPath, const char *realPath)
{
  // expanded virtual path
  char *expandedPath = ExpandPathFast (virtualPath, true);
  // expanded real path
  csString expandedRealPath = ExpandVars (realPath);

  // expand variables and split paths
  const char **rpList = SplitRealPath (expandedRealPath);

  if (!rpList) // path splitting failed
    return false;

  // try instantiating iFileSystems
  csRefArray<iFileSystem> fsList; // list of filesystem pointers

  while (*rpList)
  {
    // instantiate filesystem
    csRef<iFileSystem> fs = CreateFileSystem (*rpList);
    // insert into list
    fsList.Push (fs);
    ++rpList;
  }  

  bool result; // result of function call

  // got iFileSystems; find the required node and insert them
  {
    // first, acquire write lock
    CS::Threading::ScopedWriteLock lock (mutex);

    // acquire the node, creating the whole path if necessary
    VfsNode *node = CreateNodePath (expandedPath);

    if (node)
      // the tree has been made; mount filesystems
      result = node->MountFileSystem (rpList, fsList);
    else
      // something went wrong...
      result = false;
  }

  // free allocated memory
  cs_free (rpList);
  cs_free (expandedPath);

  return result;
}

char *csVFS::ExpandPathFast (const char *Path, bool IsDir)
{
  csStringFast<VFS_MAX_PATH_LEN> outname = "";
  size_t inp = 0, namelen = strlen (Path);

  // Copy 'Path' to 'outname', processing FS macros during the way
  while (inp < namelen)
  {
    // Get next path component
    csStringFast<VFS_MAX_PATH_LEN> tmp;
    while ((inp < namelen) && (Path [inp] != VFS_PATH_SEPARATOR))
      tmp << Path [inp++];

    // If this is the very first component, append it to cwd
    if (!tmp.IsEmpty() && (outname.Length() == 0))
    {
      outname = GetCwd ();
    } /* endif */

    // Check if path component is ".."
    if (tmp == "..")
    {
      size_t outp = outname.Length();
      // Skip back all '/' we encounter
      while ((outp > 0) && (outname [outp - 1] == VFS_PATH_SEPARATOR))
        outp--;
      // Skip back until we find another '/'
      while ((outp > 0) && (outname [outp - 1] != VFS_PATH_SEPARATOR))
        outp--;
      outname.Truncate (outp);
    }
    // Check if path component is "."
    else if (tmp == ".")
    {
      // do nothing
    }
    // Check if path component is "~"
    else if (tmp == "~")
    {
      // Strip entire output path; start from scratch
      outname = "/~/";
    }
    else
    {
      outname += tmp;
      if (IsDir || (inp < namelen))
        outname << VFS_PATH_SEPARATOR;
    } /* endif */

    // Skip all '/' in source path
    while ((inp < namelen) && (Path [inp] == VFS_PATH_SEPARATOR))
      inp++;
  } /* endwhile */

  // Allocate a new string and return it
  return CS::StrDup (outname);
}

csPtr<iDataBuffer> csVFS::ExpandPath (const char *Path, bool IsDir)
{
  char *xp = ExpandPathFast (Path, IsDir);
  return csPtr<iDataBuffer> (new CS::DataBuffer<> (xp, strlen (xp) + 1));
}

VfsNode *csVFS::GetNode (const char *path, const char **suffix)
{
  CS_ASSERT (path);

  // path is assumed to be absolute vfs path with all variables expanded
  size_t pathLen = strlen (path);

  // debug assertions
  // 1. absolute path must have path separator for root directory
  CS_ASSERT (pathLen && path[0] == VFS_PATH_SEPARATOR);
  // 2. path must end with VFS_PATH_SEPARATOR
  //CS_ASSERT (path[pathLen-1] == VFS_PATH_SEPARATOR);

  // copy the string to work with
  char *basePath = CS::StrDup (path);
  char *basePathEnd = basePath + pathLen; // points to null-terminator

  VfsNode *node = nullptr;

  while (basePath != basePathEnd)
  {
    if (*basePathEnd == VFS_PATH_SEPARATOR)
    {
      // hit VFS_PATH_SEPARATOR...
      // null-terminate right after the separator
      *(basePathEnd + 1) = '\0';

      // try retrieving pointer
      VfsNode *entry = nodeTable.Get (basePath, nullptr);
      if (entry)
      {
        // found corresponding node entry
        node = entry;
        if (suffix)
        {
          // suffix information has been requested.
          // calculate required offset from beginning of 'path'
          // to get desired suffix portion of path.
          // Since basePathEnd points to a path separator,
          // 1 has to be added;
          // then suffix is obtained by (path + suffixOffset)
          size_t suffixOffset = (basePathEnd - basePath) + 1;
          *suffix = path + suffixOffset;
        }
        // exit the loop.
        break;
      }
    }
    // proceed to left
    --basePathEnd;
  } // endwhile
  // free temporary memory
  cs_free (basePath);

  return node;
}
/*
bool csVFS::PreparePath (const char *path, bool isDir, VfsNode *&node,
  const char **suffix)
{
  node = 0;
  if (suffix)
    *suffix = "";
  // try expansion (get normalized path)
  char *fname = ExpandPathFast (path, isDir);
  if (!fname)
    return false;
  // retrieve corresponding node with suffix information
  node = GetNode (fname, suffix);
  cs_free (fname);
  return (node != 0);
}

bool csVFS::CheckIfMounted (const char *virtualPath)
{
  // this function used to behave differently than what had been said
  // by the comments
  bool ok = false;
  char * const xPath = ExpandPathFast (virtualPath, true);
  if (xPath != 0)
  {
    const char *suffix;
    VfsNode *node = GetNode (xPath, &suffix);
    if (node != 0)
    {
      // a node has been found.
      if (suffix && *suffix == '\0')
      {
        // node is perfect match.
        ok = !node->fileSystems.IsEmpty (); // true if filesystems are mounted
      }
    }
    cs_free (xPath);
  }
  return ok;
}
*/
bool csVFS::IsValidDir (const char *vfsPath)
{
  // expand path as directory
  char * const xPath = ExpandPathFast (vfsPath, true);

  if (!xPath)
    return false;

  // result of the function call
  bool result = false;
  {
    VfsNode *node = nullptr;
    const char *suffix;
    // acquire read lock
    CS::Threading::ScopedReadLock lock (mutex);
    // get corresponding node...
    node = GetNode (xPath, &suffix);

    if (node)
    {
      // with node and suffix information, see whether the directory exists
      if (*suffix == '\0')
      {
        // empty suffix, means node is an exact match
        // means it is a valid VFS directory
        result = true;
      }
      else
      {
        // partial match; e.g. node /some/path/ for /some/path/example/
        // true if given suffix refers to directory
        // TODO: implement VfsNode::IsDir ()
        result = node->IsDir (suffix);
      }
    }

  }
  // free memory
  cs_free (xPath);
  return result;
}

bool csVFS::ChDir (const char *path)
{
  // First, transform Path to absolute
  char *newwd = ExpandPathFast (path, true);
  if (!newwd)
    return false;
  tls->cwd = newwd;
  cs_free (newwd);
  return true;
}

const char *csVFS::GetCwd ()
{
  return tls->cwd;
}

void csVFS::PushDir (const char *Path)
{
  tls->dirstack.Push (tls->cwd);

  if (Path != 0)
    ChDir (Path);
}

bool csVFS::PopDir ()
{
  if (!tls->dirstack.GetSize ())
    return false;
  char *olddir = (char *) tls->dirstack.Pop ();
  bool retcode = ChDir (olddir);
  delete[] olddir;
  return retcode;
}

bool csVFS::Exists (const char *path)
{
  if (!path)
    return false;

  // TODO: deal with concurrency issues

  VfsNode *node;
  const char *suffix;
  // expand path (i.e. normalize)
  char *pathExpanded = ExpandPathFast (path, false);

  // acquire read lock
  CS::Threading::ScopedReadLock lock (mutex);

  // get the deepest node possible
  node = GetNode (pathExpanded, &suffix);

  bool result = false;

  while (node && suffix)
  {
    // if suffix is an empty string, it means the node itself
    if (suffix && suffix [0] == '\0')
    {
      result = true;
      break;
    }
    // otherwise, check for existence
    if (node->Exists (suffix))
    {
      result = true;
      break;
    }
    // the file was not found in this node
    // climb the directory tree..
    node = node->parent;
    // add current level to suffix;
    // 1. roll back until we hit a separator
    while (suffix != pathExpanded && *suffix != VFS_PATH_SEPARATOR)
      --suffix;
    // 2. skip (possibly) consecutive separators
    while (suffix != pathExpanded && *suffix == VFS_PATH_SEPARATOR)
      --suffix;
    // 3. roll back until we hit separator again
    while (suffix != pathExpanded && *suffix != VFS_PATH_SEPARATOR)
      --suffix;
    // if suffix is path separator, go forward one step
    if (*suffix == VFS_PATH_SEPARATOR)
      ++suffix;
  }

  cs_free (pathExpanded);

  return result;
}

csRef<iStringArray> csVFS::MountRoot (const char *Path)
{
  scfStringArray* outv = new scfStringArray;

  if (Path != 0)
  {
    csRef<iStringArray> roots = csInstallationPathsHelper::FindSystemRoots();
    size_t i;
    size_t n = roots->GetSize ();
    for (i = 0 ; i < n ; i++)
    {
      const char *t = roots->Get(i);
      csString s(t);
      size_t const slen = s.Length ();
      char c = '\0';

      csString vfs_dir;
      vfs_dir << Path << '/';
      for (size_t j = 0; j < slen; j++)
      {
        c = s.GetAt(j);
        if (c == '_' || c == '-' || isalnum(c))
	  vfs_dir << (char)tolower(c);
      }

      csString real_dir(s);
      if (slen > 0 && ((c = real_dir.GetAt(slen - 1)) == '/' || c == '\\'))
        real_dir.Truncate(slen - 1);
      real_dir << "$/";

      outv->Push (vfs_dir);
      Mount(vfs_dir, real_dir);
    }
  }

  return csPtr<iStringArray> (outv);
}

csPtr<iStringArray> csVFS::FindFiles (const char *Path)
{
  scfStringArray *fl = new scfStringArray;		// the output list

  //csString news;
  if (Path != 0)
  {
    VfsNode *node;				// the node we are searching
    char mask [VFS_MAX_PATH_LEN + 1];		// the filename mask
    char *xPath = ExpandPathFast (Path, false); // the expanded path
    const char *suffixWithMask = nullptr;   // suffix relative to the node
    // acquire read lock
    CS::Threading::ScopedReadLock lock (mutex);
    // get node, as well as suffix portion of path
    node = GetNode (xPath, &suffixWithMask);
    // Now separate the mask from directory suffix
    size_t dirlen = strlen (suffixWithMask);
    while (dirlen && suffixWithMask[dirlen - 1] != VFS_PATH_SEPARATOR)
      --dirlen;
    strcpy (mask, suffixWithMask + dirlen);
    // extract suffix without mask
    csString suffix (suffixWithMask, dirlen);
    // if there is no mask, apply generic mask '*'
    if (!mask[0])
      strcpy (mask, "*");

    // in new structure, node must not be NULL; root node at the worst case
    CS_ASSERT (node);

    // if node is perfect match (suffix is empty), it might have children
    // relevant to this particular query
    if (suffix.IsEmpty ())
    {
      // first add all nodes that are located one level deeper
      // these are "directories" and will have a slash appended
      size_t count = node->children.GetSize ();

      for (size_t i = 0; i < count; ++i)
      {
        VfsNode *child = node->children.Get (i);
        // does it end with slash? if not, add one

        // construct the path
        //news.Clear ();
        //news.Append (child->vfsPath);

        // if the path doesn't already exist, add it
        if (fl->Find (child->vfsPath) == csArrayItemNotFound)
          fl->Push (child->vfsPath);
      }
    }


    // Now find all files in given directory node
    if (node)
      node->FindFiles (suffix, mask, fl);

    // free memory
    cs_free (xPath);
  }

  return csPtr<iStringArray> (fl);
}

csPtr<iFile> csVFS::Open (const char *filename, int mode)
{
  if (!filename)
    return 0;

  iFile *f = nullptr;
  char *path = ExpandPathFast (filename, false); // expanded path

  {
    const char *suffix;

    // acquire read lock
    CS::Threading::ScopedReadLock lock (mutex);

    VfsNode *node = GetNode (path, &suffix);

    if (node)
      f = node->Open (mode, suffix);
  }

  cs_free (path);

  return csPtr<iFile> (f);
}

bool csVFS::Sync ()
{
  //ArchiveCache->FlushAll ();
  return true;
}

// Move a file from old virtual path to new virtual path
bool csVFS::MoveFile (const char *OldPath, const char *NewPath)
{
  // TODO: implement feature
  // 1. if two virtual path is within same iFileSystem, use iFileSystem::Move()
  // 2. if copy between two different system can be done efficiently, (e.g.
  // between two native filesystem) take advantage of it (solution required)
  // 3. otherwise, fall back to copy & delete operation
  // iFileSystem for OldPath is determined using the same resolving algorithm
  // while that for NewPath is the first writable instance of highest priority
  return false;
}

csPtr<iDataBuffer> csVFS::ReadFile (const char *FileName, bool nullterm)
{
  csRef<iFile> F (Open (FileName, VFS_FILE_READ));
  if (!F)
    return 0;

  size_t Size = F->GetSize ();
  csRef<iDataBuffer> data (F->GetAllData (nullterm));
  if (data)
  {
    return csPtr<iDataBuffer> (data);
  }

  char *buff = (char *)cs_malloc (Size + 1);
  if (!buff)
    return 0;

  // Make the file zero-terminated in the case we'll use it as an ASCIIZ string
  buff [Size] = 0;
  if (F->Read (buff, Size) != Size)
  {
    cs_free (buff);
    return 0;
  }

  return csPtr<iDataBuffer> (new CS::DataBuffer<> (buff, Size));
}

bool csVFS::WriteFile (const char *FileName, const char *Data, size_t Size)
{
  csRef<iFile> F (Open (FileName, VFS_FILE_WRITE));
  if (!F)
    return false;

  bool success = (F->Write (Data, Size) == Size);
  return success;
}

bool csVFS::DeleteFile (const char *filename)
{
  if (!filename)
    return false;

/*
  VfsNode *node;
  char suffix [VFS_MAX_PATH_LEN + 1];
  if (!PreparePath (FileName, false, node, suffix, sizeof (suffix)))
    return false;
*/
  // expand vfs path
  char *xPath = ExpandPathFast (filename, false);
  const char *suffix = nullptr;
  // get node
  VfsNode *node = GetNode (xPath, &suffix);

  bool result = node->Delete (suffix);

  //ArchiveCache->CheckUp ();
  return result;
}

bool csVFS::SymbolicLink (const char *Target, const char *Link, int priority)
{
  csRef<iDataBuffer> rpath = GetRealPath (Link);
  if (!rpath->GetSize ())
    return false;
  Mount (Target, rpath->GetData ());
  return true;
}

bool csVFS::Mount (const char *virtualPath, const char *realPath)
{
  if (!virtualPath || !realPath)
    return false;
  if (IsVerbose (VERBOSITY_MOUNT))
    csPrintf("VFS_MOUNT: Mounted: Vpath %s, Rpath %s\n", virtualPath, realPath);
  // expand real path
  csString rpExpanded = ExpandVars (realPath);
  // split real paths
  const char **rpList = SplitRealPath (rpExpanded);

  if (!rpList) // path splitting failed
    return false;

  // expand vfs path (i.e. normalize)
  char *pathExpanded = ExpandPathFast (virtualPath, true);

  // try instantiating iFileSystems
  csRefArray<iFileSystem> fsList; // list of filesystem pointers

  while (*rpList)
  {
    // instantiate filesystem
    csRef<iFileSystem> fs = CreateFileSystem (*rpList);
    // insert into list
    fsList.Push (fs);
    ++rpList;
  }  

  bool result; // result of function call

  // got iFileSystems; find the required node and insert them
  {
    // first, acquire write lock
    CS::Threading::ScopedWriteLock lock (mutex);

    // acquire the node, creating the whole path if necessary
    VfsNode *node = CreateNodePath (pathExpanded);

    if (node)
      // the tree has been made; mount filesystems
      result = node->MountFileSystem (rpList, fsList);
    else
      // something went wrong...
      result = false;

    // check whether node is empty, and delete it if necessary
    if (node->IsEmpty ())
    {
      // remove it from the tree
      // 1. delete from HT
      nodeTable.DeleteAll (node->vfsPath);
      // 2. remove from parent's children list
      VfsNode *parent = node->parent;
      size_t idx = parent->children.Find (node);
      if (idx != csArrayItemNotFound)
        parent->children.DeleteIndex (idx);
      // 3. free memory
      delete node;
      result = false;
    }
  }

  cs_free (pathExpanded);
  cs_free (rpList);

  return result;
}

bool csVFS::Unmount (const char *virtualPath, const char *realPath)
{
  if (!virtualPath)
    return false;

  if (IsVerbose (VERBOSITY_MOUNT))
    csPrintf("VFS_MOUNT: Unmounting: Vpath %s, Rpath %s\n",
	     virtualPath, realPath);

  csString rpExpanded = ExpandVars (realPath);
  // expand variables and split real paths
  const char **rpList = SplitRealPath (rpExpanded);
  if (!rpList) // path splitting failed
    return false;
  const char *suffix;
  // expand vfs path (i.e. normalize)
  char *pathExpanded = ExpandPathFast (virtualPath, true);

  // acquire upgradable lock
  CS::Threading::ScopedUpgradeableLock lock (mutex);
  // get node of given vfs path
  VfsNode *node = GetNode (pathExpanded, &suffix);

  bool result = false;

  // check whether 1. node exists and 2. no suffix (exact match)
  if (node && *suffix == '\0')
  {
    // got the exact node; try unmounting
    if (node->UnmountFileSystem (rpList))
    {
      // unmount succeeded
      result = true;
      // check whether node is empty
      if (node->IsEmpty ())
      {
        // upgrade the lock
        mutex.UpgradeUnlockAndWriteLock ();
        // remove from config list
        csString s ("VFS.Mount.");
        s += node->configKey;
        config.DeleteKey (s);
        // 1. delete from HT
        nodeTable.DeleteAll (node->vfsPath);
        // 2. remove from parent's children list
        VfsNode *parent = node->parent;
        size_t idx = parent->children.Find (node);
        if (idx != csArrayItemNotFound)
          parent->children.DeleteIndex (idx);
        // 3. free memory
        delete node;
        // unlock
        mutex.WriteUnlock ();
      }
    }
  }

  // print debug message
  if (IsVerbose (VERBOSITY_MOUNT))
    csPrintf ("VFS_MOUNT: Unmounted: Vpath %s, Rpath %s\n",
	      virtualPath, realPath);

  // free memory
  cs_free (pathExpanded);
  cs_free (rpList);

  return result;
}

size_t csVFS::GetMountedNodes (csArray<VfsNode *> &nodeList)
{
  size_t initialSize = nodeList.GetSize ();

  // FIFO (queue) for level-order traversal
  csFIFO<VfsNode *> queue;
  // insert root node
  queue.Push (root);
  // perform level-order traversal
  while (queue.GetSize () > 0)
  {
    VfsNode *node = queue.PopTop ();
    // if there are mounted filesystems, add to the list
    if (!node->fileSystems.IsEmpty ())
      nodeList.Push (node);
    size_t const count = node->children.GetSize ();
    for (size_t i = 0; i < count; ++i)
    {
      // add children to the queue
      queue.Push (node->children.Get (i));
    }
  }
  // return # of nodes added to the list
  return nodeList.GetSize () - initialSize;
}


bool csVFS::SaveMounts (const char *filename)
{
  CS::Threading::ScopedWriteLock lock (mutex);
  csArray<VfsNode *> nodeList;
  // get list of mounts
  GetMountedNodes (nodeList);
  // iterate through all nodes
  for (size_t i = 0; i < nodeList.GetSize (); i++)
  {
    VfsNode *node = (VfsNode *)nodeList.Get (i);
    // setup parameters
    csString key ("VFS.Mount.");
    csString mounts = node->GetMountListString ();
    key << node->configKey;
    // update config file
    config.SetStr (key, mounts);
  }
  return config.Save (filename);
}

bool csVFS::LoadMountsFromFile (iConfigFile* file)
{
  bool success = true;

  // Merge options from new file to ensure that new
  // variable assignments are available for mounts.
  csRef<iConfigIterator> iter = file->Enumerate ();
  while (iter->HasNext ())
  {
    iter->Next();
    config.SetStr (iter->GetKey (true), iter->GetStr ());
  }
  // Now mount the paths in the file.
  iter = file->Enumerate ("VFS.Mount.");
  while (iter->HasNext ())
  {
    iter->Next();
    const char *rpath = iter->GetKey (true);
    const char *vpath = iter->GetStr ();
    if (!Mount (rpath, vpath))
    {
      csPrintfErr("VFS_WARNING: cannot mount %s to %s\n",
		  CS::Quote::Double (rpath), CS::Quote::Double (vpath));
      success = false;
    }
  }

  return success;
}


// tests whether given 'dir' path could be chdir'd, and making sure a file of
// name 'filename' exists in the directory (if supplied)
bool csVFS::TryChDirAuto (const char *dir, const char *filename)
{
  bool ok = false;
  if (IsValidDir (dir))
  {
    // valid directory: could be ChDir'd
    if (filename == nullptr)
      ok = true; // if no filename specified, that's fine
    else
    {
      // make sure file exists
      csString testPath = ComposeVfsPath (dir, filename);
      ok = Exists (testPath);
    }
  }
  return ok && ChDir (dir);
}

bool csVFS::ChDirAuto (const char *path, const csStringArray* paths,
	const char *vfspath, const char* filename)
{
  // If the VFS path is valid we can use that.
  if (TryChDirAuto (path, filename))
    return true;

  // Now try to see if we can get it from one of the 'paths'.
  if (paths)
  {
    for (size_t i = 0; i < paths->GetSize (); ++i)
    {
      csString testpath = ComposeVfsPath (paths->Get (i), path);
      if (TryChDirAuto (testpath, filename))
	    return true;
    }
  }

  // all previous attempts failed..
  // assume 'path' refers to a real path

  // First check if it is a zip file.
  //bool is_zip = IsZipFile (path);
  char * npath = TransformPath (path, false);

  // See if we have to generate a unique VFS name.
  csString tryvfspath;
  if (vfspath)
    tryvfspath = vfspath;
  else
  {
    tryvfspath.Format ("/tmp/__automount%d__", auto_name_counter);
    ++auto_name_counter; // increment counter
  }

  // mount with given path
  bool result = Mount (tryvfspath, npath);
  if (result)
  {
    // if mount succeeded, try chdir
    result = TryChDirAuto (tryvfspath, filename);
    if (!result)
    {
      // operation failed; unmount
      Unmount (tryvfspath, npath);
    }
  }
  cs_free (npath);
  return result;
}

bool csVFS::GetFileTime (const char *filename, csFileTime &oTime)
{
  if (!filename)
    return false;

  char *expandedPath = ExpandPathFast (filename, false);

  const char *suffix = nullptr;
  VfsNode *node = GetNode (expandedPath, &suffix);

  bool success = node ? node->GetFileTime (suffix, oTime) : false;

  cs_free (expandedPath);

  return success;
}

bool csVFS::SetFileTime (const char *filename, const csFileTime &iTime)
{
  if (!filename)
    return false;

  char *expandedPath = ExpandPathFast (filename, false);

  const char *suffix = nullptr;
  VfsNode *node = GetNode (expandedPath, &suffix);

  bool success = node ? node->SetFileTime (suffix, iTime) : false;

  cs_free (expandedPath);

  return success;
}

bool csVFS::GetFilePermission (const char *filename, csFilePermission &oPerm)
{
  if (!filename)
    return false;

  char *expandedPath = ExpandPathFast (filename, false);

  const char *suffix = nullptr;
  VfsNode *node = GetNode (expandedPath, &suffix);

  bool success = node ? node->GetFilePermission (suffix, oPerm) : false;

  cs_free (expandedPath);

  return success;
}

bool csVFS::SetFilePermission (const char *filename,
                               const csFilePermission &iPerm)
{
  if (!filename)
    return false;

  char *expandedPath = ExpandPathFast (filename, false);

  const char *suffix = nullptr;
  VfsNode *node = GetNode (expandedPath, &suffix);

  bool success = node ? node->SetFilePermission (suffix, iPerm) : false;

  cs_free (expandedPath);

  return success;
}

#ifndef CS_SIZE_T_64BIT
bool csVFS::GetFileSize (const char *filename, size_t &oSize)
{
  if (!filename)
    return false;

  VfsNode *node;
  const char *suffix = nullptr;

  const char *expandedPath = ExpandPathFast (filename, false);
  // acquire read lock
  CS::Threading::ScopedReadLock lock (mutex);

  node = GetNode (expandedPath, &suffix);

  // TODO: implement error mechanism for large files
  bool success = node ? node->GetFileSize (suffix, oSize) : false;

  cs_free (expandedPath);

  return success;
}
#endif

bool csVFS::GetFileSize (const char *filename, uint64_t &oSize)
{
  if (!filename)
    return false;

  VfsNode *node;
  const char *suffix = nullptr;

  char *expandedPath = ExpandPathFast (filename, false);
  // acquire read lock
  CS::Threading::ScopedReadLock lock (mutex);

  node = GetNode (expandedPath, &suffix);

  bool success = node ? node->GetFileSize (suffix, oSize) : false;

  cs_free (expandedPath);

  return success;
}

csPtr<iDataBuffer> csVFS::GetRealPath (const char *filename)
{
  if (!filename)
    return csPtr<iDataBuffer> (nullptr);

  // expand vfs path
  char *pathExpanded = ExpandPathFast (filename, false);
  // make sure no one writes to current node
  CS::Threading::ScopedReadLock lock (mutex);
  // find appropriate node
  const char *suffix;
  VfsNode *node = GetNode (pathExpanded, &suffix);

  if (!node) // couldn't find the node
    return csPtr<iDataBuffer> (nullptr);

  bool ok = false;
  char path [CS_MAXPATHLEN + 1]; // buffer to play with

  for (size_t i = 0; !ok && i < node->fileSystems.GetSize (); i++)
  {
    //const char *rpath = node->RPathV.Get (i);
    const char *rpath = node->realPaths.Get (i);
    cs_snprintf (path, sizeof(path), "%s%s", (const char *)rpath, suffix);
    strcat (strcpy (path, rpath), suffix);
    ok = Exists (suffix); //access (path, F_OK) == 0;
  }

  if (!ok)
  {
    //CS_ASSERT(node->RPathV.GetSize () != 0);
    CS_ASSERT (node->fileSystems.GetSize () != 0);
    const char *defpath = node->fileSystems.Get(0)->GetRootRealPath ();
    CS_ASSERT (defpath != 0);
    size_t const len = strlen (defpath);
    if (len > 0 && defpath[len - 1] != VFS_PATH_SEPARATOR)
      cs_snprintf (path, sizeof(path), "%s%c%s", defpath, VFS_PATH_SEPARATOR,
		   suffix);
    else
      cs_snprintf (path, sizeof(path), "%s%s", defpath, suffix);
  }

  return csPtr<iDataBuffer> (
    new CS::DataBuffer<> (CS::StrDup (path), strlen (path) + 1));
}

csRef<iStringArray> csVFS::GetMounts ()
{
  scfStringArray *mounts = new scfStringArray;
  // acquire read lock, so we can safely read
  CS::Threading::ScopedReadLock lock (mutex);
  csArray<VfsNode *> nodeList;
  // get a list of nodes with actual mounted filesystems
  GetMountedNodes (nodeList);
  // add vfs paths of all nodes into the list
  for (size_t i = 0; i < nodeList.GetSize (); ++i)
  {
    mounts->Push (nodeList.Get (i)->vfsPath);
  }

  // use csPtr trick; no need to call DecRef ()
  return csPtr<iStringArray> (mounts);
}

csRef<iStringArray> csVFS::GetRealMountPaths (const char *vfsPath)
{
  if (!vfsPath)
    return 0;

  scfStringArray* rmounts = new scfStringArray;
  // expand given vfs path
  char *expandedPath = ExpandPathFast (vfsPath, true);

  const char *suffix = nullptr;
  // try getting the node
  VfsNode *node = GetNode (expandedPath, &suffix);

  // if node exists with empty suffix, this is a perfect match
  if (node && *suffix == '\0')
  {
    // TODO: implement adding list of real paths here
    //for (size_t i = 0; i< node->RPathV.GetSize (); i++)
      //rmounts->Push (node->RPathV[i]);
  }

  // use csPtr trick; no need to call DecRef ()
  return csPtr<iStringArray> (rmounts);
}

}
CS_PLUGIN_NAMESPACE_END(VFS)

// anonymous namespace helper definitions
namespace
{
// split a list of multiple paths delimited by VFS_PATH_DIVIDER.
// pathList is assumed to be already expanded with ExpandVars()
// the callee is required to free the returned pointer via cs_free(), just once.
const char **SplitRealPath (const char *pathList)
{
  // calculate # of segments
  size_t seg = 1;

  // each VFS_PATH_DIVIDER adds 1 segment
  for (const char *cur = pathList; *cur != '\0'; ++cur)
    if (*cur == VFS_PATH_DIVIDER)
      ++seg;

  size_t length;
  char *buffer; // single buffer containing required information
  const char **header; // header (list of strings)
  size_t headerSize;   // size of header in bytes
  char *contents;      // contents (contents of string)

  // header size = (size of char *) * ((# of segments) + 1)
  headerSize = sizeof (char *) * (seg + 1);

  // get string length
  length = strlen (pathList);
  // allocate required memory
  // buffer size = header size + (string length) + 1 bytes
  buffer = (char *)cs_malloc (headerSize + length + 1);
  // set variables
  header = (const char **)buffer;
  contents = buffer + headerSize;
  // initialize
  memset (header, 0, headerSize);
  memcpy (contents, pathList, length + 1);

  const char **entry = header; // current entry of list
  char *begin = contents; // beginning position of current section
  char *cur = contents; // current position in string
  char * const end = contents + (length + 1); // end position in string

  // split paths by commas
  while (cur != end)
  {
    if (*cur == VFS_PATH_DIVIDER || *cur == '\0')
    {
      // just hit path divider (or end of string)...
      // 1. put null-terminator
      *cur = '\0';
      // 2. left trim; skip # of whitespaces from the beginning
      begin += strspn (begin, CS_VFSSPACE);
      // 3. right trim; find 1st whitespace...
      char *right;
      for (right = cur;
           right != begin && strchr (CS_VFSSPACE, *(right-1));
           --right);

      if (right != begin)
      {
        // we got the string!
        *right = '\0';
        // store the segment address
        *entry = begin;
        // proceed to next entry..
        ++entry;
      }

      // start next segment
      begin = cur + 1;      
    }
    // proceed to next char
    ++cur;
  }

  // return the buffer as a whole
  return header;
}

// compose two vfs path components
csString ComposeVfsPath (const char *base, const char *suffix)
{
  csString path (base); // start from base path
  // append the suffix and return
  return AppendVfsPath (path, suffix);
}

// append suffix to given vfs base path
csString &AppendVfsPath (csString &base, const char *suffix)
{
  const size_t len = base.Length (); // length of path

  // if the base path already doesn't end with VFS_PATH_SEPARATOR, add one.
  if (len > 0 && base[len - 1] != VFS_PATH_SEPARATOR)
    base << VFS_PATH_SEPARATOR;

  // add the suffix part
  base << suffix;
  // done.
  return base;
}

// Transform a path so that every \ or / is replaced with $/.
// If 'add_end' is true there will also be a $/ at the end if there
// is not already one there.
// The result of this function must be deleted with cs_free().
char * TransformPath (const char* path, bool add_end)
{
  // The length we allocate below is enough in all cases.
  char * npath = (char*)cs_malloc (strlen (path)*2+2+1);
  char * np = npath;
  bool lastispath = false;
  while (*path)
  {
    if (*path == '$' && (*(path+1) == '/' || *(path+1) == '.'))
    {
      *np++ = '$';
      *np++ = *(path+1);
      path++;
      if (*(path+1) == '/')
        lastispath = true;
    }
    else if (*path == '/' || *path=='\\')
    {
      *np++ = '$';
      *np++ = '/';
      lastispath = true;
    }
    else if (*path == '.')
    {
      *np++ = '$';
      *np++ = '.';
      lastispath = false;
    }
    else
    {
      *np++ = *path;
      lastispath = false;
    }
    path++;
  }
  if (add_end && !lastispath)
  {
    *np++ = '$';
    *np++ = '/';
  }
  *np++ = 0;
  return npath;
}

char *AllocNormalizedPath (const char *s)
{
  if (s != 0)
  {
    // add trailing delimiter, then duplicate
    return CS::StrDup (ComposeVfsPath (s, ""));
  }
  return nullptr;
}

bool LoadVfsConfig (csConfigFile& cfg, const char *dir,
                    csStringSet& seen, bool verbose)
{
  bool ok = false;
  if (dir != 0)
  {
    csString s = ComposeVfsPath (dir, "vfs.cfg");
    if (seen.Contains(s))
      ok = true;
    else
    {
      seen.Request(s);
      bool const merge = !cfg.IsEmpty();
      ok = cfg.Load(s, 0, merge, false);
      if (ok && verbose)
      {
	const char *t = merge ? "merged" : "loaded";
	csPrintf("VFS_NOTIFY: %s configuration file: %s\n", t, s.GetData());
      }
    }
  }
  return ok;
}
} // end of anonymous namespace

