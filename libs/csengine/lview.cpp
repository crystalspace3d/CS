/*
    Copyright (C) 2000-2001 by Andrew Zabolotny
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

#include "cssysdef.h"
#include "csengine/lview.h"
#include "csengine/engine.h"
#include "csengine/polygon.h"
#include "csgeom/polyclip.h"
#include "ivideo/graph3d.h"
#include "igeom/clip2d.h"
#include "iengine/camera.h"
#include "iengine/sector.h"

IMPLEMENT_IBASE (csFrustumView)
  IMPLEMENTS_INTERFACE (iFrustumView)
IMPLEMENT_IBASE_END

csFrustumView::csFrustumView () :
	node_func (NULL),
	poly_func (NULL),
	curve_func (NULL),
	things_shadow (false),
	dynamic (false),
	callback (NULL),
	callback_data (NULL),
	ctxt (NULL)
{
  CONSTRUCT_IBASE (NULL);
  ctxt = new csFrustumContext ();
  csShadowBlockList* sbl = new csShadowBlockList ();
  ctxt->SetShadows ((iShadowBlockList*)sbl, false);
}

#if 0
csFrustumView::csFrustumView (const csFrustumView &iCopy)
{
  // hehe. kind of trick.
  memcpy (this, &iCopy, sizeof (csFrustumView));
  //@@@@@@@@@@@@@@@@@
  CONSTRUCT_IBASE (NULL);
  // Leave cleanup actions alone to original copy.
  ctxt = new csFrustumContext ();
  memcpy (ctxt, iCopy.GetFrustumContext (), sizeof (csFrustumContext));
  ctxt->SetCleanup (NULL);
  ctxt->SetShadows (iCopy.GetFrustumContext ()->GetShadows (), true);
}
#endif

csFrustumView::~csFrustumView ()
{
  ctxt->CallCleanups ((iFrustumView*)this);
  if (ctxt->GetLightFrustum ()) ctxt->GetLightFrustum ()->DecRef ();
  if (!ctxt->IsShared ()) ctxt->GetShadows ()->DecRef ();
  delete ctxt;
}

void csFrustumView::StartNewShadowBlock ()
{
  if (!ctxt->IsShared ()) ctxt->GetShadows ()->DecRef ();
  ctxt->SetShadows ((iShadowBlockList*)(new csShadowBlockList ()), false);
}

void csFrustumView::CreateFrustumContext ()
{
  csFrustumContext* old_ctxt = ctxt;
  // @@@ Use a pool for frustum contexts?
  // A pool would work very well here since we have limited recusion depth.

  // Leave cleanup actions alone to original copy.
  ctxt = new csFrustumContext ();
  *ctxt = *old_ctxt;
  ctxt->SetCleanup (NULL);
  ctxt->SetShadows (old_ctxt->GetShadows (), true);
}

void csFrustumView::RestoreFrustumContext (csFrustumContext* original)
{
  csFrustumContext* old_ctxt = ctxt;
  ctxt = original;
  old_ctxt->CallCleanups ((iFrustumView*)this);
  //@@@ HANDLING OF LightFrustum
  if (old_ctxt->GetLightFrustum ()) old_ctxt->GetLightFrustum ()->DecRef ();
  if (!old_ctxt->IsShared ()) old_ctxt->GetShadows ()->DecRef ();
  delete old_ctxt;
}

//---------------------------------------------------------------------------

IMPLEMENT_IBASE (csShadowBlock)
  IMPLEMENTS_INTERFACE (iShadowBlock)
IMPLEMENT_IBASE_END

csShadowBlock::csShadowBlock (iSector* sect, int draw_busy,
	int max_shadows, int delta) : shadows (max_shadows, delta)
{
  CONSTRUCT_IBASE (NULL);
  sector = sect;
  draw_busy = draw_busy;
}

csShadowBlock::csShadowBlock (int max_shadows, int delta) :
	shadows (max_shadows, delta)
{
  CONSTRUCT_IBASE (NULL);
  sector = NULL;
  draw_busy = -1;
}

csShadowBlock::~csShadowBlock ()
{
  DeleteShadows ();
}

void csShadowBlock::AddRelevantShadows (csShadowBlock* source,
    	csTransform* trans)
{
  csShadowIterator* shadow_it = source->GetCsShadowIterator ();
  while (shadow_it->HasNext ())
  {
    csShadowFrustum* csf = (csShadowFrustum*)shadow_it->Next ();
    if (csf->IsRelevant ())
    {
      if (trans)
      {
	csShadowFrustum* copycsf = new csShadowFrustum (*csf);
	copycsf->Transform (trans);
	shadows.Push (copycsf);
      }
      else
      {
        csf->IncRef ();
        shadows.Push (csf);
      }
    }
  }
  delete shadow_it;
}

void csShadowBlock::AddRelevantShadows (iShadowBlock* source,
  	csTransform* trans)
{
  AddRelevantShadows ((csShadowBlock*)source, trans);
}

void csShadowBlock::AddRelevantShadows (csShadowBlockList* source)
{
  csShadowIterator* shadow_it = source->GetCsShadowIterator ();
  while (shadow_it->HasNext ())
  {
    csShadowFrustum* csf = (csShadowFrustum*)shadow_it->Next ();
    if (csf->IsRelevant ())
    {
      csf->IncRef ();
      shadows.Push (csf);
    }
  }
  delete shadow_it;
}

void csShadowBlock::AddRelevantShadows (iShadowBlockList* source)
{
  AddRelevantShadows ((csShadowBlockList*)source);
}

void csShadowBlock::AddAllShadows (csShadowBlockList* source)
{
  csShadowIterator* shadow_it = source->GetCsShadowIterator ();
  while (shadow_it->HasNext ())
  {
    csShadowFrustum* csf = (csShadowFrustum*)shadow_it->Next ();
    csf->IncRef ();
    shadows.Push (csf);
  }
  delete shadow_it;
}

void csShadowBlock::AddAllShadows (iShadowBlockList* source)
{
  AddAllShadows ((csShadowBlockList*)source);
}

void csShadowBlock::AddUniqueRelevantShadows (csShadowBlockList* source)
{
  int i;
  int cnt = shadows.Length ();

  csShadowIterator* shadow_it = source->GetCsShadowIterator ();
  while (shadow_it->HasNext ())
  {
    csShadowFrustum* csf = (csShadowFrustum*)shadow_it->Next ();
    if (csf->IsRelevant ())
    {
      for (i = 0 ; i < cnt ; i++)
	if (((csShadowFrustum*)shadows[i]) == csf)
	  break;
      if (i >= cnt)
      {
        csf->IncRef ();
        shadows.Push (csf);
      }
    }
  }
  delete shadow_it;
}

void csShadowBlock::AddUniqueRelevantShadows (iShadowBlockList* source)
{
  AddUniqueRelevantShadows ((csShadowBlockList*)source);
}

csFrustum* csShadowBlock::AddShadow (const csVector3& origin, void* userData,
    	int num_verts, csPlane3& backplane)
{
  csShadowFrustum* sf = new csShadowFrustum (origin, num_verts);
  sf->SetBackPlane (backplane);
  sf->SetUserData (userData);
  shadows.Push (sf);
  return (csFrustum*)sf;
}

void csShadowBlock::UnlinkShadow (int idx)
{
  csShadowFrustum* sf = (csShadowFrustum*)shadows[idx];
  sf->DecRef ();
  shadows.Delete (idx);
}

//---------------------------------------------------------------------------

IMPLEMENT_IBASE (csShadowBlockList)
  IMPLEMENTS_INTERFACE (iShadowBlockList)
IMPLEMENT_IBASE_END

csShadowBlockList::csShadowBlockList () : first (NULL), last (NULL)
{
  CONSTRUCT_IBASE (NULL);
}

iShadowBlock* csShadowBlockList::NewShadowBlock (iSector* sector,
	int draw_busy, int num_shadows)
{
  csShadowBlock* n = new csShadowBlock (sector, draw_busy, num_shadows);
  AppendShadowBlock (n);
  return (iShadowBlock*)n;
}

iShadowBlock* csShadowBlockList::NewShadowBlock ()
{
  csShadowBlock* n = new csShadowBlock ();
  AppendShadowBlock (n);
  return (iShadowBlock*)n;
}

//---------------------------------------------------------------------------

csShadowFrustum::csShadowFrustum (const csShadowFrustum& orig)
	: csFrustum ((const csFrustum&)orig)
{
  this->userData = orig.userData;
  this->relevant = orig.relevant;
}

//---------------------------------------------------------------------------

IMPLEMENT_IBASE (csShadowIterator)
  IMPLEMENTS_INTERFACE (iShadowIterator)
IMPLEMENT_IBASE_END

csShadowIterator::csShadowIterator (csShadowBlock* cur, bool onlycur,
	int dir)
{
  CONSTRUCT_IBASE (NULL);
  csShadowIterator::cur = cur;
  csShadowIterator::onlycur = onlycur;
  csShadowIterator::dir = dir;
  first_cur = cur;
  Reset ();
}

void csShadowIterator::Reset ()
{
  cur = first_cur;
  if (cur) cur_num = cur->GetNumShadows ();
  if (dir == 1) i = 0;
  else i = cur_num-1;
}

csFrustum* csShadowIterator::Next ()
{
  if (!cur) { cur_shad = NULL; return NULL; }
  csShadowFrustum* s;
  if (i >= 0 && i < cur_num)
    s = (csShadowFrustum*)cur->GetShadow (i);
  else
    s = NULL;
  i += dir;
  if (i < 0 || i >= cur_num)
  {
    if (onlycur) cur = NULL;
    else if (dir == 1) cur = cur->next;
    else cur = cur->prev;
    if (cur) cur_num = cur->GetNumShadows ();
    if (dir == 1) i = 0;
    else i = cur_num-1;
  }
  cur_shad = s;
  return s;
}

csShadowBlock* csShadowIterator::GetCsCurrentShadowBlock ()
{
  if (dir == -1)
  {
    if (i < cur_num-1) return cur;
    else if (onlycur || !cur->next) return NULL;
    else return cur->next;
  }
  else
  {
    if (i > 0) return cur;
    else if (onlycur || !cur->prev) return NULL;
    else return cur->prev;
  }
}

void csShadowIterator::DeleteCurrent ()
{
  if (dir == -1)
  {
    if (i < cur_num-1)
    {
      // Delete the previous element in the current list.
      cur->UnlinkShadow (i+1);
      cur_num--;
    }
    else if (onlycur || !cur || !cur->next)
    {
      // We are at the very first element of the iterator. Nothing to do.
      return;
    }
    else
    {
      // We are the first element of this list (last since we do reverse)
      // so we delete the last element (first) of the previous (next) list.
      cur->next->UnlinkShadow (0);
    }
  }
  else
  {
    if (i > 0)
    {
      // Delete the previous element in the current list.
      i--;
      cur->UnlinkShadow (i);
      cur_num--;
    }
    else if (onlycur || !cur || !cur->prev)
    {
      // We are at the very first element of the iterator. Nothing to do.
      return;
    }
    else
    {
      // We are the first element of this list so we delete the last
      // element of the previous list.
      cur->prev->UnlinkShadow (cur->prev->GetNumShadows ()-1);
    }
  }
}

iShadowBlock* csShadowIterator::GetCurrentShadowBlock ()
{
  return (iShadowBlock*)GetCsCurrentShadowBlock ();
}

iShadowBlock* csShadowIterator::GetNextShadowBlock ()
{
  return (iShadowBlock*)GetCsNextShadowBlock ();
}

