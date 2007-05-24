
#ifndef SWIGIMPORTED
  %module ivideo
  %include "bindings/allinterfaces.i"
  #define APPLY_FOR_ALL_INTERFACES_PRE APPLY_FOR_ALL_INTERFACES
  #define APPLY_FOR_ALL_INTERFACES_POST IVIDEO_APPLY_FOR_EACH_INTERFACE

  %include "bindings/basepre.i"
#endif

%import "bindings/csgfx.i"


%ignore iGraphics2D::PerformExtensionV;
%ignore iGraphics3D::PerformExtensionV;
%rename(GetRGBA) iGraphics2D::GetRGB(int, int&, int&, int&, int&);
%include "ivideo/graph2d.h"
%include "ivideo/graph3d.h"
%include "ivideo/cursor.h"

%ignore iNativeWindowManager::AlertV;
%include "ivideo/natwin.h"

%ignore GetGlyphSize(uint8, int &, int &);
%ignore GetGlyphBitmap(uint8, int &, int &);
%ignore GetGlyphAlphaBitmap(uint8, int &, int &);
%ignore GetDimensions(char const *, int &, int &);
%include "ivideo/fontserv.h"

%include "ivideo/halo.h"
%include "ivideo/shader/shader.h"

%rename(GetKeyColorStatus) iTextureHandle::GetKeyColor() const;
%include "ivideo/texture.h"

%include "ivideo/txtmgr.h"
%include "ivideo/material.h"

// ivideo/graph3d.h
#define _CS_FX_SETALPHA(a) CS_FX_SETALPHA(a)
#undef CS_FX_SETALPHA
uint _CS_FX_SETALPHA (uint);
#define _CS_FX_SETALPHA_INT(a) CS_FX_SETALPHA_INT(a)
#undef CS_FX_SETALPHA_INT
uint _CS_FX_SETALPHA_INT (uint);


#ifndef SWIGIMPORTED
  %include "bindings/basepost.i"
#endif


#if defined(SWIGPYTHON)
%extend csSimpleRenderMesh
{
  void SetWithGenmeshFactory(iGeneralFactoryState *factory)
  {
    self->vertices = factory->GetVertices();
    self->vertexCount = factory->GetVertexCount();
    self->indices = (uint *)factory->GetTriangles();
    self->indexCount = factory->GetTriangleCount()*3;
    self->texcoords = factory->GetTexels();
  }
}
%include "bindings/python/pythvarg.i"
#endif

