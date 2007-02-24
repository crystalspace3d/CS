/*
    Crystal Space Virtual File System class
    Copyright (C) 1998,1999,2000 by Andrew Zabolotny <bit@eltech.ru>
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

#include "vfs.h"
#include "filesystem.h"
#include "csutil/cmdline.h"
#include "csutil/databuf.h"
#include "csutil/scfstringarray.h"
#include "csutil/csstring.h"
#include "csutil/strset.h"
#include "csutil/sysfunc.h"
#include "csutil/syspath.h"
#include "csutil/vfsplat.h"
#include "iutil/databuff.h"
#include "iutil/objreg.h"

CS_IMPLEMENT_PLUGIN

/*********************************************
  TODO : Sort out circular symbolic links !!
**************************************************/

CS_PLUGIN_NAMESPACE_BEGIN(vfs)
{

// Extract a filename from a full path
static csString ExtractFileName(const char *FullPath)
{
  csString strPath = FullPath;

    // Remove trailing slash
  if (strPath.GetAt(strPath.Length() - 1) == VFS_PATH_SEPARATOR)
    strPath.Truncate(strPath.Length() - 1);

  csString strFileName;

  size_t newlen = strPath.FindLast(VFS_PATH_SEPARATOR);
  strPath.SubString(strFileName, newlen+1);

  return strFileName;
}

static csString ExtractParentName(const char *path)
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

// ----------------------------------------------------------- VfsNode ---//

// Structure to hold data about VfsNode paths
struct MappedPathData
{
  // The name of the path
  csString name;

  // Index into csVfs fsPlugins array
  size_t pluginIndex;

  // The priority of the path
  int priority;

  // Is this a symbolic link
  bool symlink;
};

// Comparison function for mapped paths
int CompareMappedPaths(const MappedPathData &a, const MappedPathData &b)
{
  return (a.priority - b.priority);
}

/**
 * This class stores information about the directories in the VFS.
 * Every directory in the VFS is represented by a VfsNode, which
 * contains information about mapped paths, links and subdirectories
 * and provides access to the physical file systems functions.
 */
class VfsNode
{
public:
  ~VfsNode();

  // Get the name of this node
  const char * GetName() const 
  { return ((const char*) VirtualPathname); }

private:
  friend class csVFS;

  // Constructor
  VfsNode(const char *virtualPathname, csVFS *parentVFS, 
    VfsNode* parentNode);

  // A pointer to the parent of this node
  VfsNode *ParentNode;

  // The name of this node in the VFS tree
  csString VirtualPathname;

  // The real paths and symbolic links mapped to this node
  csArray<MappedPathData> MappedPaths;

  // List the files in this directory tree
  void FindFiles(const char *Mask, iStringArray *FileList);

  // Return a pointer to a node representing a virtual path
  VfsNode *FindNode(const char *VirtualPath);

  // Get all the current mounted directories
  void GetMounts(scfStringArray *MountArray);

  // Open a file
  iFile* Open (const char *FileName, int Mode);

  // Delete a file
  bool Delete(const char *FileName);

  // A vector of pointers to the subdirectories of this node
  VfsVector SubDirectories;

  // Does this node contain a file
  bool ContainsFile(const char *FileName) const;

  // Get the timestamp of a file in this node
  bool GetFileTime (const char *FileName, csFileTime &oTime) const;

  // Set the timestamp of a file in this node
  bool SetFileTime (const char *FileName, const csFileTime &iTime);

  // Get the size of a file in this node
  bool GetFileSize (const char *FileName, size_t &oSize);

  // Get the real path of a file in this node
  bool GetRealPath(const char *FileName, csString &path) const;

  // Add a subdirectory to this node
  bool AddSubDirectory(VfsNode *SubDir);

  // Add a subdirectory to this node
  bool RemoveSubDirectory(VfsNode *SubDir);

  // Check if the node contains a mapping to a specified path
  bool IsMapped(const char *Path);

  // Write the mounts of this node to the config file
  void WriteMounts(iConfigFile  *config);

  // Allow VFS to access node internals
  csVFS *ParentVFS;
};

// Constructor
VfsNode::VfsNode(const char *virtualPathname, csVFS *parentVFS, 
                 VfsNode* parentNode)
{
  // Create the name
	VirtualPathname = virtualPathname;

  ParentVFS = parentVFS;

  if (!ParentVFS)
  {
    delete this;
    return;
  }

  ParentNode = parentNode;

  SubDirectories.Empty();

  if (parentNode)
  {
    // Add this node to the parents directory
    parentNode->AddSubDirectory(this);
  }
}

// Destructor
VfsNode::~VfsNode()
{
  // Delete all subdirectory nodes
  SubDirectories.DeleteAll();
}

// Add a directory to this node
bool VfsNode::AddSubDirectory(VfsNode *SubDir)
{
  // Find the index to place the subdirectory
  size_t m = 0;
  size_t l = 0;
  size_t r = SubDirectories.Length ();

  while (l < r)
  {
    m = (l + r) / 2;
    int cmp = strcmp(SubDirectories[m]->GetName(), SubDir->GetName());

    if (cmp == 0)
    {
      // Duplicate Entry
      return false;
    }
    else if (cmp < 0)
      l = m + 1;
    else
      r = m;
  }
  if ((m + 1) == r)
    m++;

  // Insert the subdirectory
  SubDirectories.Insert(m, SubDir);
  return true;
}

// Remove a subdirectory from this node
bool VfsNode::RemoveSubDirectory(VfsNode *SubDir)
{
  if (SubDir == 0)
  {
    SubDirectories.DeleteAll();
    return true;
  }

  size_t m = 0;
  size_t l = 0;
  size_t r = SubDirectories.Length();
  
  while (l < r)
  {
    m = (l + r) / 2;
    int cmp = strcmp(SubDirectories[m]->GetName(), SubDir->GetName());

    if (cmp == 0)
    {
      // Delete the subdirectory
      SubDirectories.DeleteIndex(m);
      return true;
    }
    else if (cmp < 0)
      l = m + 1;
    else
      r = m;
  }
  
  // Not found
  return false;
}

// Find a node
VfsNode * VfsNode::FindNode(const char *VirtualPath)
{
  if (strlen(VirtualPath) < VirtualPathname.Length())
    return 0;

  // This is the node
  if (VirtualPathname.Compare(VirtualPath))
    return this;

  size_t m = 0;
  size_t l = 0;
  size_t r = SubDirectories.Length();
  int cmp = 0;
  
  // Use a binary search to find subdirectory
  while (l < r)
  {
    m = (l + r) / 2;
    cmp = strcmp(SubDirectories[m]->GetName(), VirtualPath);

	  if (cmp == 0)
	    return SubDirectories[m]->FindNode(VirtualPath);

    if (cmp < 0)
	    l = m + 1;

    else
      r = m;
  }

  // Not found
  return 0;
}

// Get all mounts
void VfsNode::GetMounts(scfStringArray * MountArray)
{
  // Add this node
  MountArray->Push((const char *) VirtualPathname);

  // Add all subdirectories
  for (size_t i =0; i < SubDirectories.Length(); i++)
  {
	  SubDirectories[i]->GetMounts(MountArray);  
  }
}

// Open a file in this node
iFile* VfsNode::Open (const char *FileName, int Mode)
{
  if (!FileName)
    return false;

  csString csFileName = FileName;
  csString FileToFind;
  iFile* file = 0;

  // Find the file within the node mappings
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
      VfsNode *symlinkNode 
        = ParentVFS->GetDirectoryNode(MappedPaths[i].name);
      if (symlinkNode)
      {
        file = symlinkNode->Open(csFileName, Mode);

        if (file)
          return file;
      }
    }
    else
    {
      FileToFind.Clear();
      FileToFind = MappedPaths[i].name;

      if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
        FileToFind.Append(CS_PATH_SEPARATOR);

      if (MappedPaths[i].pluginIndex == 0)
      {
          static char vfs_separator [] = {VFS_PATH_SEPARATOR, 0};
          static char cs_separator [] = {CS_PATH_SEPARATOR, 0};
          csFileName.ReplaceAll(vfs_separator, cs_separator);
      }

      FileToFind.Append(csFileName);
      
      // Open the file using the correct iFileSystem plugin
      iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);
      if (!fs)
        continue;

      file = fs->Open(FileToFind, Mode);

      // Found the file
      if (file)
        return file;
    }
  }

  // File was not found
  return 0;
}

