/*
  Crystal Space Windowing System: list box class
  Copyright (C) 1998 by Jorrit Tyberghein
  Written by Andrew Zabolotny <bit@eltech.ru>

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

#include "sysdef.h"
#include "cslistbx.h"
#include "cstimer.h"
#include "csscrbar.h"
#include "csmouse.h"
#include "csapp.h"
#include "csinput/csinput.h"

// Amount of space at left and at right of each listbox item
#define LISTBOXITEM_XSPACE              2
// Amount of space at top and at bottom of each listbox item
#define LISTBOXITEM_YSPACE              2

// Mouse scroll time interval in milliseconds
#define MOUSE_SCROLL_INTERVAL           100

// Horizontal large scrolling step
#define LISTBOX_HORIZONTAL_PAGESTEP     8

csListBoxItem::csListBoxItem (csComponent *iParent, char *iText, int iID,
  csListBoxItemStyle iStyle) : csComponent (iParent)
{
  state |= CSS_SELECTABLE | CSS_TRANSPARENT;
  ItemStyle = iStyle;
  ItemBitmap = NULL;
  DeleteBitmap = false;
  hOffset = vOffset = 0;
  id = iID;
  deltax = 0;
  SetPalette (CSPAL_LISTBOXITEM);
  SetText (iText);
  if (parent)
    SetColor (CSPAL_LISTBOXITEM_BACKGROUND, parent->GetColor (0));
}

csListBoxItem::~csListBoxItem ()
{
  if (ItemBitmap && DeleteBitmap)
    delete ItemBitmap;
}

void csListBoxItem::SuggestSize (int &w, int &h)
{
  int minh = 0;
  w = hOffset; h = vOffset;

  if (ItemBitmap)
  {
    w += ItemBitmap->Width () + 2;
    minh = h + ItemBitmap->Height ();
  } /* endif */

  if (text && parent)
  {
    w += TextWidth (text);
    h += TextHeight ();
  } /* endif */

  if (h < minh)
    h = minh;

  // Leave a bit of space at left, right, top and bottom
  w += LISTBOXITEM_XSPACE * 2;
  h += LISTBOXITEM_YSPACE * 2;
}

bool csListBoxItem::HandleEvent (csEvent &Event)
{
  switch (Event.Type)
  {
    case csevMouseDown:
      if (Event.Mouse.Button == 1)
      {
        parent->SendCommand (cscmdListBoxStartTracking, this);
        parent->SendCommand (cscmdListBoxItemClicked, this);
      }
      return true;
    case csevMouseMove:
      if ((app->MouseOwner == parent)
       && !GetState (CSS_FOCUSED))
        parent->SendCommand (cscmdListBoxTrack, this);
      return true;
    case csevMouseDoubleClick:
      if ((Event.Mouse.Button == 1) && parent)
        parent->SendCommand (cscmdListBoxItemDoubleClicked, this);
      return true;
    case csevCommand:
      switch (Event.Command.Code)
      {
        case cscmdListBoxItemCheck:
          Event.Command.Info = GetState (CSS_LISTBOXITEM_SELECTED)
            ? (void *)CS_LISTBOXITEMCHECK_SELECTED
            : (void *)CS_LISTBOXITEMCHECK_UNSELECTED;
          return true;
        case cscmdListBoxItemSet:
          if (Event.Command.Info)
            parent->SetFocused (this);
          SetState (CSS_LISTBOXITEM_SELECTED, (bool)Event.Command.Info);
          return true;
        case cscmdListBoxItemScrollVertically:
          if (bound.IsEmpty ())
            Event.Command.Info = (void *)true;
          else
          {
            int w,h;
            SuggestSize (w, h);
            if (bound.Height () < h)
              Event.Command.Info = (void *)true;
          } /* endif */
          return true;
        case cscmdListBoxItemSetHorizOffset:
          if (deltax != (int)Event.Command.Info)
          {
            deltax = (int)Event.Command.Info;
            Invalidate ();
          } /* endif */
          return true;
      } /* endswitch */
      break;
  } /* endswitch */
  return csComponent::HandleEvent (Event);
}

