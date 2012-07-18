/*
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

#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <authz.h>
#include <sddl.h>

#include "cssysdef.h"
#include "csutil/filepermission.h"

// implementation using Win32 ACL permission

// anonymous namespace for helper function forward declarations
namespace
{
  /*
   * Get Windows security descriptor of given file/directory/etc. Returned
   * security descriptor contains owner, group, and DACL information.
   */
  PSECURITY_DESCRIPTOR GetSecurityDescriptor (const wchar_t *filename);

  /*
   * Get effective ACL rights of given trustee
   * - takes authz    (Authz resource manager handle. Use
   *                     AuthzInitializeResourceManager ())
   *         security (pointer to Windows security descriptor)
   *         userSid  (pointer to SID of trustee that needs rights checked)
   *         oRights  (ACCESS_MASK variable to store results)
   * - returns true if succeeded; false otherwise
   */
  bool GetEffectiveRights (AUTHZ_RESOURCE_MANAGER_HANDLE authz,
                           PSECURITY_DESCRIPTOR security,
                           PSID userSid,
                           ACCESS_MASK &oRights);

  // Convert fixed-size (3) ACCESS_MASK array to octal-formatted Unix permission
  uint32 AccessMaskToUnixPermission (ACCESS_MASK *rights);

  // Convert octal-formatted Unix-style permission to three ACCESS_MASKs
  void PermissionToAccessMask (uint32 perm,
                               ACCESS_MASK &user,
                               ACCESS_MASK &group,
                               ACCESS_MASK &others);

  /* Get emulated Unix permission from given file
   * - takes filename (wide-character filename string)
   *         oPerm    (variable to store permission)
   * - returns 0 if succeeded; otherwise returns errno-equivalent value
   */
  int GetUnixPermission (wchar_t *filename, uint32 &oPerm);

  // Converts GetLastError () status to errno-equivalent value.
  // If the error code is unknown (or cannot be handled), -1 is returned.
  int LastErrorToErrno ();
} // anonymous namespace

namespace CS
{
  namespace Platform
  {
    int GetFilePermission (const char *filename, uint32 &oPermission)
    {
      if (!filename) // filename must not be null!
        return EINVAL;

      // UTF-16 max length (in wchar_t) <= UTF-8 length (in char)
      size_t pathLen = strlen (filename) + 1;
      CS_ALLOC_STACK_ARRAY (wchar_t, path, pathLen); // allocate buffer

      // convert to wchar_t
      csUnicodeTransform::UTF8toWC (path, pathLen, filename);

      uint32 perm = 0;
      int result = GetUnixPermission (path, perm);
      if (result == 0)
      {
        // success! return by call-by-ref
        oPermission = perm;
      }

      return result;
    }

    int SetFilePermission (const char *filename, uint32 permission)
    {
      if (!filename) // filename must not be null!
        return EINVAL;

      ACCESS_MASK user, group, others;
      PSID ownerSid = NULL, groupSid = NULL, everyoneSid = NULL;
      BOOL ownerDefaulted = FALSE, groupDefaulted = FALSE;

      // UTF-16 max length (in wchar_t) <= UTF-8 length (in char)
      size_t pathLen = strlen (path) + 1;
      // allocate buffer
      CS_ALLOC_STACK_ARRAY (wchar_t, wPath, pathLen);
      // convert to wchar_t
      csUnicodeTransform::UTF8toWC (wPath, pathLen, path);
  
      // get security descriptor
      PSECURITY_DESCRIPTOR security = GetSecurityDescriptor (wPath);

      if (!security)
        return LastErrorToErrno ();

      // retrieve SID information for trustees
      GetSecurityDescriptorOwner (security, &ownerSid, &ownerDefaulted);
      GetSecurityDescriptorGroup (security, &groupSid, &groupDefaulted);

      // others (everyone) SID is well-known, built-in SID
      DWORD everyoneSidSize = SECURITY_MAX_SID_SIZE;
      // allocate memory
      everyoneSid = LocalAlloc (LMEM_FIXED, everyoneSidSize);
      // create SID for Everyone
      CreateWellKnownSid (WinWorldSid, NULL, everyoneSid, &everyoneSidSize);

      // list of permissions that should not be denied
      const ACCESS_MASK mustHave = READ_CONTROL | SYNCHRONIZE;
      // find corresponding access mask
      PermissionToAccessMask (permission, user, group, others);
      // now derive actually required permission set
      ACCESS_MASK userDeny    = ~user & (group | others) & ~mustHave,
                  userGrant   = user,
                  groupDeny   = ~group & others & ~mustHave,
                  groupGrant  = group,
                  othersGrant = others;
  
      // create corresponding DACL entry
      // note that we cannot use canonical ACEs
      DWORD allowedAceSize = sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD);
      DWORD deniedAceSize  = sizeof (ACCESS_DENIED_ACE)  - sizeof (DWORD);
      DWORD sidLengthTotal = GetLengthSid (ownerSid)
                           + GetLengthSid (groupSid)
                           + everyoneSidSize;
      DWORD daclSize = sizeof(ACL) + allowedAceSize * 3 + deniedAceSize * 2
                                   + sidLengthTotal;
  
