/*
    Crystal Space Virtual File System SCF interface
    Copyright (C) 1998,1999 by Andrew Zabolotny <bit@eltech.ru>

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

#ifndef __CS_IUTIL_VFS_H__
#define __CS_IUTIL_VFS_H__

// current dev environment is 64-bit
// @@@FIXME: move this to configure phase
#define CS_SIZE_T_64BIT

/**\file
 * Virtual File System SCF interface
 */
/**\addtogroup vfs
 * @{ */
#include "csutil/scf.h"
#include "csutil/hash.h"
#include "iutil/databuff.h"
#include "iutil/pluginconfig.h"
#include <time.h>

//#ifndef CS_OFF64_T_DEFINED
//#ifdef CS_64B_OFF_T
//typedef off_t off64_t;
//#else
//typedef int64_t off64_t;
//#endif
//#endif

namespace CS
{
  namespace Memory
  {
    struct iAllocator;
  } // namespace Memory
} // namespace CS
struct iConfigFile;

class csStringArray;

/**
 * File permission structure - used to query
 * and set basic permissions of a file.
 */
struct csFilePermission
{
  /// Read permission of user
  bool user_read;
  /// Write permission of user
  bool user_write;
  /// Execute permission of user
  bool user_execute;

  /// Read permission of group
  bool group_read;
  /// Write permission of group
  bool group_write;
  /// Execute permission of group
  bool group_execute;

  /// Read permission of others
  bool others_read;
  /// Write permission of others
  bool others_write;
  /// Execute permission of others
  bool others_execute;

  /// Empty csFilePermission constructor. Members are not initialized.
  csFilePermission () { }

  /** Construct csFilePermission from octal representation of Unix permissions.
   * \param octal Unix-style permissions in octal format (e.g. 0744). The order
   *   of digits is from the left: 0 (octal prefix), user, group, and others.
   */
  csFilePermission (uint32 octal)
  {
    // use assignment operator to update internal fields
    this->operator= (octal);
  }

  /**
   * Assign individual fields in csFilePermission from octal representation of
   * Unix permissions.
   * \param octal Unix-style permissions in octal format (e.g. 0744). The order
   *   of digits is from the left: 0 (octal prefix), user, group, and others.
   */
  csFilePermission &operator= (uint32 octal)
  {    
    user_read      = (octal & 0400) != 0;
    user_write     = (octal & 0200) != 0;
    user_execute   = (octal & 0100) != 0;
    group_read     = (octal & 0040) != 0;
    group_write    = (octal & 0020) != 0;
    group_execute  = (octal & 0010) != 0;
    others_read    = (octal & 0004) != 0;
    others_write   = (octal & 0002) != 0;
    others_execute = (octal & 0001) != 0;

    return *this;
  }
};

/**
 * File time structure - used to query and set
 * the last-modification time of a file.
 */
struct csFileTime
{
  /// Second, 0..59
  int sec;
  /// Minute, 0..59
  int min;
  /// Hour, 0..23
  int hour;
  /// Day, 1..31
  int day;
  /// Month, 0..11
  int mon;
  /// Year, 1768, 1900, 2001, ...
  int year;

  ///empty constructor.
  csFileTime () {}

  /// Construct a csFileTime item from a <tt>struct tm</tt>
  csFileTime (const struct tm& time)
  {
      *this = time;
  }

  /// Assign a <tt>struct tm</tt>.
  void operator= (const struct tm& time)
  {
    sec  = time.tm_sec;
    min  = time.tm_sec;
    hour = time.tm_hour;
    day  = time.tm_mday;
    mon  = time.tm_mon;
    year = time.tm_year + 1900;
  }

  /// Create a <tt>struct tm</tt> from object.
  operator struct tm () const
  {
    struct tm time;
    time.tm_sec  = sec;
    time.tm_min  = min;
    time.tm_hour = hour;
    time.tm_mday = day;
    time.tm_mon  = mon;
    time.tm_year = year - 1900;
    return time;
  }
};

namespace CS
{
  namespace Deprecated
  {
    CS_DEPRECATED_METHOD_MSG ("Use assign operator of csFileTime.")
    static inline void ASSIGN_FILETIME (csFileTime &ft, const struct tm &time)
    {
      ft = time;
    }
  }
}
/** 
 * \deprecated Deprecated in 2.0. Use assign operator of csFileTime.
 */
#define ASSIGN_FILETIME(ft,tm)	\
  CS::Deprecated::ASSIGN_FILETIME(ft, tm);