// Does this node contain the file
bool VfsNode::ContainsFile(const char *FileName) const
{
  if (!FileName)
    return false;

  csString FileToFind;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
      VfsNode *symlinkNode 
        = ParentVFS->GetDirectoryNode(MappedPaths[i].name);
      if (symlinkNode)
      {
        if (symlinkNode->ContainsFile(FileName))
          return true;
      }
    }
    else
    {
       FileToFind.Clear();
       FileToFind = MappedPaths[i].name;

       if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
         FileToFind.Append(CS_PATH_SEPARATOR);

       FileToFind.Append(FileName);
       iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);

       if (!fs)
         continue;

       // Checkl if the file exists
       if (fs->Exists((const char *) FileToFind) != fkDoesNotExist)
         return true;
    }
  }

  // File is not within this node
  return false;
}

// Get the real path of a file
bool VfsNode::GetRealPath(const char *FileName, csString &path) const
{
  if (!FileName)
  {
    path.Clear();
    return false;
  }

  csString FileToFind;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
       VfsNode *symlinkNode 
         = ParentVFS->GetDirectoryNode(MappedPaths[i].name);
       if (symlinkNode)
       {
         if (symlinkNode->GetRealPath(FileName, path))
           return true;
       }
    }
    else
    {
      FileToFind.Clear();
      FileToFind = MappedPaths[i].name;

      if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
        FileToFind.Append(CS_PATH_SEPARATOR);

      FileToFind.Append(FileName);
      iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);
      
      if (!fs)
      {
        continue;
      }
      // Check if the file exists
      if (fs->Exists(FileToFind) != fkDoesNotExist)
      {
        // The real path is correct
        path = FileToFind;
        return true;
      }
    }
  }

  // File is not within this node
  return false;
}

// Delete a file from this node
bool VfsNode::Delete(const char *FileName)
{
  if (!FileName)
    return false;

  csString FileToFind;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
      VfsNode *symlinkNode 
        = ParentVFS->GetDirectoryNode(MappedPaths[i].name);
      if (symlinkNode)
      {
        if (symlinkNode->Delete(FileName))
          return true;
      }
    }
    else
    {
      FileToFind.Clear();
      FileToFind = MappedPaths[i].name;

      if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
        FileToFind.Append(CS_PATH_SEPARATOR);

      FileToFind.Append(FileName);
      iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);
      
      if (!fs)
        continue;

      if (fs->Delete((const char *) FileToFind))
        return true;
    }
  }

  // File was not found
  return false;
}

// Get a list of files in this node
void VfsNode::FindFiles(const char *Mask, iStringArray *FileList)
{
  if (!FileList)
    return;

  int counter = 0;
  // First add names of all subdirectories
  for (size_t i = 0; i < SubDirectories.Length(); i++)
  {
    if (FileList->Find(
      (const char *) ExtractFileName(SubDirectories[i]->VirtualPathname))
      == csArrayItemNotFound)
    {
      FileList->Push(
      (const char *) ExtractFileName(SubDirectories[i]->VirtualPathname));
    }
    counter++;
  }

  // Add names from the paths
  for (size_t j = 0; j < MappedPaths.Length(); j++)
  {
    // Add names in symbolic links
    if (MappedPaths[j].symlink)
    {
      VfsNode *symlinkNode = 
        ParentVFS->GetDirectoryNode((const char *) MappedPaths[j].name);
      if (symlinkNode)
      {
        symlinkNode->FindFiles(Mask, FileList);
      }
    }
    // Add names from FileSystems
    else
    {
      iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[j].pluginIndex);
      if (!fs)
        continue;

      fs->GetFilenames(MappedPaths[j].name, Mask, FileList);
    }
  }
}

bool VfsNode::GetFileTime (const char *FileName, csFileTime &oTime) const
{
  if (!FileName)
    return false;

  csString FileToFind;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
      VfsNode *symlinkNode = 
        ParentVFS->GetDirectoryNode(MappedPaths[i].name);
      if (symlinkNode)
      {
        if (symlinkNode->GetFileTime(FileName, oTime))
          return true;
      }
    }
    else
    {
       FileToFind.Clear();
       FileToFind = MappedPaths[i].name;


       if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
         FileToFind.Append(CS_PATH_SEPARATOR);

       FileToFind.Append(FileName);
       iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);

       if (!fs)
         continue;

       // Get the file time from the iFileSystem plugin
       if (fs->GetFileTime((const char *) FileToFind, oTime))
         return true;
    }
  }

  // File was not found
  return false;
}		

