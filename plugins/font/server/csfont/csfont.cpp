/*
    Copyright (C) 2000 by Norman Kramer
    Copyright (C) 2000 by W.C.A. Wijngaards
    original unplugged code and fonts by Andrew Zabolotny

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
#include "csfont.h"
#include "ivfs.h"
#include "isystem.h"
#include "csutil/util.h"
#include "csutil/csvector.h"

#include "police.fnt"
#include "courier.fnt"	// font (C) Andrew Zabolotny
#include "tiny.fnt"	// font (C) Andrew Zabolotny
#include "italic.fnt"	// font (C) Andrew Zabolotny

struct csFontDef
{
  char *Name;
  int Width;
  int Height;
  int First;
  int Glyphs;
  uint8 *FontBitmap;
  uint8 *IndividualWidth;
};

static csFontDef const FontList [] =
{
  { CSFONT_LARGE,	8, 8, 0, 256, font_Police,	width_Police	},
  { CSFONT_ITALIC,	8, 8, 0, 256, font_Italic,	width_Italic	},
  { CSFONT_COURIER,	7, 8, 0, 256, font_Courier,	width_Courier	},
  { CSFONT_SMALL,	4, 6, 0, 256, font_Tiny,	width_Tiny	}
};

int const FontListCount = sizeof (FontList) / sizeof (csFontDef);

IMPLEMENT_IBASE (csDefaultFontServer)
  IMPLEMENTS_INTERFACE (iPlugIn)
  IMPLEMENTS_INTERFACE (iFontServer)
IMPLEMENT_IBASE_END

IMPLEMENT_FACTORY (csDefaultFontServer)

EXPORT_CLASS_TABLE (csfont)
  EXPORT_CLASS (csDefaultFontServer, "crystalspace.font.server.default", 
    "Crystal Space default font server")
EXPORT_CLASS_TABLE_END

csDefaultFontServer::csDefaultFontServer (iBase *pParent) : System(0)
{
  CONSTRUCT_IBASE (pParent);
}

csDefaultFontServer::~csDefaultFontServer()
{
  if (System)
    System->DecRef();
}

bool csDefaultFontServer::Initialize (iSystem* sys)
{
  (System = sys)->IncRef ();
  return true;
}

iFont *csDefaultFontServer::LoadFont (const char *filename)
{
  // First of all, look for an already loaded font
  for (int i = 0; i < fonts.Length (); i++)
  {
    csDefaultFont *font = fonts.Get (i);
    if (!strcmp (filename, font->Name))
    {
      font->IncRef ();
      return font;
    }
  }

  // Now check the list of static fonts
  if (filename [0] == '*')
  {
    for (int i = 0; i < FontListCount; i++)
      if (!strcmp (filename, FontList [i].Name))
        return new csDefaultFont (this, FontList [i].Name,
          FontList [i].First, FontList [i].Glyphs,
          FontList [i].Width, FontList [i].Height,
          FontList [i].FontBitmap, FontList [i].IndividualWidth);
  }
  else
  {
    // Otherwise try to load the font as a .csf file
    csDefaultFont *fontdef = ReadFontFile (filename);
    if (fontdef)
    {
      delete [] fontdef->Name;
      fontdef->Name = strnew (filename);
      return fontdef;
    }
  }

  return NULL;
}

iFont *csDefaultFontServer::GetFont (int iIndex)
{
  if ((iIndex >= 0) && (iIndex < fonts.Length ()))
  {
    iFont *font = fonts.Get (iIndex);
    if (font) return font;
  }
  return NULL;
}

void csDefaultFontServer::NotifyCreate (csDefaultFont *font)
{
  fonts.Push (font);
}

void csDefaultFontServer::NotifyDelete (csDefaultFont *font)
{
  int iIndex = fonts.Find (font);
  if (iIndex >= 0)
  {
    // Set the cell to 0 to avoid the font being freed twice
    fonts.Get (iIndex) = NULL;
    fonts.Delete (iIndex);
  }
}

csDefaultFont *csDefaultFontServer::ReadFontFile(const char *file)
{
  iVFS *VFS = QUERY_PLUGIN (System, iVFS);
  iDataBuffer *fntfile = VFS->ReadFile (file);
  VFS->DecRef ();
  if (!fntfile)
  {
    System->Printf (MSG_WARNING, "Could not read font file %s.\n", file);
    return NULL;
  }

  char *data = **fntfile;
  if (data [0] != 'C' || data [1] != 'S' ||  data [2] != 'F')
  {
error:
    fntfile->DecRef ();
    return NULL;
  }

  /// the new fontdef to store info into
  csFontDef fontdef;
  memset (&fontdef, 0, sizeof (fontdef));
  fontdef.Glyphs = 256;

  char *end = strchr (data, '\n');
  char *cur = strchr (data, '[');
  if (!end || !cur)
    goto error;

  char *binary = end + 1;
  while ((end > data) && ((end [-1] == ' ') || (end [-1] == ']')))
    end--;

  cur++;
  while (cur < end)
  {
    while ((cur < end) && (*cur == ' '))
      cur++;

    char kw [20];
    size_t kwlen = 0;
    while ((cur < end) && (*cur != '=') && (kwlen < sizeof (kw) - 1))
      kw [kwlen++] = *cur++;
    kw [kwlen] = 0;
    if (!kwlen)
      break;

    cur = strchr (cur, '=');
    if (!cur) break;
    cur++;

    if (!strcmp (kw, "Font"))
    {
      char *start = cur;
      while ((cur < end) && (*cur != ' '))
        cur++;
      fontdef.Name = new char [cur - start + 1];
      memcpy (fontdef.Name, start, cur - start);
      fontdef.Name [cur - start] = 0;
    }
    else
    {
      char val [20];
      size_t vallen = 0;
      while ((cur < end) && (*cur != ' ') && (vallen < sizeof (val) - 1))
        val [vallen++] = *cur++;
      val [vallen] = 0;
      int n = atoi (val);

      if (!strcmp (kw, "Width"))
        fontdef.Width = n;
      else if (!strcmp (kw, "Height"))
        fontdef.Height = n;
      else if (!strcmp (kw, "First"))
        fontdef.First = n;
      else if (!strcmp (kw, "Glyphs"))
        fontdef.Glyphs = n;
    }
  }

#if defined(CS_DEBUG)
  printf("Reading Font %s, width %d, height %d, First %d, %d Glyphs.\n",
    fontdef.Name, fontdef.Width, fontdef.Height, fontdef.First, fontdef.Glyphs);
#endif

  fontdef.IndividualWidth = new uint8 [fontdef.Glyphs];
  memcpy (fontdef.IndividualWidth, binary, fontdef.Glyphs);

  // Compute the font size
  int fontsize = 0;
  for (int c = 0; c < fontdef.Glyphs; c++)
    fontsize += ((fontdef.IndividualWidth [c] + 7) / 8) * fontdef.Height;

  // allocate memory and copy the font
  fontdef.FontBitmap = new uint8 [fontsize];
  memcpy (fontdef.FontBitmap, binary + fontdef.Glyphs, fontsize);

  fntfile->DecRef ();
  return new csDefaultFont (this, fontdef.Name, fontdef.First, fontdef.Glyphs,
    fontdef.Width, fontdef.Height, fontdef.FontBitmap, fontdef.IndividualWidth);
}


//--//--//--//--//--//--//--//--//--//--//--//--//--//- The font object -//--//

IMPLEMENT_IBASE (csDefaultFont)
  IMPLEMENTS_INTERFACE (iFont)
IMPLEMENT_IBASE_END

csDefaultFont::csDefaultFont (csDefaultFontServer *parent, const char *name,
  int first, int glyphs, int width, int height, uint8 *bitmap,
  uint8 *individualwidth) : DeleteCallbacks (4, 4)
{
  CONSTRUCT_IBASE (parent);
  Parent = parent;
  Parent->NotifyCreate (this);
  if (name [0] != '*')
    Name = strnew (name);
  else
    Name = CONST_CAST(char*)(name);
  First = first;
  Glyphs = glyphs;
  Width = width;
  Height = height;
  FontBitmap = bitmap;
  IndividualWidth = individualwidth;

  if (IndividualWidth)
  {
    MaxWidth = 0;
    for (int i = 0; i < Glyphs; i++)
      if (MaxWidth < IndividualWidth [i])
        MaxWidth = IndividualWidth [i];
  }
  else
    MaxWidth = Width;

  uint8 *cur = bitmap;
  GlyphBitmap = new uint8 * [Glyphs];
  for (int i = 0; i < Glyphs; i++)
  {
    GlyphBitmap [i] = cur;
    cur += ((IndividualWidth [i] + 7) / 8) * Height;
  }
}

csDefaultFont::~csDefaultFont ()
{
  for (int i = DeleteCallbacks.Length () - 2; i >= 0; i -= 2)
    ((DeleteNotify)DeleteCallbacks.Get (i))(this, DeleteCallbacks.Get (i + 1));

  Parent->NotifyDelete (this);
  if (Name [0] != '*')
  {
    delete [] Name;
    delete [] FontBitmap;
    delete [] IndividualWidth;
  }
}

void csDefaultFont::SetSize (int iSize)
{
  (void)iSize;
}

int csDefaultFont::GetSize ()
{
  return 0;
}

void csDefaultFont::GetMaxSize (int &oW, int &oH)
{
  oW = MaxWidth;
  oH = Height; 
}

bool csDefaultFont::GetGlyphSize (uint8 c, int &oW, int &oH)
{
  int chr = int (c) - First;
  if ((chr < 0) || (chr > Glyphs))
  {
    oW = oH = 0;
    return false;
  }

  oW = IndividualWidth ? IndividualWidth [chr] : Width;
  oH = Height;
  return true;
}

uint8 *csDefaultFont::GetGlyphBitmap (uint8 c, int &oW, int &oH)
{
  int chr = int (c) - First;
  if ((chr < 0) || (chr > Glyphs))
  {
    oW = oH = 0;
    return NULL;
  }

  oW = IndividualWidth ? IndividualWidth [chr] : Width;
  oH = Height;
  return GlyphBitmap [chr];
}

void csDefaultFont::GetDimensions (const char *text, int &oW, int &oH)
{
  oH = Height;
  oW = 0;

  const int n = strlen (text);
  if (!IndividualWidth)
    oW = n * Width;
  else
    for (int i = 0; i < n; i++)
    {
      int chr = (*(uint8 *)text++) - First;
      if ((chr >= 0) && (chr < Glyphs))
        oW += IndividualWidth [chr];
    }
}

int csDefaultFont::GetLength (const char *text, int maxwidth)
{
  if (!IndividualWidth)
    return (strlen (text) * Width) / maxwidth;

  int n = 0;
  while (*text)
  {
    int chr = (*(uint8 *)text) - First;
    if ((chr >= 0) && (chr < Glyphs))
    {
      int w = IndividualWidth [chr];
      if (maxwidth < w)
        return n;
      maxwidth -= w;
    }
    text++; n++;
  }
  return n;
}

void csDefaultFont::AddDeleteCallback (DeleteNotify func, void *databag)
{
  DeleteCallbacks.Push ((void *)func);
  DeleteCallbacks.Push (databag);
}

bool csDefaultFont::RemoveDeleteCallback (DeleteNotify func, void *databag)
{
  for (int i = DeleteCallbacks.Length () - 2; i >= 0; i -= 2)
    if ((DeleteCallbacks.Get (i) == (void *)func)
     && (DeleteCallbacks.Get (i + 1) == databag))
    {
      DeleteCallbacks.Delete (i);
      DeleteCallbacks.Delete (i);
      return true;
    }
  return false;
}
