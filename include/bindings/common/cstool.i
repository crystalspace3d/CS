/* cstool module swig directives */

%ignore csColliderHelper::TraceBeam (iCollideSystem*, iSector*,
  const csVector3&, const csVector3&, bool, csIntersectingTriangle&,
  csVector3&, iMeshWrapper**);

/* ignore scfFakeInterface warning */
%template (scfFakecsColliderWrapper) scfFakeInterface<csColliderWrapper >;
%template(scfColliderWrapper) scfImplementationExt1<csColliderWrapper,csObject,scfFakeInterface<csColliderWrapper> >;
%include "cstool/collider.h"
%include "cstool/csview.h"
%include "cstool/csfxscr.h"

%include "cstool/cspixmap.h"
%include "cstool/enginetools.h"

%include "cstool/genmeshbuilder.h"

%ignore iPen::Rotate;

%include "cstool/pen.h"

%include "cstool/primitives.h"

%template(scfProcTexture) scfImplementationExt2<csProcTexture, csObject, iTextureWrapper, iProcTexture>;
%include "cstool/proctex.h"
%include "cstool/proctxtanim.h"

/* POST */
#ifndef SWIGIMPORTED
#undef APPLY_FOR_ALL_INTERFACES_POST
#define APPLY_FOR_ALL_INTERFACES_POST CSTOOL_APPLY_FOR_EACH_INTERFACE
%include "bindings/common/basepost.i"
cs_lang_include(cstoolpost.i)
#endif
