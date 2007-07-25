# This file was created automatically by SWIG 1.3.29.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _pycscegui
import new
new_instancemethod = new.instancemethod
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'PySwigObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


import cspace
class iCEGUI(cspace.iBase):
    __swig_setmethods__ = {}
    for _s in [cspace.iBase]: __swig_setmethods__.update(_s.__swig_setmethods__)
    __setattr__ = lambda self, name, value: _swig_setattr(self, iCEGUI, name, value)
    __swig_getmethods__ = {}
    for _s in [cspace.iBase]: __swig_getmethods__.update(_s.__swig_getmethods__)
    __getattr__ = lambda self, name: _swig_getattr(self, iCEGUI, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def Initialize(*args): return _pycscegui.iCEGUI_Initialize(*args)
    def Render(*args): return _pycscegui.iCEGUI_Render(*args)
    def GetSystemPtr(*args): return _pycscegui.iCEGUI_GetSystemPtr(*args)
    def GetFontManagerPtr(*args): return _pycscegui.iCEGUI_GetFontManagerPtr(*args)
    def GetGlobalEventSetPtr(*args): return _pycscegui.iCEGUI_GetGlobalEventSetPtr(*args)
    def GetImagesetManagerPtr(*args): return _pycscegui.iCEGUI_GetImagesetManagerPtr(*args)
    def GetLoggerPtr(*args): return _pycscegui.iCEGUI_GetLoggerPtr(*args)
    def GetMouseCursorPtr(*args): return _pycscegui.iCEGUI_GetMouseCursorPtr(*args)
    def GetSchemeManagerPtr(*args): return _pycscegui.iCEGUI_GetSchemeManagerPtr(*args)
    def GetWindowFactoryManagerPtr(*args): return _pycscegui.iCEGUI_GetWindowFactoryManagerPtr(*args)
    def GetWindowManagerPtr(*args): return _pycscegui.iCEGUI_GetWindowManagerPtr(*args)
    def EnableMouseCapture(*args): return _pycscegui.iCEGUI_EnableMouseCapture(*args)
    def DisableMouseCapture(*args): return _pycscegui.iCEGUI_DisableMouseCapture(*args)
    def EnableKeyboardCapture(*args): return _pycscegui.iCEGUI_EnableKeyboardCapture(*args)
    def DisableKeyboardCapture(*args): return _pycscegui.iCEGUI_DisableKeyboardCapture(*args)
    __swig_getmethods__["scfGetVersion"] = lambda x: _pycscegui.iCEGUI_scfGetVersion
    if _newclass:scfGetVersion = staticmethod(_pycscegui.iCEGUI_scfGetVersion)
    __swig_destroy__ = _pycscegui.delete_iCEGUI
    __del__ = lambda self : None;
    SchemeManager = property(_pycscegui.iCEGUI_GetSchemeManagerPtr)  
    System = property(_pycscegui.iCEGUI_GetSystemPtr)  
    FontManager = property(_pycscegui.iCEGUI_GetFontManagerPtr)  
    GlobalEventSet = property(_pycscegui.iCEGUI_GetGlobalEventSetPtr)  
    ImagesetManager = property(_pycscegui.iCEGUI_GetImagesetManagerPtr)  
    Logger = property(_pycscegui.iCEGUI_GetLoggerPtr)  
    MouseCursor = property(_pycscegui.iCEGUI_GetMouseCursorPtr)  
    WindowFactoryManager = property(_pycscegui.iCEGUI_GetWindowFactoryManagerPtr)  
    WindowManager = property(_pycscegui.iCEGUI_GetWindowManagerPtr)  
iCEGUI_swigregister = _pycscegui.iCEGUI_swigregister
iCEGUI_swigregister(iCEGUI)
iCEGUI_scfGetVersion = _pycscegui.iCEGUI_scfGetVersion

SWIG_BUILD_VERSION = 0x010329

msg_swig_cant_check = """
Warning: Could not check pycegui version, note you might experience
         crashes if pycegui and pycscegui were not built with compatible
         swig versions.
"""
msg_swig_incompatible = """
Warning: pycegui and pycscegui were generated with different swig versions.
         You might experience crashes because of this, or swig might not
         return wrapped cegui pointers from iCEGUI.
"""
try:
	import cegui
	if not hasattr(cegui,"SWIG_BUILD_VERSION"):
		print msg_swig_cant_check
	elif not SWIG_BUILD_VERSION == cegui.SWIG_BUILD_VERSION:
		print msg_swig_incompatible
except:
	print "Warning: You dont seem to have pycegui installed."
	print "         Please install as pycscegui needs this."