/// Composite path divider
#define VFS_PATH_DIVIDER        ','
/// The "virtual" path separator
#define VFS_PATH_SEPARATOR      '/'
/// The maximal "virtual" path+filename length
#define VFS_MAX_PATH_LEN        256

/**\name File opening flags
 * @{ */
/// File open mode mask
#define VFS_FILE_MODE		0x0000000f
/// Open file for reading
#define VFS_FILE_READ		0x00000000
/// Open file for writing
#define VFS_FILE_WRITE		0x00000001
/// Open file for append
#define VFS_FILE_APPEND		0x00000002
/// Store file uncompressed (no gain possible)
#define VFS_FILE_UNCOMPRESSED	0x80000000
/** @} */

/**\name VFS status codes
 * @{ */
/// File status ok
#define	VFS_STATUS_OK                   0
/// Unclassified error
#define VFS_STATUS_OTHER                1
/// Device has no more space for file data
#define	VFS_STATUS_NOSPACE              2
/// Not enough system resources
#define VFS_STATUS_RESOURCES            3
/**
 * Access denied: either you have no write access, the filesystem is read-only
 * or you tried to read a file opened for write access
 */
#define VFS_STATUS_ACCESSDENIED         4
/// An error occured during reading or writing data
#define VFS_STATUS_IOERROR              5
/// File is too large to be fully addressed in memory space
#define VFS_STATUS_FILETOOLARGE         6
/// File or directory was not found
#define VFS_STATUS_FILENOTFOUND         7
/// Directory within base path is a file
#define VFS_STATUS_DIRISFILE            8
/// One or more arguments are invalid
#define VFS_STATUS_INVALIDARGS          9
/// Requested operation is not supported
#define VFS_STATUS_UNSUPPORTED          0x00008000
/// Requested operation should be supported, but not implemented yet
#define VFS_STATUS_NOTIMPLEMENTED       0x00008001
/** @} */

/**\name File positioning modes
 * @{ */
/// Absolute positioning, starting from 0
#define VFS_POS_ABSOLUTE	0
/// Same as VFS_POS_ABSOLUTE
#define VFS_POS_BEGINNING	VFS_POS_ABSOLUTE
/// Relative positioning from current cursor position
#define VFS_POS_CURRENT		1
/// Relative positioning from the end
#define VFS_POS_END		2
/** @} */

/// List of extra options to be supplied for VFS operations
typedef csHash<csVariant,csString> csVfsOptionList;

/**
 * A replacement for FILE type in the virtual file space.
 *
 * Main creators of instances implementing this interface:
 * - iVFS::Open()
 * - iFileSystem::Open()
 */
struct iFile : public virtual iBase
{
  SCF_INTERFACE(iFile, 3, 0, 0);

  /// Query file name (in VFS)
  virtual const char *GetName () = 0;

  /// Query file size
  virtual uint64_t GetSize () = 0;

  /**
   * Check (and clear) file last error status
   * \sa #VFS_STATUS_ACCESSDENIED
   */
  virtual int GetStatus () = 0;

  /**
   * Read dataSize bytes and place them into the buffer at which Data points.
   * \param data Pointer to the buffer into which the data should be read.  The
   *   buffer should be at least dataSize bytes in size.
   * \param dataSize Number of bytes to read.
   * \return The number of bytes actually read.  If an error occurs, zero is
   *   returned.  Invoke GetStatus() to retrieve the error code.
   */
  virtual size_t Read (char *data, size_t dataSize) = 0;

  /**
   * Write dataSize bytes from the buffer at which Data points.
   * \param data Pointer to the data to be written.
   * \param dataSize Number of bytes to write.
   * \return The number of bytes actually written.  If an error occurs, zero is
   *   returned.  Invoke GetStatus() to retrieve the error code.
   */
  virtual size_t Write (const char *data, size_t dataSize) = 0;

  /// Flush stream.
  virtual void Flush () = 0;

  /// Returns true if the stream is at end-of-file, else false.
  virtual bool AtEOF () = 0;

  /// Query current file pointer.
  virtual uint64_t GetPos () = 0;

  /**
   * Set new file pointer.
   * \param newpos New position in file.
   * \return True if the operation succeeded, else false.
   */
  virtual bool SetPos (off64_t newpos, int ref = VFS_POS_ABSOLUTE) = 0;

