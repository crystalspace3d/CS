/*
    Copyright (C) 1998 by Jorrit Tyberghein
    Contributions made by Ivan Avramovic <ivan@avramovic.com>

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
#include <math.h>
#include <assert.h>

#include "sysdef.h"
#include "isystem.h"
#include "csgfxldr/csimage.h"
#include "csgfxldr/boxfilt.h"
#include "csutil/csvector.h"

//---------------------------------------------------------------------------

void box_filter3 (Filter3x3& f, RGBPixel* bm, int x, int y, int w, int h, int* pr, int* pg, int* pb)
{
  ULong m11, m12, m13;
  ULong m21, m22, m23;
  ULong m31, m32, m33;
  int r, g, b;
  m11 = ((x-1+w)%w) + ((y-1+h)%h) * w;
  m12 = x           + ((y-1+h)%h) * w;
  m13 = ((x+1)%w)   + ((y-1+h)%h) * w;
  m21 = ((x-1+w)%w) + y * w;
  m22 = x           + y * w;
  m23 = ((x+1)%w)   + y * w;
  m31 = ((x-1+w)%w) + ((y+1)%h) * w;
  m32 = x           + ((y+1)%h) * w;
  m33 = ((x+1)%w)   + ((y+1)%h) * w;
  r = f.f11*bm[m11].red + f.f12*bm[m12].red + f.f13*bm[m13].red +
      f.f21*bm[m21].red + f.f22*bm[m22].red + f.f23*bm[m23].red +
      f.f31*bm[m31].red + f.f32*bm[m32].red + f.f33*bm[m33].red;
  g = f.f11*bm[m11].green + f.f12*bm[m12].green + f.f13*bm[m13].green +
      f.f21*bm[m21].green + f.f22*bm[m22].green + f.f23*bm[m23].green +
      f.f31*bm[m31].green + f.f32*bm[m32].green + f.f33*bm[m33].green;
  b = f.f11*bm[m11].blue + f.f12*bm[m12].blue + f.f13*bm[m13].blue +
      f.f21*bm[m21].blue + f.f22*bm[m22].blue + f.f23*bm[m23].blue +
      f.f31*bm[m31].blue + f.f32*bm[m32].blue + f.f33*bm[m33].blue;
  *pr = r/f.tot;
  *pg = g/f.tot;
  *pb = b/f.tot;
}

void box_filter5 (Filter5x5& f, RGBPixel* bm, int x, int y, int w, int h, int* pr, int* pg, int* pb)
{
  ULong m00, m01, m02, m03, m04;
  ULong m10, m11, m12, m13, m14;
  ULong m20, m21, m22, m23, m24;
  ULong m30, m31, m32, m33, m34;
  ULong m40, m41, m42, m43, m44;
  int r, g, b;
  m00 = ((x-2+w)%w) + ((y-2+h)%h) * w;
  m01 = ((x-1+w)%w) + ((y-2+h)%h) * w;
  m02 = x           + ((y-2+h)%h) * w;
  m03 = ((x+1)%w)   + ((y-2+h)%h) * w;
  m04 = ((x+2)%w)   + ((y-2+h)%h) * w;
  m10 = ((x-2+w)%w) + ((y-1+h)%h) * w;
  m11 = ((x-1+w)%w) + ((y-1+h)%h) * w;
  m12 = x           + ((y-1+h)%h) * w;
  m13 = ((x+1)%w)   + ((y-1+h)%h) * w;
  m14 = ((x+2)%w)   + ((y-1+h)%h) * w;
  m20 = ((x-2+w)%w) + y * w;
  m21 = ((x-1+w)%w) + y * w;
  m22 = x           + y * w;
  m23 = ((x+1)%w)   + y * w;
  m24 = ((x+2)%w)   + y * w;
  m30 = ((x-2+w)%w) + ((y+1)%h) * w;
  m31 = ((x-1+w)%w) + ((y+1)%h) * w;
  m32 = x           + ((y+1)%h) * w;
  m33 = ((x+1)%w)   + ((y+1)%h) * w;
  m34 = ((x+2)%w)   + ((y+1)%h) * w;
  m40 = ((x-2+w)%w) + ((y+2)%h) * w;
  m41 = ((x-1+w)%w) + ((y+2)%h) * w;
  m42 = x           + ((y+2)%h) * w;
  m43 = ((x+1)%w)   + ((y+2)%h) * w;
  m44 = ((x+2)%w)   + ((y+2)%h) * w;
  r = f.f00*bm[m00].red + f.f01*bm[m01].red + f.f02*bm[m02].red + f.f03*bm[m03].red + f.f04*bm[m04].red +
      f.f10*bm[m10].red + f.f11*bm[m11].red + f.f12*bm[m12].red + f.f13*bm[m13].red + f.f14*bm[m14].red +
      f.f20*bm[m20].red + f.f21*bm[m21].red + f.f22*bm[m22].red + f.f23*bm[m23].red + f.f24*bm[m24].red +
      f.f30*bm[m30].red + f.f31*bm[m31].red + f.f32*bm[m32].red + f.f33*bm[m33].red + f.f34*bm[m34].red +
      f.f40*bm[m40].red + f.f41*bm[m41].red + f.f42*bm[m42].red + f.f43*bm[m43].red + f.f44*bm[m44].red;
  g = f.f00*bm[m00].green + f.f01*bm[m01].green + f.f02*bm[m02].green + f.f03*bm[m03].green + f.f04*bm[m04].green +
      f.f10*bm[m10].green + f.f11*bm[m11].green + f.f12*bm[m12].green + f.f13*bm[m13].green + f.f14*bm[m14].green +
      f.f20*bm[m20].green + f.f21*bm[m21].green + f.f22*bm[m22].green + f.f23*bm[m23].green + f.f24*bm[m24].green +
      f.f30*bm[m30].green + f.f31*bm[m31].green + f.f32*bm[m32].green + f.f33*bm[m33].green + f.f34*bm[m34].green +
      f.f40*bm[m40].green + f.f41*bm[m41].green + f.f42*bm[m42].green + f.f43*bm[m43].green + f.f44*bm[m44].green;
  b = f.f00*bm[m00].blue + f.f01*bm[m01].blue + f.f02*bm[m02].blue + f.f03*bm[m03].blue + f.f04*bm[m04].blue +
      f.f10*bm[m10].blue + f.f11*bm[m11].blue + f.f12*bm[m12].blue + f.f13*bm[m13].blue + f.f14*bm[m14].blue +
      f.f20*bm[m20].blue + f.f21*bm[m21].blue + f.f22*bm[m22].blue + f.f23*bm[m23].blue + f.f24*bm[m24].blue +
      f.f30*bm[m30].blue + f.f31*bm[m31].blue + f.f32*bm[m32].blue + f.f33*bm[m33].blue + f.f34*bm[m34].blue +
      f.f40*bm[m40].blue + f.f41*bm[m41].blue + f.f42*bm[m42].blue + f.f43*bm[m43].blue + f.f44*bm[m44].blue;
  *pr = r/f.tot;
  *pg = g/f.tot;
  *pb = b/f.tot;
}

//---------------------------------------------------------------------------

IMPLEMENT_UNKNOWN (ImageFile)
  
BEGIN_INTERFACE_TABLE (ImageFile)
	IMPLEMENTS_COMPOSITE_INTERFACE (ImageFile)
END_INTERFACE_TABLE()
	    


ImageFile::ImageFile ()
{
  image = NULL;
  status = IFE_OK;
}

ImageFile::~ImageFile ()
{
  CHK (delete [] image);
}

void ImageFile::set_dimensions (int w, int h)
{
  width = w;
  height = h;
  if (image) CHKB (delete [] image);
  CHK (image = new RGBPixel [w*h]);
}

const char* ImageFile::get_status_mesg() const
{
  if (status & IFE_BadFormat) return "wrong data format";
  else if (status & IFE_Corrupt) return "image file corrupt";
  else return "image successfully read";
}

ImageFile* ImageFile::mipmap (int steps, Filter3x3* filt1, Filter5x5* filt2)
{
  CHK (ImageFile* nimg = new ImageFile ());
  int w = width, h = height;
  int w2, h2;
  if (steps == 1) { w2 = width / 2; h2 = height / 2; }
  else { w2 = width / 4; h2 = height / 4; }
  nimg->set_dimensions (w2, h2);
  RGBPixel* nimage = nimg->get_buffer ();
  RGBPixel* bm = image, * bm2 = nimage;
  int x, y, r, g, b;
  if (steps == 1)
    if (filt1)
      for (y = 0 ; y < h ; y += 2)
        for (x = 0 ; x < w ; x += 2)
        {
          box_filter3 (*filt1, bm, x, y, w, h, &r, &g, &b);
	  bm2->red = r;
	  bm2->green = g;
	  bm2->blue = b;
          bm2++;
        }
    else
      for (y = 0 ; y < h2 ; y++)
      {
        for (x = 0 ; x < w2 ; x++)
        {
          // @@@ Consider a more accurate algorithm that shifts the source bitmap one
          // half pixel. In the current implementation there is a small shift in the
          // texture data.
          r = bm->red + (bm+1)->red + (bm+w)->red + (bm+w+1)->red;
          g = bm->green + (bm+1)->green + (bm+w)->green + (bm+w+1)->green;
          b = bm->blue + (bm+1)->blue + (bm+w)->blue + (bm+w+1)->blue;
	  bm2->red = r;
	  bm2->green = g;
	  bm2->blue = b;
	  bm2++;
          bm += 2;
        }
        bm += w;
      }
  else
    if (filt2)
      for (y = 0 ; y < h ; y += 4)
        for (x = 0 ; x < w ; x += 4)
        {
          box_filter5 (*filt2, bm, x+1, y+1, w, h, &r, &g, &b);
	  bm2->red = r;
	  bm2->green = g;
	  bm2->blue = b;
          bm2++;
        }
    else
      for (y = 0 ; y < h2 ; y++)
      {
        for (x = 0 ; x < w2 ; x++)
        {
          r = bm->red         + (bm+1)->red       + (bm+2)->red       + (bm+3)->red +
              (bm+w)->red     + (bm+w+1)->red     + (bm+w+2)->red     + (bm+w+3)->red +
              (bm+w+w)->red   + (bm+w+w+1)->red   + (bm+w+w+2)->red   + (bm+w+w+3)->red +
              (bm+w*3)->red   + (bm+w*3+1)->red   + (bm+w*3+2)->red   + (bm+w*3+3)->red;
          g = bm->green         + (bm+1)->green       + (bm+2)->green       + (bm+3)->green +
              (bm+w)->green     + (bm+w+1)->green     + (bm+w+2)->green     + (bm+w+3)->green +
              (bm+w+w)->green   + (bm+w+w+1)->green   + (bm+w+w+2)->green   + (bm+w+w+3)->green +
              (bm+w*3)->green   + (bm+w*3+1)->green   + (bm+w*3+2)->green   + (bm+w*3+3)->green;
          b = bm->blue         + (bm+1)->blue       + (bm+2)->blue       + (bm+3)->blue +
              (bm+w)->blue     + (bm+w+1)->blue     + (bm+w+2)->blue     + (bm+w+3)->blue +
              (bm+w+w)->blue   + (bm+w+w+1)->blue   + (bm+w+w+2)->blue   + (bm+w+w+3)->blue +
              (bm+w*3)->blue   + (bm+w*3+1)->blue   + (bm+w*3+2)->blue   + (bm+w*3+3)->blue;
	  bm2->red = r;
	  bm2->green = g;
	  bm2->blue = b;
          bm2++;
          bm += 4;
        }
        bm += w*3;
      }

  // Really ugly hack to support 3 levels without having to do to much work
  // in the above routines. steps==3 is done with 2 steps above and here I call
  // mipmap again for the final step.
  if (steps == 3)
  {
    ImageFile* nimg2 = nimg->mipmap (1, filt1, filt2);
    CHK (delete nimg);
    nimg = nimg2;
  }

  return nimg;
}

ImageFile* ImageFile::mipmap (int steps)
{
  CHK (ImageFile* nimg = new ImageFile ());
  int w = width;
  int w2, h2;
  if (steps == 1) { w2 = width / 2; h2 = height / 2; }
  else if (steps == 2) { w2 = width / 4; h2 = height / 4; }
  else { w2 = width / 8; h2 = height / 8; }
  nimg->set_dimensions (w2, h2);
  RGBPixel* nimage = nimg->get_buffer ();
  RGBPixel* bm = image, * bm2 = nimage;
  int x, y;
  if (steps == 1)
    for (y = 0 ; y < h2 ; y++)
    {
      for (x = 0 ; x < w2 ; x++)
      {
        *bm2++ = *bm;
        bm += 2;
      }
      bm += w;
    }
  else if (steps == 2)
    for (y = 0 ; y < h2 ; y++)
    {
      for (x = 0 ; x < w2 ; x++)
      {
        *bm2++ = *bm;
        bm += 4;
      }
      bm += w*3;
    }
  else
    for (y = 0 ; y < h2 ; y++)
    {
      for (x = 0 ; x < w2 ; x++)
      {
        *bm2++ = *bm;
        bm += 8;
      }
      bm += w*7;
    }

  return nimg;
}

ImageFile* ImageFile::blend (Filter3x3* filt1)
{
  CHK (ImageFile* nimg = new ImageFile ());
  nimg->set_dimensions (width, height);
  RGBPixel* nimage = nimg->get_buffer ();
  RGBPixel* bm = image, * bm2 = nimage;
  int x, y, r, g, b;
  for (y = 0 ; y < height ; y++)
    for (x = 0 ; x < width ; x++)
    {
      box_filter3 (*filt1, bm, x, y, width, height, &r, &g, &b);
      bm2->red = r;
      bm2->green = g;
      bm2->blue = b;
      bm2++;
    }
  return nimg;
}

//---------------------------------------------------------------------------

// Register all file formats we want
#define REGISTER_FORMAT(format) \
  extern bool Register##format (); \
  bool __##format##Support = Register##format ();

#if defined (DO_GIF)
  REGISTER_FORMAT (GIF)
#endif
#if defined (DO_PNG)
  REGISTER_FORMAT (PNG)
#endif
#if defined (DO_JPG)
  REGISTER_FORMAT (JPG)
#endif
#if defined (DO_TGA)
  REGISTER_FORMAT (TGA)
#endif
#if defined (DO_BMP)
  REGISTER_FORMAT (BMP)
#endif

// Unfortunately, we can't use static variable here since Register()
// can happened to be called BEFORE loaderlist is initialized... so we
// have to track initialization in Register() itself...
csVector *ImageLoader::loaderlist = NULL;

bool ImageLoader::Register (ImageLoader* loader)
{
  if (!loaderlist)
    loaderlist = new csVector (8, 8);
  loaderlist->Push ((csSome)loader);
  return true;
}

ImageFile* ImageLoader::load (ISystem* sys, char* filename)
{
  FILE* fp;
  sys->FOpen (filename, "rb", &fp);
  if (!fp) return NULL;
  ImageFile* i = load(fp);
  sys->FClose (fp);
  return i;
}

ImageFile* ImageLoader::load (FILE* fp)
{
  fseek(fp, 0, SEEK_END);
  ULong size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  CHK (UByte* buf = new UByte [size+1] );
  fread(buf, 1, size+1, fp);
  ImageFile* i = load(buf,size);
  CHK (delete [] buf);
  return i;
}

ImageFile* ImageLoader::load (UByte* buf, ULong size)
{
  int i=0;
  while (i < loaderlist->Length() )
  {
    ImageLoader* l = (ImageLoader*) (loaderlist->Get (i));
    ImageFile* img = l->LoadImage (buf,size);
    if (img) return img;
    i++; 
  }
  return NULL;
}

//---------------------------------------------------------------------------