void csListBoxItem::SetState (int mask, bool enable)
{
  int oldstate = state;
  csComponent::SetState (mask, enable);
  if ((oldstate ^ state) & CSS_LISTBOXITEM_SELECTED)
  {
    Invalidate ();
    parent->SendCommand (GetState (CSS_LISTBOXITEM_SELECTED) ?
      cscmdListBoxItemSelected : cscmdListBoxItemDeselected, this);
  } /* endif */
  if ((oldstate ^ state) & CSS_FOCUSED)
  {
   Invalidate ();
   if (GetState (CSS_FOCUSED))
     parent->SendCommand (cscmdListBoxMakeVisible, this);
  } /* endif */
}

void csListBoxItem::SetBitmap (csSprite2D *iBitmap, bool iDelete)
{
  if (ItemBitmap && DeleteBitmap)
    delete ItemBitmap;
  ItemBitmap = iBitmap;
  DeleteBitmap = iDelete;
  Invalidate ();
}

void csListBoxItem::Draw ()
{
  bool selected = GetState (CSS_LISTBOXITEM_SELECTED);
  if (selected)
    Box (0, 0, bound.Width (), bound.Height (), CSPAL_LISTBOXITEM_SELECTION);
  if (GetState (CSS_FOCUSED))
  {
    int w,h;
    SuggestSize (w, h);
    int bh = bound.Height ();
    if (bh > h) h = bh;
    Rect3D (0, 0, bound.Width (), h, CSPAL_LISTBOXITEM_SELRECT,
      CSPAL_LISTBOXITEM_SELRECT);
  } /* endif */

  int color;
  if (GetState (CSS_SELECTABLE))
  {
    if (ItemStyle == cslisNormal)
      if (selected)
        color = CSPAL_LISTBOXITEM_SNTEXT;
      else
        color = CSPAL_LISTBOXITEM_UNTEXT;
    else
      if (selected)
        color = CSPAL_LISTBOXITEM_SETEXT;
      else
        color = CSPAL_LISTBOXITEM_UETEXT;
  } else
    color = CSPAL_LISTBOXITEM_DTEXT;

  int x = LISTBOXITEM_XSPACE - deltax + hOffset;
  if (ItemBitmap)
  {
    Sprite2D (ItemBitmap, x, vOffset + (bound.Height () - ItemBitmap->Height ()) / 2);
    x += ItemBitmap->Width () + 2;
  } /* endif */
  if (text)
  {
    Text (x, vOffset + (bound.Height () - TextHeight () + 1) / 2, color, -1, text);
    x += TextWidth (text);
  } /* endif */
  csComponent::Draw ();
}

csListBox::csListBox (csComponent *iParent, int iStyle,
  csListBoxFrameStyle iFrameStyle) : csComponent (iParent)
{
  state |= CSS_SELECTABLE;
  ListBoxStyle = iStyle;
  FrameStyle = iFrameStyle;
  SetPalette (CSPAL_LISTBOX);
  deltax = 0;
  csScrollBarFrameStyle sbsty;
  switch (FrameStyle)
  {
    case cslfsNone:
      BorderWidth = BorderHeight = 0;
      sbsty = cssfsThinRect;
      break;
    case cslfsThinRect:
      BorderWidth = BorderHeight = 2;
      sbsty = cssfsThinRect;
      break;
    case cslfsThickRect:
      BorderWidth = BorderHeight = 2;
      sbsty = cssfsThickRect;
      break;
    default:
      return;
  } /* endswitch */
  CHK (firstvisible = first = new csTimer (this, MOUSE_SCROLL_INTERVAL));
  if (iStyle & CSLBS_HSCROLL)
    CHKB (hscroll = new csScrollBar (this, sbsty))
  else
    hscroll = NULL;
  if (iStyle & CSLBS_VSCROLL)
    CHKB (vscroll = new csScrollBar (this, sbsty))
  else
    vscroll = NULL;
}