  /**
   * Request whole content of the file as a single data buffer.
   * \param nullterm Set this to true if you want a null char to be appended
   *  to the buffer (e.g. for use with string functions.)
   * \remarks Null-termination might have a performance penalty (depending upon
   *  where the file is stored.) Use only when needed.
   * \return The complete data contained in the file; or an invalidated pointer
   *  if this object does not support this function (e.g. write-only VFS
   *  files).  Check for an invalidated result via csRef<>::IsValid().  Do not
   *  modify the contained data!
   */
  virtual csPtr<iDataBuffer> GetAllData (bool nullterm = false) = 0;

  /**
   * Request whole content of the file as a single data buffer.
   * Uses the allocator \a allocator if memory allocations are necessary.
   * \return The complete data contained in the file; or an invalidated pointer
   *  if this object does not support this function (e.g. write-only VFS
   *  files).  Check for an invalidated result via csRef<>::IsValid().  Do not
   *  modify the contained data!
   */
  virtual csPtr<iDataBuffer> GetAllData (CS::Memory::iAllocator* allocator) = 0;

  /**
   * Request whole or part of the content of the file as a file object.
   * \param offset Offset of data to return.
   * \param size Size of data to return. If \c ~0 all the data starting at
   *  \a offset up to the end of the file is returned.
   * \return A file object operating on a part of the original file.
   * \remarks Note that the returned file will not support writing.
   */
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0) = 0;
};

/**
 * Represents a file system mountable on Virtual File System tree.
 */
struct iFileSystem : public virtual iBase
{
  SCF_INTERFACE(iFileSystem, 1, 0, 3);

  /**
   * Opens a file within current file system. 
   * \param path Virtual path relative to current mount point.
   * \param pathPrefix Absolute virtual path where current iFileSystem is
   *   mounted.
   * \param mode File mode to open.
   * \param options (optional) Arbitrary options that might be used in file 
   *   opening process. Available options might vary between iFileSystem
   *   implementations.
   * \return Pointer to iFile instance of corresponding file.
   */
  virtual csPtr<iFile> Open (const char *path,
                             const char *pathPrefix,
                             int mode,
                             const csVfsOptionList &options
                                         = csVfsOptionList ()) = 0;

  /**
   * Change root directory of current filesystem.
   * \param newRoot Path of new root directory relative to current root, in VFS format
   *   without any variables
   * \param mustExist If true, operation will fail if given new root directory does not exist
   */
  virtual bool ChRoot (const char *newRoot, bool mustExist = false) = 0;

  /**
   * Moves a file within current file system. 
   * \param OldPath Virtual path of source file relative to current mount point.
   * \param NewPath Virtual path of destination path relative to current mount
   *   point.
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool Move (const char *oldPath, const char *newPath) = 0;

  /**
   * Get permission of specified file within current file system.
   * \param filename Virtual path of file to get permission
   * \param oPerm Reference to csFilePermission structure to be written
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool GetPermission (const char *filename,
                              csFilePermission &oPerm) = 0;

  /**
   * Set permission of specified file within current file system.
   * \param filename Virtual path of file to set permission
   * \param iPerm csFilePermission structure containing desired permission.
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool SetPermission (const char *filename,
                              const csFilePermission &iPerm) = 0;

  /**
   * Get file time of specified file within current file system.
   * \param filename Virtual path of file to get time
   * \param oTime Reference of csFileTime structure to be written
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool GetTime (const char *filename, csFileTime &oTime) = 0;

  /**
   * Set file time of specified file within current file system.
   * \param filename Virtual path of file to set time
   * \param iTime csFileTime structure containing desired file time.
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool SetTime (const char *filename, const csFileTime &iTime) = 0;

  /**
   * Query file size of specified file within current file system.
   * \param filename Virtual path of file to query size
   * \param oSize 64-bit unsigned integer variable to receive file size
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool GetSize (const char *filename, uint64_t &oSize) = 0;

  /**
   * Query whether a file or directory of given name exists.
   * \param filename Virtual path of file or directory to check existence
   * \return true if exists; false otherwise.
   * \remarks This method does not change error status.
   */
  virtual bool Exists (const char *filename) = 0;

  /**
   * Deletes a given file.
   * \param filename Virtual path of file to delete
   * \return true if operation succeeded; false otherwise. Use GetStatus() for
   *   error information.
   */
  virtual bool Delete (const char *filename) = 0;