bool VfsNode::SetFileTime (const char *FileName, const csFileTime &iTime)
{
  if (!FileName)
    return false;

  csString FileToFind;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
      VfsNode *symlinkNode = 
        ParentVFS->GetDirectoryNode(MappedPaths[i].name);
      if (symlinkNode)
      {
        if (symlinkNode->SetFileTime(FileName, iTime))
          return true;
      }
    }
    else
    {
       FileToFind.Clear();
       FileToFind = MappedPaths[i].name;

       if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
         FileToFind.Append(CS_PATH_SEPARATOR);

       FileToFind.Append(FileName);
       iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);

       if (!fs)
         continue;

       // Set the file time with the iFileSystem plugin
       if (fs->SetFileTime((const char *) FileToFind, iTime))
         return true;
    }
  }

  // File was not found
  return false;
}

bool VfsNode::GetFileSize (const char *FileName, size_t &oSize)
{
  if (!FileName)
    return false;

  csString FileToFind;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].symlink)
    {
      VfsNode *symlinkNode = 
        ParentVFS->GetDirectoryNode(MappedPaths[i].name);
      if (symlinkNode)
      {
        if (symlinkNode->GetFileSize(FileName, oSize))
          return true;
      }
    }
    else
    {
       FileToFind.Clear();
       FileToFind = MappedPaths[i].name;

       if (FileToFind[FileToFind.Length() -1] != CS_PATH_SEPARATOR)
         FileToFind.Append(CS_PATH_SEPARATOR);

       FileToFind.Append(FileName);
       iFileSystem *fs = ParentVFS->GetPlugin(MappedPaths[i].pluginIndex);

       if (!fs)
         continue;

       // Get the file size from the iFileSystem plugin
       if (fs->GetFileSize((const char *) FileToFind, oSize))
         return true;
    }
  }

  // File was not found
  return false;
}

bool VfsNode::IsMapped(const char *Path)
{
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (MappedPaths[i].name.Compare(Path))
    {
      return true;
    }
  }
  return false;
}

// Write the mounts of this node to the config file
void VfsNode::WriteMounts(iConfigFile  *config)
{
  // Write all real paths to a string
  csString realPaths;
  for (size_t i = 0; i < MappedPaths.Length(); i++)
  {
    if (!MappedPaths[i].symlink)
    {
      realPaths.Append(MappedPaths[i].name);
      if (i != MappedPaths.Length() -1)
      {
        realPaths.Append(", ");
      }
    }
  }

  // Write the information into the config file
  csString s("VFS.Mount.");
  s += VirtualPathname;
  config->SetStr (s, realPaths);

  // Write all the mounts of the subdirectories
  for (size_t j = 0; j < SubDirectories.Length(); j++)
  {
    SubDirectories[j]->WriteMounts(config);
  }
}

// --------------------------------------------------------- VfsVector ---//

// Comparison method to compare nodes
int VfsVector::Compare (VfsNode* const& Item1, VfsNode* const& Item2)
{
  return strcmp (Item1->GetName(), Item2->GetName());
}


// ------------------------------------------------------------- csVFS ---//
SCF_IMPLEMENT_FACTORY (csVFS)

// csVFS contructor
csVFS::csVFS (iBase *iParent) :
  scfImplementationType(this, iParent),
  object_reg(0),
  RootNode(0),
  CwdNode(0),
  StaleCwd(0),
  auto_name_counter(0)
{
  // We need a recursive mutex.
  mutex = csMutex::Create (true); 

  // Always register native filesystem plugin at index 0;
  fsPlugins.Push(new csNativeFileSystem(this));

  // Register the archive plugin
  fsPlugins.Push(new csArchiveFileSystem(this));

  // Create the root node
  RootNode = new VfsNode("/", this, 0);

  // Set the current node to the root node
  CwdNode = RootNode;

#ifdef VFS_AUTOCONFIGURE
  AutoConfigPluginPtr = new AutoConfigPlugin();
#endif
}

// csVFS destructor
csVFS::~csVFS ()
{
  // Free memory used by VFS Nodes
  delete RootNode;
}

const char * csVFS::GetCwd () const
{ 
  // Get the name of the current directory
  return CwdNode->GetName(); 
}

bool csVFS::Initialize (iObjectRegistry* r)
{
  // TODO: complete initialize
  object_reg = r;

// Autoconfiguration
#ifdef VFS_AUTOCONFIGURE
  AutoConfigPluginPtr->Configure(this, r);
#endif

  // !! ADD vfs search path
  return true;
}

bool csVFS::LoadConfigFile(const char* VirtualPath, bool Mount)
{
  if (!VirtualPath)
    return false;

  bool const merge = !config.IsEmpty();

  // Load into the class config file
  bool ok = config.Load(VirtualPath, this, merge, false);

  if (Mount)
  {
    bool result = false;
    // Create a new temp config file to mount
    csConfigFile * tmpConfig = new csConfigFile;
    if (tmpConfig->Load(VirtualPath, this, false, false))
    {
      if (MountConfigFile(tmpConfig))
         result = true;
    }
    delete tmpConfig;
    return result;
  }

  return ok;
}

bool csVFS::MountConfigFile(csConfigFile* conf)
{
  if (conf == 0)
    conf = &config;

  csRef<iConfigIterator> iterator (conf->Enumerate ("VFS.Mount."));
  csStringArray realPaths;
  while (iterator->HasNext())
  {
    iterator->Next();
    csString vp = iterator->GetKey (true);
    csString rp = iterator->GetStr ();

     // Mount each entry
    realPaths.Empty();
    realPaths.SplitString(rp, ", ");
    for (size_t i= 0; i < realPaths.Length(); i++)
    {
      if (!realPaths[i] || strlen(realPaths[i]) == 0)
        continue;

      Mount(vp, realPaths[i]);
    }
  }
  return true;
}

bool csVFS::ChDir (const char *Path)
{
  // Must be a path
  if (!Path || strlen(Path) == 0)
    return false;

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(Path);
  
  if (fullPath.IsEmpty())
    fullPath = "/";

  // See if path exists
  VfsNode *newCwd = GetDirectoryNode(fullPath);
  
  // Assign new Cwd
  if (newCwd)
  {
    // Path exists
	  CwdNode = newCwd;

    // Free memory used by stale Cwd Node
    if (StaleCwd)
    {
      delete StaleCwd;
      StaleCwd = 0;
    }

	  return true;
  }

  // Try mount path
  csString strPath;
  newCwd = GetParentDirectoryNode((const char *) fullPath, true, true);

  if (!newCwd)
    return false;

  newCwd->GetRealPath((const char *) ExtractFileName(fullPath), strPath);
  
  // Try and mount path
  if (!Mount((const char *) fullPath, (const char *) strPath, 0, 0))
    return false;
  
  newCwd = GetDirectoryNode((const char *) fullPath);

  // mount succeeded
  if (newCwd)
  {
	  CwdNode = newCwd;
	  return true;
  }

  // failure to change directory
  return false;
}