      // make sure it fits DWORD boundary (4-byte/32-bit)
      if (daclSize & 0x03)
      {
        // if either last 2 bits are set,
        daclSize &= 0xFFFFFFFC; // remove the bits
        daclSize += 0x04;       // add 4 (round up)
      }

      // allocate memory for DACL
      PACL dacl = (PACL)LocalAlloc (LMEM_FIXED, daclSize);

      if (!dacl)
      {
        // DACL memory allocation failed
        int error = LastErrorToErrno ();
        LocalFree (everyoneSid);
        LocalFree (security);
        return error;
      }

      // initialize empty DACL
      if (!InitializeAcl (dacl, daclSize, ACL_REVISION))
      {
        // ACL initialization failed.
        int error = LastErrorToErrno ();
        LocalFree (dacl);
        LocalFree (everyoneSid);
        LocalFree (security);
        return error;
      }

      // Add entries to ACL in pre-defined order
      AddAccessDeniedAce  (dacl, ACL_REVISION, userDeny,    ownerSid);
      AddAccessAllowedAce (dacl, ACL_REVISION, userGrant,   ownerSid);
      AddAccessDeniedAce  (dacl, ACL_REVISION, groupDeny,   groupSid);
      AddAccessAllowedAce (dacl, ACL_REVISION, groupGrant,  groupSid);
      AddAccessAllowedAce (dacl, ACL_REVISION, othersGrant, everyoneSid);

      int result = 0;

      // set security information of file
      if (SetNamedSecurityInfoW (wPath,            // path of the file
          SE_FILE_OBJECT,                      // given name is a file
          DACL_SECURITY_INFORMATION            // set DACL of this object
        | PROTECTED_DACL_SECURITY_INFORMATION, // prevent DACL inheritance
          NULL,                                // SID of new owner (unused)
          NULL,                                // SID of new group (unused)
          dacl,                                // new DACL
          NULL))                               // new SACL (unused)
      {
        // nonzero return value => error
        result = LastErrorToErrno ();
      }

      // free memory
      LocalFree (dacl);
      LocalFree (everyoneSid);
      LocalFree (security);

      return result;
    }
  } // namespace Platform
} // namespace CS

// anonymous namespace - helper function body
namespace
{
  PSECURITY_DESCRIPTOR GetSecurityDescriptor (const wchar_t *filename)
  {
    void *security = NULL;
    BOOL present, defaulted;
    PACL dacl = NULL;
    DWORD lengthNeeded = 0;
    SECURITY_INFORMATION request = DACL_SECURITY_INFORMATION
                                 | OWNER_SECURITY_INFORMATION
                                 | GROUP_SECURITY_INFORMATION;

    // calculate required buffer size
    GetFileSecurityW (filename, request, NULL, 0, &lengthNeeded);
    // do we have a proper size?
    if(!lengthNeeded)
      return NULL;

    // allocate memory
    size_t size = lengthNeeded;
    security = LocalAlloc (LMEM_FIXED, size);

    // if memory allocation fails, return NULL
    if (!security)
      return NULL;

    // use GetFileSecurity() to retrieve DACL information
    if (GetFileSecurityW (filename, request, security, size, &lengthNeeded))
    {
      if (lengthNeeded <= size && IsValidSecurityDescriptor (security))
      {
        // success!
        return security;
      }
    }

    // failed..
    LocalFree (security);
    return NULL;
  }

  bool GetEffectiveRights (AUTHZ_RESOURCE_MANAGER_HANDLE authz,
                           PSECURITY_DESCRIPTOR security,
                           PSID userSid,
                           ACCESS_MASK &oRights)
  {  
    LUID unused = {0};   // for parameter
    AUTHZ_CLIENT_CONTEXT_HANDLE context = NULL;  

    // if SID is invalid, nothing to do here
    if (!userSid || !IsValidSid (userSid))
      return false;

    // try initialize context
    if (AuthzInitializeContextFromSid (0,
                                       userSid,
                                       authz,
                                       NULL,
                                       unused,
                                       NULL,
                                       &context))
    {
      bool result = false;

      AUTHZ_ACCESS_REQUEST accessRequest = {0, };
      AUTHZ_ACCESS_REPLY   accessReply   = {0, };
      DWORD error = 0;
      ACCESS_MASK grantedAccess = 0;

      // setup access request structure
      accessRequest.DesiredAccess = MAXIMUM_ALLOWED;
      accessRequest.ObjectTypeList = NULL;
      accessRequest.ObjectTypeListLength = 0;
      accessRequest.PrincipalSelfSid = NULL;    
      accessRequest.OptionalArguments = NULL;
      // setup access reply structure
      accessReply.GrantedAccessMask = &grantedAccess;
      accessReply.Error = &error;
      accessReply.ResultListLength = 1;

      // test effective permissions
      if (AuthzAccessCheck (0,
                            context,
                            &accessRequest,
                            NULL,
                            security,
                            NULL,
                            0,
                            &accessReply,
                            NULL))
      {
        // return results
        oRights = grantedAccess;
        result = true;
      }

      // free used context
      AuthzFreeContext (context);
      return result;
    }

    return false;
  }