  /**
   * Query directory listing of given path.
   * \param path Path to directory
   * \return iStringArray containing list of files and directories in the given
   *       directory if succeeded. Returns an invalid smart pointer when failed;
   *       check with csRef<>::IsValid().
   */
  virtual csPtr<iStringArray> List (const char *path) = 0;

  /**
   * Get real path of which this filesystem is based on.
   * \return csString containing real path of filesystem root
   */
  virtual csString GetRootRealPath () = 0;

  /**
   * Query and reset last error status.
   * \return Last error status. If there was no error, returns VFS_STATUS_OK.
   */
  virtual int GetStatus () = 0;

  /**
   * Query whether a given path is directory.
   * \param filename Virtual path to see whether it is a directory
   * \return true if path represents a directory; false otherwise.
   * \remarks This function also returns false when given path does not have
   *   a corresponding filesystem object. Use Exists() to determine whether
   *   the path exists.
   */
  virtual bool IsDir (const char *path) = 0;

  /**
   * Flush any pending read/write operation within current iFileSystem.
   */
  virtual bool Flush () = 0;
};

/**
 * Instantiates iFileSystem from a path string
 */
struct iFileSystemFactory : public virtual iBase
{
  SCF_INTERFACE (iFileSystemFactory, 0, 0, 0);

  /**
   * If this factory supports given real path, returns pointer to iFileSystem.
   * Otherwise, returns 0 (nullptr).
   * \param realPath Real path without protocol specifier
   * \param oStatus reference of variable to receive VFS_STATUS status code
   */
  virtual csPtr<iFileSystem> Create (const char *realPath, int &oStatus) = 0;
};

/**
 * Instantiates iFileSystem for an archive file.
 */
struct iArchiveHandler : public virtual iBase
{
  SCF_INTERFACE(iArchiveHandler, 1, 0, 0);

  /**
   * Checks whether this archive handler supports a given file, and returns
   * iFileSystem instance if supported.
   * \param parentFS    parent iFileSystem containing requested archive file
   * \param archivePath path of archive file within parent filesystem
   */
  virtual csPtr<iFileSystem> GetFileSystem (iFileSystem *parentFS,
                                            const char *parentFSPath,
                                            const char *archivePath) = 0;
};

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
 *
 * Main creators of instances implementing this interface:
 * - The VFS plugin (crystalspace.kernel.vfs)
 *
 * Main ways to get pointers to this interface:
 * - csQueryRegistry<iVFS>()
 */
struct iVFS : public virtual iBase
{
  SCF_INTERFACE(iVFS, 4, 0, 1);


  /// Set current working directory
  virtual bool ChDir (const char *path) = 0;

  /// Get current working directory
  virtual const char *GetCwd () = 0;

  /**
   * Push current directory and optionally change to a different directory.
   * \param path Path which should become the new working directory after the
   *   current directory is remembered.  May be null.
   * \remarks If path is not the null pointer, then current working directory
   *   is remembered and Path is set as the new working directory.  If Path is
   *   the null pointer, then the current working directory is remembered, but
   *   not changed.
   */
  virtual void PushDir (char const* path = 0) = 0;
  /**
   * Pop current directory.  Set the current working directory to the one most
   * recently pushed.
   * \return True if there was a remembered directory and the invocation of
   *   ChDir() for the remembered directory succeeded; else false if there was
   *   no remembered directory, or ChDir() failed.
   */
  virtual bool PopDir () = 0;

  /**
   * Expand given virtual path, interpret all "." and ".."'s relative to
   * 'current virtual directory'.
   * \param path The path to expand.
   * \param isDir If true, the expanded path will be terminated with '/'.
   * \return A new iDataBuffer object.
   */
  virtual csPtr<iDataBuffer> ExpandPath (
    const char *path, bool isDir = false) = 0;

  /// Check whether a file exists
  virtual bool Exists (const char *path) = 0;

  /**
   * Find absolute paths of all files in a virtual directory and return an
   * array with their names.
   */
  virtual csPtr<iStringArray> FindFiles (const char *path) = 0;

  /**
   * Open a file on the VFS filesystem.
   * \param filename The VFS path of the file in the VFS filesystem.
   * \param mode Combination of VFS_FILE_XXX constants.
   * \return A valid iFile if the file was opened successfully, otherwise an
   *  invalidated iFile.  Use csRef<>::IsValid() to check validity.
   * \sa #VFS_FILE_MODE
   */
  virtual csPtr<iFile> Open (const char *filename, int mode) = 0;