void csVFS::PushDir (char const* Path)
{
	// Push the current directory onto the stack
	DirectoryStack.Push(CwdNode);

  if (Path)
  {
	  // Change the current directory
    if (!ChDir(Path))
      DirectoryStack.Pop();
  }
}

bool csVFS::PopDir ()
{
	// Check that the stack is not empty
	if (DirectoryStack.Length() < 1)
		return false;

  csScopedMutexLock lock (mutex);

	// Pop the stack and change the cwd
	CwdNode = DirectoryStack.Pop();

	return true;
}

csString csVFS::_ExpandPath (const char *Path) const
{
  csString ExpandedPath = Path;

  if (ExpandedPath.Length() == 0)
    return ExpandedPath;

    // Remove trailing slash
  if (ExpandedPath.GetAt(ExpandedPath.Length() - 1) == VFS_PATH_SEPARATOR)
    ExpandedPath.Truncate(ExpandedPath.Length() - 1);

  // Split string up
  csStringArray directories;
  directories.SplitString((const char*) ExpandedPath, "/");

  if (ExpandedPath.Length() == 0)
    ExpandedPath = "/";
  else
    ExpandedPath = "";

  size_t loop = 1;

  csScopedMutexLock lock (mutex);

  if (Path[0] != VFS_PATH_SEPARATOR)
  {
    if (strcmp((const char *) CwdNode->VirtualPathname, "/") != 0)
    {
      ExpandedPath = CwdNode->VirtualPathname;
    }
    loop = 0;
  }

  while(loop < directories.Length() && directories[loop])
  {
    // Parent directory
    if (strcmp(directories[loop], "..") == 0)
    {
      size_t newlen = ExpandedPath.FindLast(VFS_PATH_SEPARATOR);
      ExpandedPath.Truncate(newlen);
    }
    // Current directory
    else if (strcmp(directories[loop], ".") == 0)
    {
      // Do nothing
    }
    else if (strcmp(directories[loop], "~") == 0)
    {
      ExpandedPath = "/~/";
    }
    else
    {
      ExpandedPath.Append(VFS_PATH_SEPARATOR);
      ExpandedPath.Append(directories[loop]);
    }
    loop++;
  }

  return ExpandedPath;
}

csPtr<iDataBuffer> csVFS::ExpandPath (const char *Path, bool IsDir) const
{
  csString strXp = _ExpandPath(Path);
  char *xp = new char[strXp.Length() +1];
  strcpy(xp, (const char *) strXp);
  xp[strXp.Length()] = 0;

  return csPtr<iDataBuffer> (new csDataBuffer (xp, strlen (xp) + 1));
}

bool csVFS::Exists (const char *Path)
{
  if (!Path)
    return false;

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(Path);
  
  if (fullPath.IsEmpty())
    fullPath = "/";

  // Find the parent directory of the files we are checking
  VfsNode *node = GetClosestDirectoryNode(fullPath);

  if (!node)
	  return false;
 
  // Calculate the filename
  csString fName = fullPath.Slice(strlen(node->GetName()) + 1);

  // Check if the node contains the file
  return node->ContainsFile(fName);
}

csPtr<iStringArray> csVFS::FindFiles (const char *Path)
{
  // Array to store cosntents
  scfStringArray *fl = new scfStringArray;

  if (!Path)
  {
    csPtr<iStringArray> v(fl);
    return v;
  }

  csString fullPath = _ExpandPath(Path);
  
  if (fullPath.IsEmpty())
    fullPath = "/";

  csScopedMutexLock lock (mutex);

  // Get directory node
	VfsNode *node = GetDirectoryNode(fullPath);

  csString strMask;

	if (!node)
  {
    // Get parent directory node
    node = GetParentDirectoryNode(fullPath, false);
    if (!node)
    {
      // Does not exist
      csPtr<iStringArray> v(fl);
      return v;
    }
    strMask = ExtractFileName(fullPath);
  }

  if (strMask.Length() == 0)
  {
    strMask = "*";
  }
  // Get file from node
  node->FindFiles((const char *) strMask, fl);

  fl->Sort(true);

  csPtr<iStringArray> v(fl);
  return v;
}

csPtr<iFile> csVFS::Open (const char *FileName, int Mode)
{
  iFile *f = 0;

  if (!FileName)
  {
    // Return the results
    return csPtr<iFile> (f);
  }

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(FileName);
    
  if (fullPath.IsEmpty())
    fullPath = "/";

  // Get the closest parent directory node
  VfsNode *node = GetClosestDirectoryNode(fullPath);

  if (!node)
  {
    // Return the results
    return csPtr<iFile> (f);
  }

  csString newPath; 
  fullPath.SubString(newPath, strlen(node->GetName()) + 1);

  // Get the file pointer
  f = node->Open(newPath, Mode);

  // Check if a physical directory was created
  if (!newPath.Compare(ExtractFileName(fullPath)))
  {
    csString RealPath;

    // Mount the created directory
    if (node->GetRealPath(newPath, RealPath))
    {
      node = GetParentDirectoryNode(fullPath, true, false);
      Mount(node->GetName(), ExtractParentName(RealPath));
    }
  }

  // Return the results
  return csPtr<iFile> (f);
}
  
csPtr<iDataBuffer> csVFS::ReadFile (const char *FileName, bool nullterm)
{
  if (!FileName)
    return csPtr<iDataBuffer> (0);

  // Acquire Lock
  csScopedMutexLock lock (mutex);

  // Open File for reading
  csRef<iFile> F (Open (FileName, VFS_FILE_READ));

  if (!F)
    return csPtr<iDataBuffer> (0);

  // Get size
  size_t Size = F->GetSize();

  // Get Data
  csRef<iDataBuffer> data (F->GetAllData(nullterm));
  if (data)
  {
    return csPtr<iDataBuffer> (data);
  }

  char *buff = new char [Size + 1];
  if (!buff)
    return 0;

  // Make the file zero-terminated in the case we'll use it as an 
  // ASCIIZ string
  buff [Size] = 0;
  if (F->Read(buff, Size) != Size)
  {
   // delete [] buff;
    return csPtr<iDataBuffer> (0);
  }

  return csPtr<iDataBuffer> (new csDataBuffer (buff, Size));
}

bool csVFS::WriteFile (const char *FileName, const char *Data, size_t Size)
{
  // Acquire Lock
  csScopedMutexLock lock (mutex);

  // Open File for writing
  csRef<iFile> F (Open (FileName, VFS_FILE_WRITE));

  if (!F)
    return false;

  return (F->Write (Data, Size) == Size);
}