void csListBox::Draw ()
{
  switch (FrameStyle)
  {
    case cslfsNone:
      break;
    case cslfsThinRect:
      Rect3D (0, 0, bound.Width (), bound.Height (),
          CSPAL_INPUTLINE_LIGHT3D, CSPAL_INPUTLINE_DARK3D);
      Rect3D (1, 1, bound.Width () - 1, bound.Height () - 1,
          CSPAL_INPUTLINE_DARK3D, CSPAL_INPUTLINE_LIGHT3D);
      break;
    case cslfsThickRect:
      Rect3D (0, 0, bound.Width (), bound.Height (),
          CSPAL_INPUTLINE_LIGHT3D, CSPAL_INPUTLINE_DARK3D);
      Rect3D (1, 1, bound.Width () - 1, bound.Height () - 1,
          CSPAL_INPUTLINE_2LIGHT3D, CSPAL_INPUTLINE_2DARK3D);
      break;
  } /* endswitch */
  Box (BorderWidth, BorderHeight, bound.Width () - BorderWidth,
    bound.Height () - BorderHeight, FrameStyle == cslfsThickRect ?
    CSPAL_INPUTLINE_BACKGROUND2 : CSPAL_INPUTLINE_BACKGROUND);

  csComponent::Draw ();
}

void csListBox::PlaceItems (bool setscrollbars)
{
  int cury = BorderHeight;
  bool vis = false;

  // if focused item is not selectable, find next selectable item
  if (focused && !focused->GetState (CSS_SELECTABLE))
  {
    csComponent *cur = focused;
    do
    {
      cur = cur->next;
      if (ULong (cur->SendCommand (cscmdListBoxItemCheck, NULL)) == CS_LISTBOXITEMCHECK_SELECTED)
        break;
    } while (cur != focused);
    if (cur == focused)
    {
      SetFocused (NextChild (focused));
      focused->SetState (CSS_LISTBOXITEM_SELECTED, true);
    }
    else
      focused = cur;
  } /* endif */

  csComponent *cur = first;
  csRect itembound;
  csRect clipbound (BorderWidth, BorderHeight, bound.Width () - BorderWidth,
    bound.Height () - BorderHeight);
  if (hscroll)
    clipbound.ymax = hscroll->bound.ymin;
  if (vscroll)
    clipbound.xmax = vscroll->bound.xmin;
  vertcount = 0;
  // collect listbox statistics
  maxdeltax = 0;
  int itemcount = 0;
  int numfirst = 0;
  bool foundfirst = false;
  cur = first;
  while (cur)
  {
    if (cur == firstvisible)
    {
      foundfirst = true;
      // start reserving space from first visible item
      vis = true;
    } /* endif */

    int w, h;
    cur->SuggestSize (w, h);
    if (w && h)
    {
      itemcount++;
      if (!foundfirst)
        numfirst++;
      if (w > maxdeltax)
        maxdeltax = w;
    } /* endif */

    if (vis)
    {
      // Query current item width and height
      if (h > 0)
      {
        // Set current item x,y and height
        itembound.Set (BorderWidth, cury, bound.Width (), cury + h);
        itembound.Intersect (clipbound);
        if (!itembound.IsEmpty ())
          vertcount++;
        cur->SetRect (itembound);
        cur->SendCommand (cscmdListBoxItemSetHorizOffset, (void *)deltax);
        cury += h;
      } /* endif */
    } else
      if (w && h)
        cur->SetRect (0, 0, -1, -1);
    cur = cur->next;
    if (cur == first)
      break;
  } /* endwhile */

  if (setscrollbars)
  {
    if (vscroll)
    {
      vsbstatus.value = numfirst;
      vsbstatus.maxvalue = itemcount - vertcount;
      vsbstatus.size = vertcount;
      vsbstatus.maxsize = itemcount;
      vsbstatus.step = 1;
      vsbstatus.pagestep = vertcount;
      vscroll->SendCommand (cscmdScrollBarSet, &vsbstatus);
    } /* endif */
    if (hscroll)
    {
      hsbstatus.maxsize = maxdeltax;
      maxdeltax -= clipbound.Width ();
      if (maxdeltax < 0) maxdeltax = 0;
      hsbstatus.value = deltax;
      hsbstatus.maxvalue = maxdeltax;
      hsbstatus.size = clipbound.Width ();
      hsbstatus.step = 1;
      hsbstatus.pagestep = LISTBOX_HORIZONTAL_PAGESTEP;
      hscroll->SendCommand (cscmdScrollBarSet, &hsbstatus);
    } /* endif */
  } /* endif */
}