  /**
   * Open a file on the VFS filesystem.
   * \param filename The VFS path of the file in the VFS filesystem.
   * \param mode Combination of VFS_FILE_XXX constants.
   * \param options Arbitrary options that might be used in file-opening
   *  process. Each option might or might not honored, depending on underlying
   *  iFileSystem implementation.
   * \return A valid iFile if the file was opened successfully, otherwise an
   *  invalidated iFile.  Use csRef<>::IsValid() to check validity.
   * \sa #VFS_FILE_MODE
   */
  virtual csPtr<iFile> Open (const char *filename,
                             int mode,
                             const csVfsOptionList &options) = 0;

  /**
   * Get an entire file at once. This is more effective than opening files 
   * and reading the file in blocks.  Note that the returned buffer can 
   * be null-terminated (so that it can be conveniently used with string 
   * functions) but the extra null-terminator is not counted as part of the 
   * returned size.
   * \param filename VFS path of the file to be read.
   * \param nullterm Null-terminate the returned buffer.
   * \return An iDataBuffer containing the file contents if the file was opened
   *  and read successfully, otherwise a null reference.  Use
   *  csRef<>::IsValid() to check validity.
   * \remarks Null-termination might have a performance penalty (dependent on
   *  where the file is stored). Use only when needed.
   */
  virtual csPtr<iDataBuffer> ReadFile (const char *filename,
    bool nullterm = true) = 0;

  /**
   * Write an entire file in one pass.
   * \param name Name of file to write.
   * \param data Pointer to the data to be written.
   * \param size Number of bytes to write.
   * \return True if the write succeeded, else false.
   */
  virtual bool WriteFile (const char *name, const char *data, size_t size) = 0;

  /**
   * Delete a file on VFS
   * \return True if the deletion succeeded, else false.
   */
  virtual bool DeleteFile (const char *filename) = 0;

  /**
   * Close all opened archives, free temporary storage etc.
   * \return True if the synchronization succeeded, else false.
   */
  virtual bool Sync () = 0;

  /**
   * Create or add a symbolic link within the VFS 
   *  (works like unix 'ln -s' command)
   * If the link already exists, then the target will be added to the link
   * At the moment just remounts it at `Target'.
   * \param target The target that the link will point to
   * \param link The path of the link within the VFS, if this is 0 then the 
   *  link will be created in the current directory with the same name as 
   *  the target
   * \param priority Currently unused
   * \return True if successful, else false.
   */
  virtual bool SymbolicLink(const char *target, const char *link = 0, 
    int priority = 0) = 0;

  /**
   * Mount an VFS path on a "real-world-filesystem" path.
   * \param virtualPath The location in the virtual filesystem in which to
   *   mount realPath.
   * \param realPath The physical filesystem path to mount at virtualPath.
   *   All VFS pseudo-variables and anything that appears in the right-hand
   *   side of an equal sign in vfs.cfg is valid.
   * \return True if the mount succeeded, else false.
   */
  virtual bool Mount (const char *virtualPath, const char *realPath) = 0;

  /**
   * Unmount a VFS path.
   * \param virtualPath The location in the virtual filesystem which is to be
   *   unmounted.
   * \param realPath The physical filesystem path corresponding to the virtual
   *   path.
   * \remarks A single virtual path may represent multiple physical locations;
   *   in which case, the physical locations appear as a conglomerate in the
   *   virtual filesystem.  The realPath argument allows unmounting of just a
   *   single location represented by the given virtualPath.  If realPath is
   *   the null pointer, then all physical locations represented by virtualPath
   *   are umounted.
   * \return True if the unmount succeeded, else false.
   */
  virtual bool Unmount (const char *virtualPath, const char *realPath) = 0;

  /**
   * Mount the root directory or directories beneath the given virtual path.
   * \return A list of absolute virtual pathnames mounted by this operation.
   * \remarks On Unix, there is only a single root directory, but on other
   *   platforms there may be many.  For example, on Unix, if virtualPath is
   *   "/native", then the single Unix root directory "/" will be mounted
   *   directly to "/native".  On Windows, which has multiple root directories,
   *   one for each drive letter, they will be mounted as "/native/a/",
   *   "/native/c/", "/native/d/", and so on.
   */
  virtual csRef<iStringArray> MountRoot (const char *virtualPath) = 0;