bool csVFS::DeleteFile (const char *FileName)
{
  if (!FileName)
    return false;

  csScopedMutexLock lock (mutex);

  // Get the parent directory node
  VfsNode *node = GetParentDirectoryNode(FileName, false);

  if (!node)
    return false;

  return node->Delete(FileName);	
}

bool csVFS::Sync ()
{
  // Acquire lock
  csScopedMutexLock lock (mutex);

  // Sync all iFileSystem plugins
  for (size_t i = 0; i < fsPlugins.Length(); i++)
    fsPlugins[i]->Sync();

	return true;	
}

bool csVFS::Mount (const char *VirtualPath, const char *RealPath)
{
  // Mount without priority and autodetect plugin
  return  Mount(VirtualPath, RealPath, 0, 0);
}

bool csVFS::Mount(const char *VirtualPath, const char *RealPath, 
                  int priority, size_t plugin)
{
  if (!VirtualPath || !RealPath)
	  return false;

  // Create the node for the mount
  csString tmp = _ExpandPath(VirtualPath);
  
  // Remove trailing slash
  if (tmp.GetAt(tmp.Length() - 1) == VFS_PATH_SEPARATOR)
      tmp.Truncate(tmp.Length() - 1);

  tmp.Append("/tmp");

  csScopedMutexLock lock (mutex);

  VfsNode *node;

  // Create the data structure
  struct MappedPathData pathData;
  pathData.name = ExpandRealPath(RealPath);

  pathData.priority = priority;
  pathData.pluginIndex = plugin;
  pathData.symlink = false;

  // Check if a plugin index was given
  if (plugin != 0)
  {
    node = GetParentDirectoryNode(tmp, true);

    if (!node)
    {
	    return false;
    }

    if (!node->IsMapped(pathData.name))
    {
       // Insert the mount into the node
      node->MappedPaths.InsertSorted(pathData, CompareMappedPaths);
    }
    return true;
  }

  // Real Path is a directory
  if (isDirectory(pathData.name))
  {
    node = GetParentDirectoryNode(tmp, true);
    if (!node)
    {
	    return false;
    }

    if (!node->IsMapped(pathData.name))
    {
      // Insert the mount into the node
      node->MappedPaths.InsertSorted(pathData, CompareMappedPaths);
    }
	  return true;
  }

  csString mountName = "";
  // Find the correct plugin for handling the RealPath
  for (size_t i = 1; i < fsPlugins.Length(); i++)
  {
    // Start from 1 because we don't want to 
    // check the NativeFileSystem first
    if(fsPlugins[i]->CanHandleMount(pathData.name))
    {
      node = GetParentDirectoryNode(tmp, true);
      
      if (!node)
      {
	      return false;
      }

      pathData.pluginIndex = i;

      if (!node->IsMapped(pathData.name))
      {
        // Insert the mount into the node
        node->MappedPaths.InsertSorted(pathData, CompareMappedPaths);
      }
      return true;
    }
  }

  // Try remove the node if it was made
  Unmount(VirtualPath, 0);

  // RealPath is not valid
  return false;	
}

bool csVFS::Unmount (const char *VirtualPath, const char *RealPath)
{
  if (!VirtualPath)
	  return false;

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(VirtualPath);
  
  if (fullPath.IsEmpty())
    fullPath = "/";

  // Get the appropriate node
  VfsNode *node = GetDirectoryNode(fullPath);

  // Node does not exists
  if (!node)
    return false;

  StaleCwd = 0;
  if (strncmp(CwdNode->GetName(), fullPath, strlen(fullPath)) == 0)
  {
     StaleCwd = new VfsNode(CwdNode->GetName(), this, 0);
  }
  
  if (!RealPath)
  {
    // Delete the node
    if (node->ParentNode)
    {
      if (node->MappedPaths.Length() > 0)
      {
        node->ParentNode->RemoveSubDirectory(node);
      }
      else
        return false;
    }
    else
    {
      node->MappedPaths.Empty();
    }
    if (StaleCwd)
      CwdNode = StaleCwd;
    return true;
  }

  // Find the path within the node
  for (size_t i = 0; i < node->MappedPaths.Length(); i++)
  {
    if (node->MappedPaths[i].name.Compare(ExpandRealPath(RealPath)))
    {
       if (!node->MappedPaths.DeleteIndex(i))
         return false;

       if (StaleCwd)
          CwdNode = StaleCwd;

       return true;
    }
  }
  
  // Real Path was not found within the node
  return false;	
}
  
csRef<iStringArray> csVFS::MountRoot (const char *VirtualPath)
{
  // Create the output array
  scfStringArray* outv = new scfStringArray;

  // Get a lock on the file system
  csScopedMutexLock lock (mutex);

  // Fin dht esystem roots
  csRef<iStringArray> roots = csInstallationPathsHelper::FindSystemRoots();
  size_t i;
  size_t n = roots->Length ();

  // Go through each root
  VfsNode *node;
  for (i = 0 ; i < n ; i++)
  {
    char const* t = roots->Get(i);

    csString s(t);
    size_t const slen = s.Length();
    char c = '\0';

    // Create the directory name
    csString vfs_dir;
    vfs_dir << _ExpandPath(VirtualPath) << '/';

    for (size_t j = 0; j < slen; j++)
    {
      c = s.GetAt(j);
      if (c == '_' || c == '-' || isalnum(c))
     vfs_dir << (char)tolower(c);
    }

    // Create the real path name
    csString real_dir(s);

// Remove trailing slash
#if!(defined (CS_PLATFORM_DOS) || defined (CS_PLATFORM_WIN32))
    if (slen > 0 && (c = real_dir.GetAt(slen - 1)) == '/' || c == '\\')
      real_dir.Truncate(slen - 1);

#else
    if (slen > 4 && (c = real_dir.GetAt(slen - 1)) == '/' || c == '\\')
      real_dir.Truncate(slen - 1);
#endif

    outv->Push (vfs_dir);

    vfs_dir.Append("/tmp");

    // Create the node
    node = GetParentDirectoryNode(vfs_dir, true);

    if (!node)
    {
      // Error occurred
      continue;
    }

    // Create the data structure
    struct MappedPathData pathData;
    pathData.name = ExpandRealPath(real_dir);
    pathData.priority = 0;
    pathData.pluginIndex = 0;
    pathData.symlink = false;

    // Insert the mapped path
    node->MappedPaths.InsertSorted(pathData, CompareMappedPaths);
  }

  // Return the array
  csRef<iStringArray> v(outv);
  outv->DecRef ();
  return v;
}

