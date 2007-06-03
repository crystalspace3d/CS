# This file was created automatically by SWIG 1.3.28.
# Don't modify this file, modify the SWIG interface instead.

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

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


def _swig_setattr_nondynamic_method(set):
    def set_attr(self,name,value):
        if (name == "thisown"): return self.this.own(value)
        if hasattr(self,name) or (name == "this"):
            set(self,name,value)
        else:
            raise AttributeError("You cannot add attributes to %s" % self)
    return set_attr


import cspace
class iCEGUI(cspace.iBase):
    thisown = property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    def __repr__(self):
        try: strthis = "at 0x%x" %( self.this, ) 
        except: strthis = "" 
        return "<%s.%s; proxy of C++ iCEGUI instance %s>" % (self.__class__.__module__, self.__class__.__name__, strthis,)
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
    __swig_destroy__ = _pycscegui.delete_iCEGUI
    __del__ = lambda self : None;
    scfGetVersion = staticmethod(_pycscegui.iCEGUI_scfGetVersion)
    SchemeManager = property(_pycscegui.iCEGUI_GetSchemeManagerPtr)  
    System = property(_pycscegui.iCEGUI_GetSystemPtr)  
    FontManager = property(_pycscegui.iCEGUI_GetFontManagerPtr)  
    GlobalEventSet = property(_pycscegui.iCEGUI_GetGlobalEventSetPtr)  
    ImagesetManager = property(_pycscegui.iCEGUI_GetImagesetManagerPtr)  
    Logger = property(_pycscegui.iCEGUI_GetLoggerPtr)  
    MouseCursor = property(_pycscegui.iCEGUI_GetMouseCursorPtr)  
    WindowFactoryManager = property(_pycscegui.iCEGUI_GetWindowFactoryManagerPtr)  
    WindowManager = property(_pycscegui.iCEGUI_GetWindowManagerPtr)  
_pycscegui.iCEGUI_swigregister(iCEGUI)

iCEGUI_scfGetVersion = _pycscegui.iCEGUI_scfGetVersion

SWIG_BUILD_VERSION = 0x010328

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



