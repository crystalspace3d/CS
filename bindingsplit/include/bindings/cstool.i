%module cstool
%import "bindings/cspace.i"
%{
#include "crystalspace.h"
%}
LANG_FUNCTIONS


/* REST */
/*
%ignore csInitializer::RequestPlugins(iObjectRegistry*, ...);
%ignore csInitializer::RequestPluginsV;
%rename (_RequestPlugins) csInitializer::RequestPlugins(iObjectRegistry*,
  csArray<csPluginRequest> const&);

%ignore csInitializer::SetupEventHandler (iObjectRegistry*, csEventHandlerFunc,
  const csEventID events[]);
%ignore csInitializer::SetupEventHandler (iObjectRegistry*, csEventHandlerFunc);
%rename (_SetupEventHandler) csInitializer::SetupEventHandler (iObjectRegistry*,
  iEventHandler *, const csEventID[]);
%typemap(default) const char * configName { $1 = 0; }

// Swig 1.3.24 doesn't handle pointer default args well unless we tell it
// to use an alternate way for that function
%feature("compactdefaultargs") csInitializer::SetupConfigManager;
%include "cstool/initapp.h"
%typemap(default) const char * configName;
*/
%ignore csColliderHelper::TraceBeam (iCollideSystem*, iSector*,
  const csVector3&, const csVector3&, bool, csIntersectingTriangle&,
  csVector3&, iMeshWrapper**);
%template(pycsColliderWrapper) scfImplementationExt1<csColliderWrapper,csObject,scfFakeInterface<csColliderWrapper> >;
%include "cstool/collider.h"
%include "cstool/csview.h"
%include "cstool/csfxscr.h"

%include "cstool/cspixmap.h"
%include "cstool/enginetools.h"

/*%template(csPluginRequestArray) csArray<csPluginRequest>;*/
%ignore iPen::Rotate;

%include "cstool/pen.h"

%include "cstool/primitives.h"

/* pythpost */
/* work around broken Rotate function with swig 1.3.28 */
#if defined(SWIGPYTHON)
%extend iPen {
        void _Rotate(float a)
        { self->Rotate(a); }
    %pythoncode %{
    def Rotate(self,a):
         return _cspace.iPen__Rotate(a)
    %}
}
#endif

/*
%pythoncode %{
  def _csInitializer_RequestPlugins (reg, plugins):
    """Replacement of C++ version."""
    def _get_tuple (x):
      if callable(x):
        return tuple(x())
      else:
        return tuple(x)
    requests = csPluginRequestArray()
    for cls, intf, ident, ver in map(
        lambda x: _get_tuple(x), plugins):
      requests.Push(csPluginRequest(
        csString(cls), csString(intf), ident, ver))
    return csInitializer._RequestPlugins(reg, requests)

  csInitializer.RequestPlugins = staticmethod(_csInitializer_RequestPlugins)
%}
%pythoncode %{
  def _csInitializer_SetupEventHandler (reg, obj,
      eventids=None):
    """Replacement of C++ versions."""
    if callable(obj):
      # obj is a function
      hdlr = _EventHandlerFuncWrapper(obj)
      hdlr.thisown = 1
    else:
      # assume it is a iEventHandler
      hdlr = obj
    if eventids==None:
      eventids=[csevFrame(reg), csevInput(reg), csevKeyboardEvent(reg), \
                csevMouseEvent(reg), csevQuit(reg), CS_EVENTLIST_END]
    return csInitializer._SetupEventHandler(reg, hdlr, eventids)

  csInitializer.SetupEventHandler = \
    staticmethod(_csInitializer_SetupEventHandler)
%}

*/