bool csVFS::SaveMounts (const char *FileName)
{
  // Get a lock on the VFS
  csScopedMutexLock lock (mutex);
  
  // Create a config file
  csConfigFile * tmpConfig = new csConfigFile;

  // Write all the mount to the config file
  RootNode->WriteMounts(tmpConfig);

  // Set the config file
  config = (*tmpConfig);

  // Save the config file
  return tmpConfig->Save (FileName);
}


bool csVFS::LoadMountsFromFile (iConfigFile* file)
{
  // Flag to indicate success
  bool success = true;

  /// Get iterator to all mount sections
  csRef<iConfigIterator> iter = file->Enumerate ("VFS.Mount.");
  while (iter->HasNext ())
  {
	iter->Next();
    const char *rpath = iter->GetKey (true);
    const char *vpath = iter->GetStr ();
    // Mount the path
    if (!Mount (rpath, vpath)) 
    {
      csPrintfErr("VFS_WARNING: cannot mount \"%s\" to \"%s\"\n", rpath,vpath);
      // Failure...
      success = false;
    }
  }
  // Return the result
  return success;
}

bool csVFS::Clean()
{
  // Delete all mapped paths;
  RootNode->MappedPaths.DeleteAll();
  // Delete all subdirectories of the root node;
  return RootNode->RemoveSubDirectory(0);
}

bool csVFS::TryChDirAuto(const char *Path, const char *FileName)
{
  // Acquire lock
  csScopedMutexLock lock (mutex);

  // Check if the VFS Path exists
  VfsNode *node = GetDirectoryNode(Path);

  if (!node)
    return false;

  // Check for filename
  if (FileName)
  {
     if (node->ContainsFile(FileName))
     {
        CwdNode = node;
        return true;
     }
  }
  else
  {
    CwdNode = node;
    return true;
  }

  return false;
}

bool csVFS::ChDirAuto (const char* path, const csStringArray* paths,
					   const char* vfspath, const char* filename)
{
  csString fullPath = _ExpandPath(path);
  
  if (fullPath.IsEmpty())
    fullPath = VFS_PATH_SEPARATOR;

  // Check if the VFS Path exists
  if (TryChDirAuto(fullPath, filename))
    return true;

  // Check the paths
  if (paths)
  {
    for (size_t i = 0; i < paths->Length(); i++)
    {
      csString testpath = paths->Get(i);
      if (testpath.GetAt(testpath.Length() - 1) == VFS_PATH_SEPARATOR)
      {
        testpath.Truncate(testpath.Length() -1);
      }

      testpath.Append(fullPath);

      // Test if the path exists
      if (TryChDirAuto(testpath, filename))
        return true;
    }
  }
  
  csString vpath;

  // Mount

  if (vfspath)
    vpath = vfspath;
  else
  {
    vpath.Format("/tmp/__automount%d__", auto_name_counter);
    auto_name_counter++;
  }

  if (!Mount(vpath, path))
    return false;

  // Try change directory
  return ChDir(vpath);
}

bool csVFS::GetFileTime (const char *FileName, csFileTime &oTime)
{
  if (!FileName)
    return false;

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(FileName);
  
  if (fullPath.IsEmpty())
    return false;

  VfsNode *node = GetParentDirectoryNode(fullPath, false);

  if (!node)
    return false;

  return node->GetFileTime(fullPath, oTime);
}		

bool csVFS::SetFileTime (const char *FileName, const csFileTime &iTime)
{
  if (!FileName)
    return false;

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(FileName);
  
  if (fullPath.IsEmpty())
    return false;

  VfsNode *node = GetParentDirectoryNode(fullPath, false);

  if (!node)
    return false;

  return node->SetFileTime(fullPath, iTime);
}

bool csVFS::GetFileSize (const char *FileName, size_t &oSize)
{
  if (!FileName)
    return false;

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(FileName);
  
  if (fullPath.IsEmpty())
    return false;

  VfsNode *node = GetParentDirectoryNode(fullPath, false);

  if (!node)
    return false;

  return node->GetFileSize(fullPath, oSize);
}

csPtr<iDataBuffer> csVFS::GetRealPath (const char *FileName)
{
  if (!FileName)
    csPtr<iDataBuffer> (0);

  csScopedMutexLock lock (mutex);

  csString fullPath = _ExpandPath(FileName);
  
  if (fullPath.IsEmpty())
    fullPath = "/";

  VfsNode *node = GetDirectoryNode(fullPath);

  csString path;
  if (!node)
  {
    node = GetParentDirectoryNode(fullPath, false);
    if (!node)
    {
      return csPtr<iDataBuffer> (0);
    }
  
    // Try get the real path
    if (!node->GetRealPath(ExtractFileName(fullPath), path))
    {
      return csPtr<iDataBuffer> (0);
    }
  }
  else
  {
    if (node->MappedPaths.IsEmpty())
      return csPtr<iDataBuffer> (0);

    path = node->MappedPaths[0].name;
  }

  return csPtr<iDataBuffer> (new csDataBuffer (
    csStrNew (path), path.Length() + 1));
}

// Get virtual mount paths
csRef<iStringArray> csVFS::GetMounts ()
{
  // An array to hold the mounts
  scfStringArray* mounts = new scfStringArray;

  csScopedMutexLock lock (mutex);

  // Get all mounted nodes
  RootNode->GetMounts(mounts);  

  csRef<iStringArray> m (mounts);
  mounts->DecRef ();

  return m;

}

csRef<iStringArray> csVFS::GetRealMountPaths (const char *VirtualPath)
{
  scfStringArray *rmounts = new scfStringArray;

  if (VirtualPath)
  {
    csScopedMutexLock lock (mutex);

    csString fullPath = _ExpandPath(VirtualPath);
  
    if (fullPath.IsEmpty())
      fullPath = "/";

    // Find the node
    VfsNode *node = GetDirectoryNode(fullPath);
  
    if (node)
    {
      // Add only the real paths
      for (size_t i = 0; i < node->MappedPaths.Length(); i++)
      {
        if (!node->MappedPaths[i].symlink)
	        rmounts->Push(node->MappedPaths[i].name);
      }
    }
  }

  // return the results
  csRef<iStringArray> r (rmounts);
  rmounts->DecRef();

  return r;
}

// Register a filesystem plugin
size_t csVFS::RegisterPlugin(iFileSystem *FileSystem)
{
  csScopedMutexLock lock (mutex);

	// Add the plugin
	return fsPlugins.PushSmart(FileSystem);
}