bool csListBox::SetRect (int xmin, int ymin, int xmax, int ymax)
{
  if (csComponent::SetRect (xmin, ymin, xmax, ymax))
  {
    if (hscroll)
      hscroll->SetRect (0, bound.Height () - CSSB_DEFAULTSIZE,
        bound.Width () - (vscroll ? CSSB_DEFAULTSIZE - 1 : 0),
        bound.Height ());
    if (vscroll)
      vscroll->SetRect (bound.Width () - CSSB_DEFAULTSIZE, 0,
        bound.Width (),
        bound.Height () - (hscroll ? CSSB_DEFAULTSIZE - 1 : 0));
    PlaceItems (true);
    return true;
  } else
    return false;
}

static bool do_select (csComponent *child, void *param)
{
  if (child != (csComponent *)param)
    child->SetState (CSS_LISTBOXITEM_SELECTED, true);
  return false;
}

static bool do_deselect (csComponent *child, void *param)
{
  if (child != (csComponent *)param)
    child->SetState (CSS_LISTBOXITEM_SELECTED, false);
  return false;
}

static bool do_deleteitem (csComponent *child, void *param)
{
  (void)param;
  CHK (delete child);
  return false;
}

static bool do_true (csComponent *child, void *param)
{
  (void)child; (void)param;
  return true;
}