  uint32 AccessMaskToUnixPermission (ACCESS_MASK *rights)
  {
    uint32 result = 0;

    // rights array must be of fixed size: 3 (user, group, others)
    for (size_t i = 0; i < 3; ++i)
    {
      uint32 value = 0;
      // test against each flag

      // read
      value |= (rights[i] & FILE_GENERIC_READ) ? 4 : 0;
      // write
      value |= (rights[i] & FILE_GENERIC_WRITE) ? 2 : 0;
      // execute
      value |= (rights[i] & FILE_GENERIC_EXECUTE) ? 1 : 0;
      // add outcome of each iteration to result.
      // User   i=2 -> bits [6,9)
      // Group  i=1 -> bits [3,6)
      // Others i=0 -> bits [0,3)
      result |= (value << (3 * i)); 
    }

    return result;
  }

  void PermissionToAccessMask (uint32 perm,
                               ACCESS_MASK &user,
                               ACCESS_MASK &group,
                               ACCESS_MASK &others)
  {
    // we only do direct translations
    user   = ((perm & 0400) != 0) * FILE_GENERIC_READ
           | ((perm & 0200) != 0) * FILE_GENERIC_WRITE
           | ((perm & 0100) != 0) * FILE_GENERIC_EXECUTE;
    group  = ((perm & 0040) != 0) * FILE_GENERIC_READ
           | ((perm & 0020) != 0) * FILE_GENERIC_WRITE
           | ((perm & 0010) != 0) * FILE_GENERIC_EXECUTE;
    others = ((perm & 0004) != 0) * FILE_GENERIC_READ
           | ((perm & 0002) != 0) * FILE_GENERIC_WRITE
           | ((perm & 0001) != 0) * FILE_GENERIC_EXECUTE;
  }

  int GetUnixPermission (wchar_t * filename, uint32 &oPerm)
  {
    AUTHZ_RESOURCE_MANAGER_HANDLE authz;
    PSID ownerSid, groupSid, othersSid = NULL;
    BOOL ownerDefaulted, groupDefaulted;

    // Retrieve owner, group, and DACL information
    PSECURITY_DESCRIPTOR security = GetSecurityDescriptor (filename);

    // if failed to get security descriptor, return appropriate code
    if (!security)
      return LastErrorToErrno (); 
  
    // Get owner and group SID
    GetSecurityDescriptorOwner (security, &ownerSid, &ownerDefaulted);
    GetSecurityDescriptorGroup (security, &groupSid, &groupDefaulted);

    // others (everyone) SID is well-known, built-in SID
    DWORD othersSidSize = SECURITY_MAX_SID_SIZE;
    // allocate memory
    othersSid = LocalAlloc (LMEM_FIXED, othersSidSize);
    // create SID for Everyone
    CreateWellKnownSid (WinWorldSid, NULL, othersSid, &othersSidSize);

    int result = 0; // function return code (errno)

    // try initialize authz resource manager
    if (AuthzInitializeResourceManager (AUTHZ_RM_FLAG_NO_AUDIT,
                                        NULL, NULL, NULL, NULL, &authz))
    {
      // ACCESS_MASK for each entity
      ACCESS_MASK rights [3] = {0, 0, 0};
      // test for others (everyone)
      GetEffectiveRights (authz, security, othersSid, rights[0]);
      // test for group
      GetEffectiveRights (authz, security, groupSid, rights[1]);
      // test for owner
      GetEffectiveRights (authz, security, ownerSid, rights[2]);

      // free resource manager
      AuthzFreeResourceManager (authz);
      // return results
      oPerm = AccessMaskToUnixPermission (rights);
    }
    else
      result = LastErrorToErrno ();

    // release memory
    LocalFree (othersSid);
    LocalFree (security);
  
    return result;
  }

  int LastErrorToErrno ()
  {
    switch (GetLastError())
    {
    case ERROR_SUCCESS:
      // no error
      return 0;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      return ENOENT;
    case ERROR_INVALID_HANDLE:
      return EINVAL;
    case ERROR_ACCESS_DENIED:
      return EACCES;
    case ERROR_TOO_MANY_OPEN_FILES:
      return EMFILE;
    default:
      // unknown error.
      return -1; // this is not a valid errno value
    }
  }
} // anonymous namespace
