/*
    Debugging tools.
    Copyright (C) 2001 by Jorrit Tyberghein

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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cssysdef.h"
#include "csutil/scf.h"
#include "csutil/util.h"
#include "csutil/debug.h"
#include "iutil/objreg.h"

//-----------------------------------------------------------------------------

struct csDGEL
{
  void* object;
  bool used;
  char* description;
  char* file;
  int linenr;
  int num_parents;
  csDGEL** parents;
  int num_children;
  csDGEL** children;
  bool marker;

  void AddChild (csDGEL* child)
  {
    if (!children)
      children = new csDGEL*[1];
    else
    {
      csDGEL** new_children = new csDGEL*[num_children+1];
      memcpy (new_children, children, sizeof (csDGEL*)*num_children);
      delete[] children;
      children = new_children;
    }
    children[num_children++] = child;
  }
  void RemoveChild (csDGEL* child)
  {
    if (!children) return;
    if (num_children == 1)
    {
      if (child == children[0])
      {
        delete[] children;
	children = NULL;
	num_children = 0;
      }
      return;
    }
    // Allocate the array one too big for the new elements because
    // it is possible that we don't find the element to remove.
    csDGEL** new_children = new csDGEL*[num_children];
    int i, j = 0;
    for (i = 0 ; i < num_children ; i++)
    {
      if (child != children[i])
	new_children[j++] = children[i];
    }
    num_children = j;
    delete[] children;
    children = new_children;
  }
  void AddParent (csDGEL* parent)
  {
    if (!parents)
      parents = new csDGEL*[1];
    else
    {
      csDGEL** new_parents = new csDGEL*[num_parents+1];
      memcpy (new_parents, parents, sizeof (csDGEL*)*num_parents);
      delete[] parents;
      parents = new_parents;
    }
    parents[num_parents++] = parent;
  }
  void RemoveParent (csDGEL* parent)
  {
    if (!parents) return;
    if (num_parents == 1)
    {
      if (parent == parents[0])
      {
        delete[] parents;
	parents = NULL;
	num_parents = 0;
      }
      return;
    }
    // Allocate the array one too big for the new elements because
    // it is possible that we don't find the element to remove.
    csDGEL** new_parents = new csDGEL*[num_parents];
    int i, j = 0;
    for (i = 0 ; i < num_parents ; i++)
    {
      if (parent != parents[i])
	new_parents[j++] = parents[i];
    }
    num_parents = j;
    delete[] parents;
    parents = new_parents;
  }
};

class csDebugGraph : public iBase
{
public:
  int num_els;
  int max_els;
  csDGEL* els;
  bool exact;

  csDebugGraph ()
  {
    SCF_CONSTRUCT_IBASE (NULL);
    num_els = 0;
    max_els = 100;
    els = new csDGEL [max_els];
    exact = true;
  }
  virtual ~csDebugGraph ()
  {
    Clear ();
  }
  void Clear ()
  {
    int i;
    for (i = 0 ; i < num_els ; i++)
    {
      delete[] els[i].description;
      delete[] els[i].file;
      delete[] els[i].parents;
      delete[] els[i].children;
    }
    delete[] els;
    num_els = 0;
    max_els = 100;
    els = new csDGEL [max_els];
  }

  csDGEL* AddEl ()
  {
    if (num_els >= max_els)
    {
      // First check if there isn't a free element we can use before
      // extending the array. This only happens if we are not in
      // exact mode.
      if (!exact)
      {
        int i;
        for (i = 0 ; i < num_els ; i++)
        {
          if (!els[i].used)
	  {
	    els[i].used = true;
	    return &els[i];
	  }
        }
      }
      max_els += 100;
      csDGEL* new_els = new csDGEL [max_els];
      memcpy (new_els, els, sizeof (csDGEL) * num_els);
      delete[] els;
      els = new_els;
    }

    csDGEL* el = &els[num_els];
    el->object = NULL;
    el->used = true;
    el->description = NULL;
    el->file = NULL;
    el->num_parents = 0;
    el->parents = NULL;
    el->num_children = 0;
    el->children = NULL;
    num_els++;
    return el;
  }

  csDGEL* FindEl (void* object)
  {
    int i;
    for (i = 0 ; i < num_els ; i++)
    {
      if (els[i].object == object) return &els[i];
    }
    return NULL;
  }

  SCF_DECLARE_IBASE;
};

SCF_IMPLEMENT_IBASE (csDebugGraph)
SCF_IMPLEMENT_IBASE_END

static csDebugGraph* SetupDebugGraph (iObjectRegistry* object_reg)
{
  iBase* idg = CS_QUERY_REGISTRY_TAG (object_reg, "__Debug_Graph__");
  if (!idg)
  {
    idg = new csDebugGraph ();
    object_reg->Register (idg, "__Debug_Graph__");
  }
  idg->DecRef ();
  return (csDebugGraph*)idg;
}

//-----------------------------------------------------------------------------

void csDebuggingGraph::GraphMode (iObjectRegistry* object_reg, bool exact)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  dg->exact = exact;
}

void csDebuggingGraph::AddObject (iObjectRegistry* object_reg,
	void* object, char* file, int linenr,
  	char* description, ...)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  csDGEL* el = dg->FindEl (object);
  CS_ASSERT (el == NULL);	// Object should not occur!
  el = dg->AddEl ();

  if (description)
  {
    char buf[1000];
    va_list arg;
    va_start (arg, description);
    vsprintf (buf, description, arg);
    va_end (arg);
    el->description = csStrNew (buf);
  }
  else el->description = NULL;

  el->object = object;
  el->file = file ? csStrNew (file) : NULL;
  el->linenr = linenr;
}

void csDebuggingGraph::RemoveObject (iObjectRegistry* object_reg,
	void* object, char* file, int linenr)
{
  (void)file;
  (void)linenr;
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  csDGEL* el = dg->FindEl (object);
  if (!el)
  {
    printf ("Suspicious! Cannot find element!\n");
    fflush (stdout);
    return;
  }
  if (!dg->exact)
  {
    // Unlink from parents and children.
    int i;
    for (i = 0 ; i < el->num_parents ; i++)
    {
      el->parents[i]->RemoveChild (el);
    }
    for (i = 0 ; i < el->num_children ; i++)
    {
      el->children[i]->RemoveParent (el);
    }
  }
  el->used = false;
}

void csDebuggingGraph::AddParent (iObjectRegistry* object_reg,
	void* child, void* parent)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  csDGEL* p_el = dg->FindEl (parent);
  csDGEL* c_el = dg->FindEl (child);
  CS_ASSERT (p_el != NULL);
  CS_ASSERT (c_el != NULL);
  c_el->AddParent (p_el);
}

void csDebuggingGraph::AddChild (iObjectRegistry* object_reg,
	void* parent, void* child)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  csDGEL* p_el = dg->FindEl (parent);
  csDGEL* c_el = dg->FindEl (child);
  CS_ASSERT (p_el != NULL);
  CS_ASSERT (c_el != NULL);
  p_el->AddChild (c_el);
}

void csDebuggingGraph::RemoveParent (iObjectRegistry* object_reg,
	void* child, void* parent)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  csDGEL* p_el = dg->FindEl (parent);
  csDGEL* c_el = dg->FindEl (child);
  CS_ASSERT (p_el != NULL);
  CS_ASSERT (c_el != NULL);
  c_el->RemoveParent (p_el);
}

void csDebuggingGraph::RemoveChild (iObjectRegistry* object_reg,
	void* parent, void* child)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  csDGEL* p_el = dg->FindEl (parent);
  csDGEL* c_el = dg->FindEl (child);
  CS_ASSERT (p_el != NULL);
  CS_ASSERT (c_el != NULL);
  p_el->RemoveChild (c_el);
}

void csDebuggingGraph::Clear (iObjectRegistry* object_reg)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  dg->Clear ();
}

void csDebuggingGraph::Dump (iObjectRegistry* object_reg)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);

  csDGEL* els = dg->els;
  // First mark all elements as unused and count the number
  // of elements we have.
  int i, cnt = 0;
  for (i = 0 ; i < dg->num_els ; i++)
  {
    if (els[i].object)
    {
      cnt++;
      els[i].marker = false;
    }
    else
      els[i].marker = true;
  }

  printf ("====================================================\n");
  printf ("Total number of objects in graph: %d\n", cnt);

  // Find the first unmarked object and dump it.
  i = 0;
  while (i < dg->num_els)
  {
    if (!els[i].marker)
    {
      Dump (object_reg, els[i].object, true, false);
      i = 0;	// Restart scan.
      printf ("----------------------------------------------------\n");
    }
    else i++;
  }
  fflush (stdout);
}

static void DumpSubTree (int indent, const char* type, csDGEL* el)
{
  char spaces[1000];
  int ind = indent;
  if (ind > 999) ind = 999;
  char* sp = spaces;
  while (ind >= 10)
  {
    strcpy (sp, "          ");
    sp += 10;
    ind -= 10;
  }
  while (ind >= 1)
  {
    *sp++ = ' ';
    ind--;
  }
  *sp = 0;

  if (el->marker || *type == 'P' || !el->used)
  {
    if (*type != 'P') el->marker = true;
    printf ("%s%s %p%s\n", spaces, type, el->object,
    	el->used ? "" : " (BAD LINK!)");
  }
  else
  {
    el->marker = true;
    printf ("%s%s %p %s (%s,%d) #p=%d #c=%d\n",
    	spaces, type, el->object, el->description,
    	el->file, el->linenr, el->num_parents, el->num_children);
    int i;
    for (i = 0 ; i < el->num_parents ; i++)
    {
      DumpSubTree (indent+2, "P", el->parents[i]);
    }
    for (i = 0 ; i < el->num_children ; i++)
    {
      DumpSubTree (indent+2, "C", el->children[i]);
    }
  }
  fflush (stdout);
}

static int compare_el (const void* vel1, const void* vel2)
{
  csDGEL* el1 = *(csDGEL**)vel1;
  csDGEL* el2 = *(csDGEL**)vel2;
  if (el1->num_parents < el2->num_parents) return -1;
  else if (el1->num_parents > el2->num_parents) return 1;
  else return 0;
}

void csDebuggingGraph::Dump (iObjectRegistry* object_reg, void* object,
	bool find_root, bool reset_mark)
{
  csDebugGraph* dg = SetupDebugGraph (object_reg);
  int i;
  if (reset_mark)
  {
    // First mark all elements as unused.
    csDGEL* els = dg->els;
    for (i = 0 ; i < dg->num_els ; i++)
    {
      if (els[i].used) els[i].marker = false;
      else els[i].marker = true;
    }
  }

  csDGEL* el = dg->FindEl (object);

  // First copy all elements that belong to this sub-graph
  // to a local array.
  csDGEL** local_els = new csDGEL* [dg->num_els];
  int done = 0, num = 0;
  local_els[num++] = el; el->marker = true;
  while (done < num)
  {
    csDGEL* lel = local_els[done++];
    for (i = 0 ; i < lel->num_parents ; i++)
      if (!lel->parents[i]->marker)
      {
        local_els[num++] = lel->parents[i];
	lel->parents[i]->marker = true;
      }
    for (i = 0 ; i < lel->num_children ; i++)
      if (!lel->children[i]->marker)
      {
        local_els[num++] = lel->children[i];
	lel->children[i]->marker = true;
      }
  }

  // Now mark all elements as unused again.
  for (i = 0 ; i < num ; i++)
    local_els[i]->marker = false;

  // Sort all elements based on the number of parents.
  // This means that 'root' like elements will come first in the
  // array.
  qsort (local_els, num, sizeof (csDGEL*), compare_el);

  // Now dump all parents here until all are marked.
  for (i = 0 ; i < num ; i++)
  {
    if (!local_els[i]->used)
    {
      local_els[i]->marker = true;
    }
    else if (!local_els[i]->marker)
    {
      DumpSubTree (0, "R", local_els[i]);
    }
  }

  delete[] local_els;
}

//-----------------------------------------------------------------------------

