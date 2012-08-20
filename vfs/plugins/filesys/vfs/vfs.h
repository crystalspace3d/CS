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

#ifndef __CS_VFS_H__
#define __CS_VFS_H__

#include "csutil/cfgfile.h"
#include "csutil/parray.h"
#include "csutil/memheap.h"
#include "csutil/refcount.h"
#include "csutil/scf_implementation.h"
#include "csutil/threading/rwmutex.h"
#include "csutil/threading/tls.h"
#include "csutil/stringarray.h"
#include "iutil/vfs.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"		  

struct iConfigFile;

CS_PLUGIN_NAMESPACE_BEGIN(VFS)
{

class VfsNode;

struct HeapRefCounted :
  public CS::Memory::CustomAllocatedDerived<CS::Memory::Heap>,
  public CS::Utility::AtomicRefCount
{
};

typedef CS::Memory::AllocatorHeap<csRef<HeapRefCounted> > VfsHeap;

/**
 * The Virtual Filesystem Class is intended to be the only way for Crystal
 * Space engine to access the files. This gives unified control over the
 * way how files are found, read and written. VFS gives the following
 * goodies over the standard file i/o functions:
 * - Multiple search directories. Several "real" directories can be
 *   collected together into one "virtual" directory.
 * - Directories can be mapped to "real" directories as well as to
 *   archives (.zip files). Files are compressed/decompressed
 *   transparently for clients.
 * - The Virtual File System is unique across all operating systems
 *   Crystal Space supports, no matter of features of the underlying OS.
 *
 * This class has only most basic features of a real filesystem: file
 * reading and writing (no simultaneous read and write mode are allowed
 * because it would be rather complex to implement it for archives).
 * However, most programs don't even need such functionality, and for
 * sure Crystal Space itself doesn't need it. Files open for writing are
 * always truncated. A simple meaning for getting a list of files in a
 * virtual directory is implemented; however the user is presented with
 * only a list of file names; no fancy things like file size, time etc
 * (file size can be determined after opening it for reading).
 */
class csVFS : public scfImplementation2<csVFS, iVFS, iComponent>
{
private:
  friend class VfsNode;

  /// Mutexes to make VFS thread-safe.
  mutable CS::Threading::ReadWriteMutex nodeMutex;
  mutable CS::Threading::ReadWriteMutex factoryMutex;

  // Set of data to store in thread-local storage
  struct VfsTls
  {
    // Current working directory (in fact, the automatically-added prefix path)
    // NOTE: cwd ALWAYS ends in '/'!
    csString cwd;
    // Directory stack (used in PushDir () and PopDir ())
    csStringArray dirstack;
    
    VfsTls();
  };

  // Set of metadata for archive handler
  struct ArchiveHandlerMetadata
  {
    csString className;
    csStringArray extensions;

    ArchiveHandlerMetadata () { }
    ArchiveHandlerMetadata (const char *handlerName) :
      className (handlerName)
    {
    }

    bool operator== (const ArchiveHandlerMetadata &another)
    {
      if (this == &another)
        return true;

      return className.Compare (another.className);
    }

    bool operator!= (const ArchiveHandlerMetadata &another)
    {
      return !(*this == another);
    }
  };

  // A table of VFS nodes
  csHash<VfsNode *, const char *> nodeTable;
  // Pointer to root node
  VfsNode * const root;
  // List of factories
  csStringArray factoryList;
  // List of archive handlers
  csArray<ArchiveHandlerMetadata> archiveHandlerList;

  // Thread-local values
  CS::Threading::ThreadLocal<VfsTls> tls;
  // The installation directory (the value of $@)
  char *basedir;
  // Full path of application's resource directory (the value of $*)
  char *resdir;
  // Full path of the directory containing the application executable or
  // the Cocoa application wrapper (the value of $^)
  char *appdir;
  // The initialization file
  csConfigFile config;
  // Reference to the object registry.
  iObjectRegistry *object_reg;
  // ChDirAuto() may need to generate unique temporary names for mount points.
  // It uses a counter to do so.
  int auto_name_counter;
  // Verbosity flags.
  unsigned int verbosity;
public:
  enum
  {
    VERBOSITY_NONE  = 0,
    VERBOSITY_DEBUG = 1 << 0,
    VERBOSITY_SCAN  = 1 << 1,
    VERBOSITY_MOUNT = 1 << 2,
    VERBOSITY_ALL  = ~0
  };

  csRef<HeapRefCounted> heap;
public:
  /// Initialize VFS by reading contents of given INI file
  csVFS (iBase *iParent);
  /// Virtual File System destructor
  virtual ~csVFS ();

  /// Is verbosity enabled for a mode or set of modes?
  bool IsVerbose (unsigned int mask) const { return (verbosity & mask) != 0; }
  /// Get verbosity flags.
  unsigned int GetVerbosity () const { return verbosity; }

  /// Set current working directory
  virtual bool ChDir (const char *path);
  /// Get current working directory
  virtual const char *GetCwd ();

  /// Push current directory
  virtual void PushDir (char const* path = 0);
  /// Pop current directory
  virtual bool PopDir ();

  /**
   * Expand given virtual path, interpret all "." and ".."'s relative to
   * 'current virtual directory'. Return a new iString object.
   * If IsDir is true, expanded path ends in an '/', otherwise no.
   */
  virtual csPtr<iDataBuffer> ExpandPath (const char *path, bool isDir = false);

  /// Check whenever a file exists
  virtual bool Exists (const char *path);

  /// Find all files in a virtual directory and return an array of their names
  virtual csPtr<iStringArray> FindFiles (const char *path);
  /// Replacement for standard fopen()
  virtual csPtr<iFile> Open (const char *filename, int mode);
  /// Replacement for standard fopen(), allowing arbitrary options
  virtual csPtr<iFile> Open (const char *filename,
                             int mode,
                             const csHash<csVariant,csString> &options);
  /**
   * Get an entire file at once. You should delete[] returned data
   * after usage. This is more effective than opening files and reading
   * the file in blocks.  Note that the returned buffer is always null-
   * terminated (so that it can be conveniently used with string functions)
   * but the extra null-terminator is not counted as part of the returned size.
   */
  virtual csPtr<iDataBuffer> ReadFile (const char *filename, bool nullterm);
  /// Write an entire file in one pass.
  virtual bool WriteFile (const char *filename, const char *data, size_t size);

  /// Delete a file on VFS
  virtual bool DeleteFile (const char *filename);

  /// Close all opened archives, free temporary storage etc.
  virtual bool Sync ();

  /// Create or add a symbolic link
  virtual bool SymbolicLink (const char *target, const char *link = 0, 
    int priority = 0);

  /// Mount an VFS path on a "real-world-filesystem" path
  virtual bool Mount (const char *virtualPath, const char *realPath);
  /// Unmount an VFS path; if RealPath is 0, entire VirtualPath is unmounted
  virtual bool Unmount (const char *virtualPath, const char *realPath);
  
  /// Mount the root directory or directories 
  virtual csRef<iStringArray> MountRoot (const char *virtualPath);

  /// Save current configuration back into configuration file
  virtual bool SaveMounts (const char *filename);
  /// Load a configuration file
  virtual bool LoadMountsFromFile (iConfigFile* file);

  /// Auto-mount ChDir.
  virtual bool ChDirAuto (const char* path, const csStringArray* paths = 0,
  	const char* vfspath = 0, const char* filename = 0);

  /// Initialize the Virtual File System
  virtual bool Initialize (iObjectRegistry *object_reg);

  /// Move one file to new VFS path
  virtual bool MoveFile (const char *oldPath, const char *newPath);

  /// Query file local date/time
  virtual bool GetFileTime (const char *filename, csFileTime &oTime);
  /// Set file local date/time
  virtual bool SetFileTime (const char *filename, const csFileTime &iTime);
 
  /// Query file permissions
  virtual bool GetFilePermission (const char *filename,
                                  csFilePermission &oPerm);
  /// Set file permissions
  virtual bool SetFilePermission (const char *filename,
                                  const csFilePermission &iPerm);

// this function is enabled only when size_t is not 64-bit
#ifndef CS_SIZE_T_64BIT
  /// Query file size (without opening it)
  virtual bool GetFileSize (const char *filename, size_t &oSize);
#endif
  /// Query file size (without opening it)
  virtual bool GetFileSize (const char *filename, uint64_t &oSize);

  /**
   * Query real-world path from given VFS path.
   * This will work only for files that are stored on real filesystem,
   * not in archive files. You should expect this function to return
   * 0 in this case.
   */
  virtual csPtr<iDataBuffer> GetRealPath (const char *filename);

  /// Get all current virtual mount paths
  virtual csRef<iStringArray> GetMounts ();

  /// Get the real paths associated with a mount
  virtual csRef<iStringArray> GetRealMountPaths (const char *virtualPath);

private:
  /// Initialize static nodes
  void InitStaticNodes ();

  /// Register a plugin of given name
  void RegisterPlugin (const char *className);

  /// Register a given protocol
  bool AddProtocolHandler (const char *className,
                           const char *protocol);

  /// Register archive handler
  bool AddArchiveHandler (const ArchiveHandlerMetadata &metadata);

  /// Get list of factory classes for given protocol
  void GetFactoryClasses (csStringArray &oList, const char *protocol);

  /**
   * Create archive filesystem from given parent filesystem.
   * Acquire read lock before calling this function.
   */
  csPtr<iFileSystem> CreateArchiveFS (iFileSystem *parent,
                                      const char *parentPath,
                                      const char *archiveName,
                                      const char *suffix);

  /// Same as ExpandPath() but with less overhead
  char *ExpandPathFast (const char *path, bool isDir = false);

  /// Read and set the VFS config file
  bool ReadConfig ();

  /// Add a virtual link: real path can contain $(...) macros
  virtual bool AddLink (const char *virtualPath, const char *realPath);

  /// Instantiate a single file system, which could be used for mounting
  csPtr<iFileSystem> CreateFileSystem (const char *realPath);

  /**
   * Retrieve the best-matching node for given VFS path.
   * \param vfsPath VFS path of the node to request
   * \param suffix (out, optional) address of the pointer to store the address
   *    of suffix portion of path
   * \return pointer to node best representing given VFS path
   * \remarks This function is not thread safe. It is caller's responsibility
   *    to protect the function call by using read locks.
   */
  VfsNode *GetNode (const char *path, const char **suffix = nullptr);

  /**
   * Retrieve the list of nodes with mounted filesystems.
   * \param nodeList reference to csArray instance to receive list of nodes
   * \return number of nodes added to the list
   */
  size_t GetMountedNodes (csArray<VfsNode *> &nodeList);

  /// Get value of a VFS/environment variable
  const char *GetValue (const char *varName);

  /// Copy a string from src to dst and expand all VFS/environment variables
  csString ExpandVars (char const *src);

  /**
   * Retrieve a node for given VFS path, creating a path of nodes if necessary.
   * \param vfsPath VFS path of the node to request
   * \return pointer to node representing given VFS path
   * \remarks This function is not thread safe. It is caller's responsibility
   *    to protect the function call by using write locks.
   */
  VfsNode *CreateNodePath (const char *vfsPath);

  /**
   * Check if a virtual path represents a valid directory.
   */
  bool IsValidDir (const char *vfsPath);

  /**
   * Helper for ChDirAuto(). Checks if dir is mounted and invokes ChDir() if it
   * is and returns true. If filename is specified, then ensures that it exists
   * in dir. If filename is not present, returns false and does not invoke
   * ChDir().
   */
  bool TryChDirAuto(char const* dir, char const* filename);
};

}
CS_PLUGIN_NAMESPACE_END(VFS)

#endif // __CS_VFS_H__
