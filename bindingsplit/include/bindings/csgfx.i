%module csgfx
%import "bindings/cspace.i"
%{
#include "crystalspace.h"
%}
LANG_FUNCTIONS

%rename(asRGBcolor) csRGBpixel::operator csRGBcolor;
%include "csgfx/rgbpixel.h"
%ignore ShaderVarName;
%include "csgfx/shadervar.h"
%template(csShaderVariableArrayReadOnly) iArrayReadOnly<csShaderVariable * >;
%template(csShaderVariableArrayChangeElements)
iArrayChangeElements<csShaderVariable * >;
%template(csShaderVariableArray) iArrayChangeAll<csShaderVariable * >;

%template(csImageBaseBase) scfImplementation1<csImageBase, iImage>;
%include "csgfx/imagebase.h"
%template(csImageMemoryBase) scfImplementationExt0<csImageMemory, csImageBase>;
%include "csgfx/imagememory.h"

#ifdef SWIGPYTHON
%include "bindings/python/pyshadervar.i"
#endif