bool csListBox::HandleEvent (csEvent &Event)
{
  switch (Event.Type)
  {
    case csevMouseUp:
      if ((Event.Mouse.Button == 1) && (app->MouseOwner == this))
      {
        app->CaptureMouse (NULL);
        return true;
      } /* endif */
      break;
    case csevKeyDown:
      switch (Event.Key.Code)
      {
        case CSKEY_UP:
          if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0)
          {
            SendCommand (cscmdListBoxTrack, (void *)focused->prev);
            return true;
          } /* endif */
          return false;
        case CSKEY_DOWN:
          if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0)
          {
            SendCommand (cscmdListBoxTrack, (void *)focused->next);
            return true;
          } /* endif */
          return false;
        case CSKEY_LEFT:
          if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == CSMASK_CTRL)
          {
            if (deltax > LISTBOX_HORIZONTAL_PAGESTEP)
              deltax -= LISTBOX_HORIZONTAL_PAGESTEP;
            else
              deltax = 0;
            PlaceItems ();
            return true;
          }
          else if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0)
          {
            if (deltax > 0)
              deltax--;
            else
              return true;
            PlaceItems ();
            return true;
          } /* endif */
          return false;
        case CSKEY_RIGHT:
          if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == CSMASK_CTRL)
          {
            if (deltax + LISTBOX_HORIZONTAL_PAGESTEP <= maxdeltax)
              deltax += LISTBOX_HORIZONTAL_PAGESTEP;
            else
              deltax = maxdeltax;
            PlaceItems ();
            return true;
          }
          else if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0)
          {
            if (deltax  < maxdeltax)
              deltax++;
            else
              return true;
            PlaceItems ();
            return true;
          } /* endif */
          return false;
        case CSKEY_PGUP:
          if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0)
          {
            for (int i = 0; i < vertcount; i++)
              SendCommand (cscmdListBoxTrack, (void *)focused->prev);
            return true;
          }
          else if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == CSMASK_CTRL)
          {
            SendCommand (cscmdListBoxTrack, (void *)NextChild (first));
            return true;
          }
          return false;
        case CSKEY_PGDN:
          if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0)
          {
            for (int i = 0; i < vertcount; i++)
              SendCommand (cscmdListBoxTrack, (void *)focused->next);
            return true;
          }
          else if ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == CSMASK_CTRL)
          {
            SendCommand (cscmdListBoxTrack, (void *)PrevChild (first));
            return true;
          }
          return false;
        case CSKEY_HOME:
          if (((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0) && (deltax != 0))
          {
            deltax = 0;
            PlaceItems ();
            return true;
          } /* endif */
          return false;
        case CSKEY_END:
          if (((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == 0) && (deltax != maxdeltax))
          {
            deltax = maxdeltax;
            PlaceItems ();
            return true;
          } /* endif */
          return false;
        case '/':
          if ((ListBoxStyle & CSLBS_MULTIPLESEL)
           && ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == CSMASK_CTRL))
          {
            ForEachItem (do_select, NULL, false);
            return true;
          } /* endif */
          return false;
        case '\\':
          if ((ListBoxStyle & CSLBS_MULTIPLESEL)
           && ((Event.Key.ShiftKeys & CSMASK_ALLSHIFTS) == CSMASK_CTRL))
          {
            ForEachItem (do_deselect, NULL);
            return true;
          } /* endif */
          return false;
      } /* endswitch */
      break;
    case csevCommand:
      switch (Event.Command.Code)
      {
        case cscmdListBoxClear:
          if (app->MouseOwner == this)
            app->CaptureMouse (NULL);
          ForEachItem (do_deleteitem, NULL, false);
          firstvisible = first;
          PlaceItems ();
          return true;
        case cscmdListBoxItemSelected:
          if ((ListBoxStyle & CSLBS_MULTIPLESEL) == 0)
            ForEachItem (do_deselect, Event.Command.Info);
          // fallback to resend
        case cscmdListBoxItemDeselected:
        case cscmdListBoxItemClicked:
        case cscmdListBoxItemDoubleClicked:
          // resend command to parent
          if (parent)
            parent->HandleEvent (Event);
          return true;
        case cscmdListBoxStartTracking:
        {
          csComponent *item = (csComponent *)Event.Command.Info;
          selstate = item->GetState (CSS_LISTBOXITEM_SELECTED) != 0;
          app->CaptureMouse (this);

          SetFocused (item);
          Select ();
          if (ListBoxStyle & CSLBS_MULTIPLESEL)
          {
            if (app->GetKeyState (CSKEY_CTRL))
              item->SetState (CSS_LISTBOXITEM_SELECTED, selstate = !selstate);
            else
            {
              ForEachItem (do_deselect, (void *)item);
              item->SetState (CSS_LISTBOXITEM_SELECTED, selstate = true);
            } /* endif */
          } else
            item->SetState (CSS_LISTBOXITEM_SELECTED, selstate = true);
          break;
        }
        case cscmdListBoxTrack:
        {
          csComponent *item = (csComponent *)Event.Command.Info;
          if (app->MouseOwner != this)
            selstate = true;
          if (item->GetState (CSS_SELECTABLE))
          {
            if (app->MouseOwner != this)
              ForEachItem (do_deselect, (void *)item);
            SetFocused (item);
            Select ();
            item->SetState (CSS_LISTBOXITEM_SELECTED, (ListBoxStyle & CSLBS_MULTIPLESEL)
             ? selstate : true);
          } /* endif */
          return true;
        }
        case cscmdListBoxMakeVisible:
          MakeItemVisible ((csComponent *)Event.Command.Info);
          return true;
        case cscmdListBoxQueryFirstSelected:
          Event.Command.Info = ForEachItem (do_true, NULL, true);
          return true;
        case cscmdTimerPulse:
          if (app && app->MouseOwner == this)
          {
            app->GetMouse ()->GetPosition (Event.Mouse.x, Event.Mouse.y);
            GlobalToLocal (Event.Mouse.x, Event.Mouse.y);
            if (app->MouseOwner == this)
            {
              if (Event.Mouse.y < BorderHeight)
                SendCommand (cscmdListBoxTrack, (void *)focused->prev);
              else if ((Event.Mouse.y > bound.Height () - BorderHeight)
                    || (hscroll
                     && (Event.Mouse.y >= hscroll->bound.ymin)))
                     SendCommand (cscmdListBoxTrack, (void *)focused->next);
            } /* endif */
          } /* endif */
          return true;
        case cscmdScrollBarValueChanged:
        {
          csScrollBar *bar = (csScrollBar *)Event.Command.Info;
          csScrollBarStatus sbs;
          if (!bar || bar->SendCommand (cscmdScrollBarGetStatus, &sbs))
            return true;

          if (sbs.maxvalue <= 0)
            return true;

          if (bar == hscroll)
          {
            hsbstatus = sbs;
            if (deltax != hsbstatus.value)
            {
              deltax = hsbstatus.value;
              PlaceItems (false);
            } /* endif */
          } else if (bar == vscroll)
          {
            vsbstatus = sbs;
            csComponent *cur = first;
            do
            {
              if (cur->SendCommand (cscmdListBoxItemCheck, NULL))
              {
                if (sbs.value == 0)
                {
                  if (firstvisible != cur)
                  {
                    firstvisible = cur;
                    PlaceItems (false);
                  } /* endif */
                  break;
                } /* endif */
                sbs.value--;
              } /* endif */
              cur = cur->next;
            } while (cur != first); /* enddo */
          } /* endif */
          return true;
        }
      } /* endswitch */
      break;
  } /* endswitch */
  return csComponent::HandleEvent (Event);
}