// Create or add a symbolic link
bool csVFS::SymbolicLink(const char *Target, const char *Link, int priority)
{
  csScopedMutexLock lock (mutex);

  VfsNode *node = GetDirectoryNode(Link);

  // Node does not exist
  if (!node)
  {
    // Create the node
	  csString tmp = _ExpandPath(Link);

    // Remove trailing slash
    if (tmp.GetAt(tmp.Length() - 1) == VFS_PATH_SEPARATOR)
      tmp.Truncate(tmp.Length() - 1);

	  tmp.Append("/tmp");
	  node = GetParentDirectoryNode(tmp, true);
    if (!node)
      return false;
  }

  struct MappedPathData mp;
  mp.name = _ExpandPath(Target);
  mp.pluginIndex = 0;
  mp.priority = priority;
  mp.symlink = true;

  // Add the target to the symlinks of the directory
  node->MappedPaths.InsertSorted(mp, CompareMappedPaths);

  return true;
}

// Get a node corresponding to a directory
VfsNode* csVFS::GetDirectoryNode(const char *Path)
{
  csString strPath = Path;

  // Remove trailing slash
  if (strPath.GetAt(strPath.Length() - 1) == VFS_PATH_SEPARATOR)
    strPath.Truncate(strPath.Length() - 1);

  strPath.Append("/tmp");

  return GetParentDirectoryNode(strPath, false);
}

// Get a node corresponding to a parent directory
VfsNode* csVFS::GetClosestDirectoryNode(const char *path)
{
  if (strlen(path) == 0)
	  return 0;

  if (strlen(path) == 1 && path[0] == '/')
    return RootNode;

  csString strPath = path;

  // Remove trailing slash
  if (strPath.GetAt(strPath.Length() - 1) == VFS_PATH_SEPARATOR)
    strPath.Truncate(strPath.Length() - 1);

  csStringArray directories;
  directories.SplitString(strPath, "/");

  VfsNode * node;
  VfsNode * tmpNode;
  size_t counter = 0;
  csString currentDir;

  // Absolute Path
  if (path[0] == VFS_PATH_SEPARATOR)
  {
    node = RootNode;
	  counter = 1;
	  currentDir = "";
  }
  // Relative Path
  else
  {
    node = CwdNode;
	  if (strcmp(node->GetName(), "/") != 0)
    {
	    currentDir = node->GetName();
    }
    else
    {
      node = RootNode;
	    currentDir = "";
    }
  }

  bool exists = true;
  while(exists && counter < directories.Length() - 1)
  {
    currentDir.Append(VFS_PATH_SEPARATOR);
	  currentDir.Append(directories[counter]);

    // Try find the node
	  tmpNode = node->FindNode(currentDir);
	  if (!tmpNode)
	  {
      exists = false;
      continue;
	  }
	  node = tmpNode;
	  counter++;
  }

  return node;
}

// Get a node corresponding to a parent directory
VfsNode* csVFS::GetParentDirectoryNode(const char *path, bool create, bool mount)
{
  if (strlen(path) == 0)
	  return 0;

  if (strlen(path) == 1 && path[0] == '/')
    return RootNode;

  csString strPath = path;

  // Remove trailing slash
  if (strPath.GetAt(strPath.Length() - 1) == VFS_PATH_SEPARATOR)
    strPath.Truncate(strPath.Length() - 1);

  csStringArray directories;
  directories.SplitString(strPath, "/");

  VfsNode * node;
  VfsNode * tmpNode;
  size_t counter = 0;
  csString currentDir;

  // Absolute Path
  if (path[0] == VFS_PATH_SEPARATOR)
  {
    node = RootNode;
	  counter = 1;
	  currentDir = "";
  }
  // Relative Path
  else
  {
    node = CwdNode;
	  if (strcmp(node->GetName(), "/") != 0)
    {
	    currentDir = node->GetName();
    }
    else
    {
      node = RootNode;
	    currentDir = "";
    }
  }

  while(counter < directories.Length() - 1)
  {
    currentDir.Append(VFS_PATH_SEPARATOR);
	  currentDir.Append(directories[counter]);

    // Try find the node
	  tmpNode = node->FindNode(currentDir);
	  if (!tmpNode)
	  {

	    if (!create)
	    {
        // Node does not exist
        return 0;
	    }
      if (mount)
      {
        csString realPath;
        node->GetRealPath(ExtractFileName(currentDir), realPath);
        if (!Mount(currentDir, realPath))
          return 0;
        tmpNode = GetDirectoryNode(currentDir);
      }
      else
      {
        // Create the node
        tmpNode = new VfsNode(currentDir, this, node);
      }
	  }
	  node = tmpNode;
	  counter++;
  }

  return node;
}

#ifndef _S_IFDIR
#define _S_IFDIR S_IFDIR
#endif

bool csVFS::isDirectory(const char *path)
{
  if (!path)
    return false;

  // stat the path
  struct stat stats;
  if (stat (path, &stats) != 0)
    return false;

  // path is a directory
  return ((stats.st_mode & _S_IFDIR) != 0);
}

iFileSystem* csVFS::GetPlugin(size_t index) const
{
  if (index >= fsPlugins.Length())
    return 0;

  // Return a pointer to the plugin
  return fsPlugins.Get(index);
}

// ----------------------------------------------------- AutoConfig-------- //
csVFS::AutoConfigPlugin::~AutoConfigPlugin()
{

}

/*
 * This method will attempt to auto-configure the VFS.
 * It will mount the relevant resource directories, and map them to symbolic
 * links within the root of the vfs.
 */