  /**
   * Save current configuration back into configuration file
   * \return True if the operation succeeded, else false.
   */
  virtual bool SaveMounts (const char *filename) = 0;
  /**
   * Loads mounts from a configuration file
   * \return True if no error occured, false otherwise.
   */
  virtual bool LoadMountsFromFile (iConfigFile* file) = 0;

  /**
   * Convenience function to set the current VFS directory to the given path.
   * The path can be any of the following:
   * - A valid VFS path. In that case this is equivalent to calling
   *   ChDir(path).
   * - A real path (using '/', '\', or '$/' for path delimiter). In this
   *   case this path will be mounted on the 'vfspath' parameter and a
   *   ChDir(vfspath) will happen.
   * - A real path to a zip file. In this case the zip file will
   *   be mounted on the 'vfspath' parameter and a ChDir(vfspath) will
   *   happen.
   *
   * \param path is the path to mount (VFS, real, zip, ...)
   * \param paths is an array of possible vfs paths to also look in
   *        for the given file. This can an empty array or 0. If not empty
   *        then this routine will first try to find the 'path' as such
   *        if it is a VFS path. Otherwise it will scan all paths here
   *        and add the path as a prefix. Only if all this fails will it
   *        try to interprete the input path as a real world dir or zip file.
   * \param vfspath is the temporary name to use in case it is not a vfs
   *        path. If not given then this routine will try to use a suitable
   *        temporary name.
   * \param filename is an optional filename that must be present on the
   *        given path. If this is given then the chdir will only work
   *        if the given file is present.
   */
  virtual bool ChDirAuto (const char* path, const csStringArray* paths = 0,
  	const char* vfspath = 0, const char* filename = 0) = 0;

  /**
   * Move file from oldPath to newPath
   * \param oldPath virtual path of the file to move
   * \param newPath new virtual path of file
   * \remarks This feature is currently unsupported.
   */
  virtual bool MoveFile (const char *oldPath, const char *newPath) = 0;

  /**
   * Query file date/time.
   * \return True if the query succeeded, else false.
   */
  virtual bool GetFileTime (const char *filename, csFileTime &oTime) = 0;
  /**
   * Set file date/time.
   * \return True if the operation succeeded, else false.
   */
  virtual bool SetFileTime (const char *filename, const csFileTime &iTime) = 0;


  /**
   * Query file size (without opening it).
   * \return True if the query succeeded, else false.
   */
  virtual bool GetFileSize (const char *filename, uint64_t &oSize) = 0;

  // if 64-bit, size_t is equivalent to uint64_t.
  // deprecated overload is only available for non 64-bit systems.
#ifndef CS_SIZE_T_64BIT
  /**
   * Query file size (without opening it).
   * \return True if the query succeeded, else false.
   */
  CS_DEPRECATED_METHOD_MSG("Use uint64_t overload instead.")
  virtual bool GetFileSize (const char *filename, size_t &oSize) = 0;
#endif

  /**
   * Query file permission.
   * \return True if the query succeeded, else false.
   */
  virtual bool GetFilePermission (const char *filename, 
                                  csFilePermission &oPerm) = 0;

  /**
   * Set file permission.
   * \return True if the operation succeeded, else false.
   */
  virtual bool SetFilePermission (const char *filename,
                                  const csFilePermission &iPerm) = 0;
  /**
   * Query real-world path from given VFS path.
   * \param filename The virtual path for which the physical path is desired.
   * \remarks This will work only for files that are stored on real filesystem,
   *   not in archive files.  You should expect this function to return an
   *   invalidated iDataBuffer for filesystems which do not support this
   *   operation.
   * \return An iDataBuffer containing the actual physical path corresponding
   *   to the virtual path named by filename, or an invalidated iDataBuffer if
   *   the operation fails or is not supported.  Use csRef<>::IsValid() to
   *   check validity.
   */
  virtual csPtr<iDataBuffer> GetRealPath (const char *filename) = 0;

  /**
   * Get a list of all current virtual mount paths
   * \return An iStringArray containing all the current virtual mounts
   */
  virtual csRef<iStringArray> GetMounts () = 0;

  /**
   * Get the real paths associated with a mount
   * \param virtualPath The virtual path of a mount point
   * \return An iStringArray containing all the real filesystem paths associated
   * with the virtualPath mount, or an empty array if the virtualPath isn't
   * mounted.
   */
  virtual csRef<iStringArray> GetRealMountPaths (const char *virtualPath) = 0;
};

/** @} */

#endif // __CS_IUTIL_VFS_H__