void csListBox::MakeItemVisible (csComponent *item)
{
  if (!item->SendCommand (cscmdListBoxItemScrollVertically, (void *)false))
  {
    // item is already visible
    return;
  }

  csComponent *cur = firstvisible;
  while ((cur != first) && (cur != item))
    cur = cur->prev;

  if (cur == item)
    firstvisible = cur;
  else
  {
    cur = item;
    int cy = bound.Height () - BorderHeight;
    if (hscroll)
      cy = hscroll->bound.ymin;
    firstvisible = item;
    while (firstvisible != first)
    {
      int w, h;
      cur->SuggestSize (w, h);
      cy -= h;
      if (cy < BorderHeight)
        break;
      firstvisible = cur;
      cur = cur->prev;
    } /* endwhile */
  } /* endif */
  PlaceItems ();
  if (parent)
    parent->SendCommand (cscmdListBoxItemFocused, (void *)focused->id);
}

csComponent *csListBox::ForEachItem (bool (*func) (csComponent *child,
  void *param), void *param, bool iSelected)
{
  if (!func)
    return NULL;

  csComponent *start = first;
  csComponent *cur = start;
  while (cur)
  {
    csComponent *next = cur->next;

    ULong reply = (long)cur->SendCommand (cscmdListBoxItemCheck, NULL);
    bool ok;
    if (iSelected)
      ok = (reply == CS_LISTBOXITEMCHECK_SELECTED);
    else
      ok = (reply == CS_LISTBOXITEMCHECK_SELECTED)
        || (reply == CS_LISTBOXITEMCHECK_UNSELECTED);
    if (ok && func (cur, param))
      return cur;
    if ((cur == next) || ((cur = next) == start))
      break;
  } /* endwhile */
  return NULL;
}