bool csVFS::AutoConfigPlugin::Configure(iVFS *vfs, 
                                        iObjectRegistry *object_reg)
{
  if (!vfs)
    return false;

  /// The install directory
  csString InstallDirectory;

  /// The user directory
  csString UserDirectory;

  /// The application directory
  csString AppDirectory;

  /// The resource directory
  csString ResourceDirectory;

  static const char* vfsSubdirs[] = {
    "etc/" CS_PACKAGE_NAME,
    "etc", 
    "",
    0};

  csPathsList configPaths;
  const char* crystalconfig = getenv("CRYSTAL_CONFIG");
  if (crystalconfig)
    configPaths.AddUniqueExpanded (crystalconfig);

  csPathsList* basedirs = 
    csInstallationPathsHelper::GetPlatformInstallationPaths();
  configPaths.AddUniqueExpanded (*basedirs * csPathsList  (vfsSubdirs));
  delete basedirs;
  
  configPaths.AddUniqueExpanded (".");

  InstallDirectory = csGetConfigPath();
  configPaths.AddUniqueExpanded (InstallDirectory);

  csStringArray Directories;

  // Try and mount the user directory
  if (vfs->Mount("/mnt/user", csGetPlatformConfigPath("", false)))
	{
    // Add it to the directory list
		//Directories.Push("/mnt/user");
	}


  // Get the command line parser plugin
  csRef<iCommandLineParser> cmdline =
    CS_QUERY_REGISTRY (object_reg, iCommandLineParser);
  if (cmdline)
  {
    ResourceDirectory = cmdline->GetResourceDir();
    ((csVFS*)vfs)->ResourceDirectory = ResourceDirectory;
    AppDirectory = cmdline->GetAppDir();
    ((csVFS*)vfs)->AppDirectory = AppDirectory;
  
    // Try and mount the resource directory
    if (vfs->Mount("/mnt/resource", ResourceDirectory))
	  {
      // Add it to the directory list
		  Directories.Push("/mnt/resource");
	  }
    // Try and mount the app directory
    if (vfs->Mount("/mnt/app", AppDirectory))
	  {
      // Add it to the directory list
		  Directories.Push("/mnt/app");
	  }
  }

  bool mountedInstall = false;
  for (size_t i = 0; i < configPaths.Length(); i++)
  {
    if (vfs->Mount("/mnt/install", configPaths[i].path))
    {
      if (!mountedInstall)
      {
      		//Directories.Push("/mnt/install");
          mountedInstall = true;
      }
    }
  }

  vfs->LoadConfigFile("/mnt/install/data/config/vfs.cfg", false);

  ((csVFS*)vfs)->InstallDirectory = InstallDirectory;
  
  ((csVFS*)vfs)->MountConfigFile();

  csStringArray mountedRealPaths;

  // An iterator to iterate through directories
  csArray<const char *,csStringArrayElementHandler>::Iterator i(
    Directories.GetIterator());

  csRef<iStringArray> Contents;
  csString DirectoryName;
  csString VirtualDirectoryName;
  csString RealDirectoryName;
  csRef<iStringArray> RealPaths;
  csString SymLinkName;

  // Go through each directory in the array
  while (i.HasNext())
  {
    DirectoryName = i.Next();

    // Get the contents of the directory
	  Contents = vfs->FindFiles(DirectoryName);

    // Go through each file in the directory contents
    for (size_t ctCounter = 0; ctCounter < Contents->Length(); 
      ctCounter++)
	  {
      // Flag to indicate if symbolic link was created
      bool createdSymLink = false;
      bool isDirectory = true;

      // Create the name for the virtual symbolic link
      VirtualDirectoryName = DirectoryName;
      VirtualDirectoryName.Append(VFS_PATH_SEPARATOR);
      VirtualDirectoryName.Append(Contents->Get(ctCounter));

      // Get the real paths mapped to the directory
      RealPaths = vfs->GetRealMountPaths(DirectoryName);
      for (size_t rpCounter = 0; rpCounter < RealPaths->Length(); 
        rpCounter++)
	    {
        // Construct the real path name for the directory contents
        RealDirectoryName = RealPaths->Get(rpCounter);
        RealDirectoryName.Append(CS_PATH_SEPARATOR);
        RealDirectoryName.Append(Contents->Get(ctCounter));

        if (mountedRealPaths.Contains(RealDirectoryName) != csArrayItemNotFound)
          continue;

        mountedRealPaths.PushSmart(RealDirectoryName);

        // Try and mount the directory contents
        if (vfs->Mount(VirtualDirectoryName, RealDirectoryName))
        {
          struct stat stats;
          if ((stat (RealDirectoryName, &stats) == 0) 
                && (stats.st_mode & _S_IFDIR) == 0)
            isDirectory = false;

          // If it was successfully mounted, try create the symbolic link
          if (!createdSymLink)
          {
            // Construct the symbolic link name
            SymLinkName = VFS_PATH_SEPARATOR;
            SymLinkName.Append(Contents->Get(ctCounter));
            if (!isDirectory)
            {
              size_t period = SymLinkName.Find(".");
              SymLinkName.Truncate(period);
            }
            // Create the symbolic link
            vfs->SymbolicLink(VirtualDirectoryName, SymLinkName);

            // Toggle the flag
            createdSymLink = true;
          }
          // Recursively add directories (takes very long if many deep 
          //directory hierarchy)
          //Directories.Push(VirtualDirectoryName);
        }
      }
	  }
  }
  return true;
}

csString csVFS::ExpandRealPath(char const *Path)
{
  csString result = _ExpandRealPath(Path);
  if (result.GetAt(result.Length() - 1) == CS_PATH_SEPARATOR)
    result.Truncate(result.Length() - 1);
  return result;
}

// Expand a Path
csString csVFS::_ExpandRealPath(char const *Path)
{
  csString dst;
  char *src_start = csStrNew(Path);
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
        {
          dst << _ExpandRealPath(alternative);
        }
      }
      else
      {
	      // @@@ FIXME: protect against circular references
        dst << _ExpandRealPath(value);
        if (strcmp(InstallDirectory, value) == 0 || strcmp(ResourceDirectory, value) == 0 || strcmp(AppDirectory, value) == 0)
          dst << CS_PATH_SEPARATOR;
      }
    } /* endif */
    else
      dst << *src++;
  } /* endif */
  delete[] src_start;

  return dst;
}

const char * csVFS::GetValue(const char *VarName)
{
  // Look in environment first
  const char *value = getenv (VarName);
  if (value)
    return value;

  iConfigFile *Config = &config;

  // Now look in "VFS.Unix" section, for example
  csString Keyname;
  Keyname << "VFS." CS_PLATFORM_NAME "." << VarName;
  value = Config->GetStr (Keyname, 0);
  if (value)
    return value;

  // Now look in "VFS.Alias" section for alias section name
  const char *alias = Config->GetStr ("VFS.Alias." CS_PLATFORM_NAME, 0);
  // If there is one, look into that section too
  if (alias)
  {
    Keyname.Clear();
    Keyname << alias << '.' << VarName;
    value = Config->GetStr (Keyname, 0);
  }
  if (value)
    return value;

  // Handle predefined variables here so that user
  // can override them in config file or environment

  // check for OS-specific predefined variables
  value = csCheckPlatformVFSVar(VarName);
  if (value)
    return value;

  static char path_separator [] = {VFS_PATH_SEPARATOR, 0};
  if (strcmp (VarName, path_separator) == 0)	// Path separator variable?
  {
    static char path_sep [] = {CS_PATH_SEPARATOR, 0};
    return path_sep;
  }

  if (strcmp (VarName, "*") == 0) // Resource directory?
    return ResourceDirectory;
   
  if (strcmp (VarName, "^") == 0) // Application or Cocoa wrapper directory?
    return AppDirectory;
       
  if (strcmp (VarName, "@") == 0) // Installation directory?
    return InstallDirectory;

  return 0;
}

} CS_PLUGIN_NAMESPACE_END(vfs)
