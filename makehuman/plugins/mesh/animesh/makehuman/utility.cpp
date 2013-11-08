/*
    Copyright (C) 2012-2013 by Anthony Legrand
    Copyright (C) 2013 by Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

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
#include "character.h"

CS_PLUGIN_NAMESPACE_BEGIN (MakeHuman)
{

/*-------------------------------------------------------------------------*
 * Utility functions
 *-------------------------------------------------------------------------*/

bool MakeHumanManager::ReportError (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (objectRegistry));
  if (rep)
    rep->ReportV (CS_REPORTER_SEVERITY_ERROR,
		  "crystalspace.mesh.animesh.makehuman",
		  msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
  return false;
}

bool MakeHumanManager::ReportWarning (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (objectRegistry));
  if (rep)
    rep->ReportV (CS_REPORTER_SEVERITY_WARNING,
		  "crystalspace.mesh.animesh.makehuman",
		  msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
  return false;
}

bool MakeHumanManager::ReportInfo (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (objectRegistry));
  if (rep)
    rep->ReportV (CS_REPORTER_SEVERITY_NOTIFY,
		  "crystalspace.mesh.animesh.makehuman",
		  msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
  return false;
}

bool MakeHumanCharacter::ReportError (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (manager->objectRegistry));
  if (rep)
    rep->ReportV (CS_REPORTER_SEVERITY_ERROR,
		  "crystalspace.mesh.animesh.makehuman",
		  msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
  return false;
}

bool MakeHumanCharacter::ReportWarning (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (manager->objectRegistry));
  if (rep)
    rep->ReportV (CS_REPORTER_SEVERITY_WARNING,
		  "crystalspace.mesh.animesh.makehuman",
		  msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
  return false;
}

csPtr<iFile> MakeHumanManager::OpenFile (const char* filename, const char* vfsPath)
{
  // TODO: clean this code
  size_t index;
  csString filenameVFS;
  csString filenamestr (filename);
  if (!vfs->Exists (filename))
  {
    index = filenamestr.FindLast ('\\');
    if (index == (size_t) -1)
      index = filenamestr.FindLast ('/');

    if (index != (size_t) -1)
    {
      csString path = filenamestr.Slice (0, index + 1);
      filenameVFS = vfsPath;

      if (!vfs->Mount (filenameVFS.GetData (), path.GetData ()))
      {
	ReportError ( "Mount failed on path %s", path.GetData ());
        return csPtr<iFile> (nullptr);
      }

      filenameVFS += filenamestr.Slice (index + 1);
    }
    else
      filenameVFS = csString (vfsPath).Append (filenamestr);
  }
  else
    filenameVFS = filenamestr;

  if (!vfs->Exists (filenameVFS.GetData ()))
  {
    ReportError ( "File %s does not exist", filename);
    return csPtr<iFile> (nullptr);
  }

  csRef<iFile> file = vfs->Open (filenameVFS.GetData (), VFS_FILE_READ);
  if (!file.IsValid ())
    ReportError ( "Could not open file %s", filename);

  return csPtr<iFile> (file);
}

bool MakeHumanManager::ParseLine  (iFile* file, char* buf, size_t nbytes)
{
  if (!file)
    return false;

  char c = '\n';
  while (c == '\n' || c == '\r')
    if (!file->Read (&c, 1))
      break;

  if (file->AtEOF ())
    return false;

  char* p = buf;
  const char* plim = p + nbytes - 1;
  while (p < plim)
  {
    if (c == '\n' || c == '\r')
      break;
    *p++ = c;
    if (!file->Read (&c, 1))
      break;
  }

  *p = '\0';
  return true;
}

bool MakeHumanManager::ParseWord (const char* txt, char* buf, size_t& start)
{
  int index = start;

  while (txt[index] == ' ')
  {
    start++;
    index++;
  }

  while (txt[index] != ' '
	 && txt[index] != '\0')
  {
    buf[index - start] = txt[index];
    index++;
  }

  buf[index - start] = '\0';
  bool found = index - start != 0;
  start = index;
  return found;
}


csString MakeHumanCharacter::Description () const
{
  csString txt;
  txt += "=======================================\n";
  txt += "== MakeHuman character parameters:\n";
  txt += "== -------------------------------\n";

/*
  for (csHash<float, csString>::ConstGlobalIterator it = parameters.GetIterator (); it.HasNext (); )
  {
    csString parameter;
    float value = it.Next (parameter);
    txt += "== - ";
    txt += parameter + ": ";
    txt += value;
    txt += "\n";
  }
*/
  txt += "== - gender: ";
  txt += GetParameter ("macro", "gender");
  txt += "\n";

  txt += "== - age: ";
  txt += GetParameter ("macro", "age");
  txt += "\n";

  txt += "== - african: ";
  txt += GetParameter ("macro", "african");
  txt += "\n";

  txt += "== - asian: ";
  txt += GetParameter ("macro", "asian");
  txt += "\n";

  txt += "== - tone: ";
  txt += GetParameter ("macro", "tone");
  txt += "\n";

  txt += "== - weight: ";
  txt += GetParameter ("macro", "weight");
  txt += "\n";

  txt += "== - breastFirmness: ";
  txt += GetParameter ("gender", "breastFirmness");
  txt += "\n";

  txt += "== - breastSize: ";
  txt += GetParameter ("gender", "breastSize");
  txt += "\n";

  for (csHash<float, csString>::ConstGlobalIterator it = parameters.GetIterator (); it.HasNext (); )
  {
    csString parameter;
    float value = it.Next (parameter);

    // TODO: use FindParameter
    if (!manager->parameters[parameter]) continue;

    txt += "== - ";
    txt += parameter + ": ";
    txt += value;
    txt += "\n";
  }

  txt += "=======================================\n";
  return txt;
}

}
CS_PLUGIN_NAMESPACE_END (MakeHuman)
