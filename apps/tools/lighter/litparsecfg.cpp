/*
    Copyright (C) 2004 by Jorrit Tyberghein

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
#include "iutil/document.h"
#include "csutil/xmltiny.h"
#include "litparsecfg.h"
#include "lighter.h"

litConfigParser::litConfigParser (Lighter* lighter, iObjectRegistry* object_reg)
{
  litConfigParser::lighter = lighter;
  litConfigParser::object_reg = object_reg;
  init_token_table (xmltokens);
}

bool litConfigParser::ParseMulti (iDocumentNode* multi_node,
    litMeshSelectChildren* meshsel)
{
  csRef<iDocumentNodeIterator> it = multi_node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csRef<litMeshSelect> a;
    if (!ParseMeshSelect (child, a))
      return false;
    meshsel->AddMeshSelect (a);
  }
  return true;
}

bool litConfigParser::ParseMeshSelect (iDocumentNode* meshsel_node,
    csRef<litMeshSelect>& meshsel)
{
  csRef<iDocumentNodeIterator> it = meshsel_node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_AND:
	meshsel.AttachNew (new litMeshSelectAnd ());
	if (!ParseMulti (child, (litMeshSelectChildren*)(litMeshSelect*)meshsel))
	  return false;
	return true;
      case XMLTOKEN_OR:
	meshsel.AttachNew (new litMeshSelectOr ());
	if (!ParseMulti (child, (litMeshSelectChildren*)(litMeshSelect*)meshsel))
	  return false;
	return true;
      case XMLTOKEN_NOT:
	{
	  csRef<litMeshSelect> a;
	  if (!ParseMeshSelect (child, a))
	    return false;
	  meshsel.AttachNew (new litMeshSelectNot (a));
	}
	return true;
      case XMLTOKEN_ALL:
	meshsel.AttachNew (new litMeshSelectAll ());
	return true;
      case XMLTOKEN_STATICPOS:
	meshsel.AttachNew (new litMeshSelectByMOFlags (CS_MESH_STATICPOS,
	      CS_MESH_STATICPOS));
	return true;
      case XMLTOKEN_STATICSHAPE:
	meshsel.AttachNew (new litMeshSelectByMOFlags (CS_MESH_STATICSHAPE,
	      CS_MESH_STATICSHAPE));
	return true;
      case XMLTOKEN_STATIC:
	meshsel.AttachNew (new litMeshSelectByMOFlags (
	      CS_MESH_STATICSHAPE|CS_MESH_STATICPOS,
	      CS_MESH_STATICSHAPE|CS_MESH_STATICPOS));
	return true;
      case XMLTOKEN_NAME:
	meshsel.AttachNew (new litMeshSelectByName (child->GetContentsValue ()));
	return true;
      case XMLTOKEN_REGEXP:
	meshsel.AttachNew (new litMeshSelectByNameRE (child->GetContentsValue ()));
	return true;
      case XMLTOKEN_TYPE:
	meshsel.AttachNew (new litMeshSelectByType (child->GetContentsValue ()));
	return true;
      default:
        return lighter->Report ("Unknown token <%s> in mesh selector!", value);
    }
  }
  return true;
}

bool litConfigParser::ParseLighter (iDocumentNode* lighter_node,
  	csRef<litMeshSelect>& casters_selector,
	csRef<litMeshSelect>& receivers_selector)
{
  csRef<iDocumentNodeIterator> it = lighter_node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_SELECT_CASTERS:
        if (!ParseMeshSelect (child, casters_selector))
	  return false;
        break;
      case XMLTOKEN_SELECT_RECEIVERS:
        if (!ParseMeshSelect (child, receivers_selector))
	  return false;
        break;
      default:
        return lighter->Report ("Unknown token <%s> in <lighter>!", value);
    }
  }
  return true;
}

bool litConfigParser::ParseConfigFile (const char* vfsfile,
  	csRef<litMeshSelect>& casters_selector,
	csRef<litMeshSelect>& receivers_selector)
{
  csRef<iFile> databuff = lighter->vfs->Open (vfsfile, VFS_FILE_READ);
  if (!databuff || !databuff->GetSize ())
    return lighter->Report ("Could not open config file '%s' on VFS!",
    	vfsfile);

  csRef<iDocumentSystem> docsys =
      CS_QUERY_REGISTRY (object_reg, iDocumentSystem);
  if (!docsys) docsys = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());
  csRef<iDocument> doc = docsys->CreateDocument ();
  const char* error = doc->Parse (databuff);
  if (error != 0)
    return lighter->Report ("Document system error for file '%s': %s!",
    	vfsfile, error);
  csRef<iDocumentNode> lighter_node = doc->GetRoot ()->GetNode ("lighter");
  if (!lighter_node)
    return lighter->Report ("Config file '%s' does not start with <lighter>!",
    	vfsfile);

  return ParseLighter (lighter_node, casters_selector, receivers_selector);
}

