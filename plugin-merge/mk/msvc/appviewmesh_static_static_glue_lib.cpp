// This file is automatically generated.
#include "cssysdef.h"

#if defined(CS_BUILD_SHARED_LIBS)
  CS_DECLARE_DEFAULT_STATIC_VARIABLE_REGISTRATION
  // Needed to work around some order of initialization issues
  static void csStaticVarCleanup_local (void (*p)())
  { csStaticVarCleanup_csutil (p); }
  CS_DEFINE_STATIC_VARIABLE_REGISTRATION (csStaticVarCleanup_local);
#endif
struct CS_EXPORT_SYM _Bind_viewmesh_static_plugins
{
  _Bind_viewmesh_static_plugins ();
};
struct _static_use { _static_use (); };
_Bind_viewmesh_static_plugins::_Bind_viewmesh_static_plugins () {}
// Needed to pull in _cs_static_use object file
namespace { _static_use _static_use_bind; }

