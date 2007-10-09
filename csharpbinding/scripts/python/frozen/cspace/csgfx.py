# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.31
#
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _csgfx
import new
new_instancemethod = new.instancemethod
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
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


import core
_SetSCFPointer = _csgfx._SetSCFPointer
_GetSCFPointer = _csgfx._GetSCFPointer
if not "core" in dir():
    core = __import__("cspace").__dict__["core"]
core.AddSCFLink(_SetSCFPointer)
CSMutableArrayHelper = core.CSMutableArrayHelper

class csRGBcolor(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, csRGBcolor, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, csRGBcolor, name)
    __repr__ = _swig_repr
    __swig_setmethods__["red"] = _csgfx.csRGBcolor_red_set
    __swig_getmethods__["red"] = _csgfx.csRGBcolor_red_get
    if _newclass:red = _swig_property(_csgfx.csRGBcolor_red_get, _csgfx.csRGBcolor_red_set)
    __swig_setmethods__["green"] = _csgfx.csRGBcolor_green_set
    __swig_getmethods__["green"] = _csgfx.csRGBcolor_green_get
    if _newclass:green = _swig_property(_csgfx.csRGBcolor_green_get, _csgfx.csRGBcolor_green_set)
    __swig_setmethods__["blue"] = _csgfx.csRGBcolor_blue_set
    __swig_getmethods__["blue"] = _csgfx.csRGBcolor_blue_get
    if _newclass:blue = _swig_property(_csgfx.csRGBcolor_blue_get, _csgfx.csRGBcolor_blue_set)
    def __init__(self, *args): 
        this = _csgfx.new_csRGBcolor(*args)
        try: self.this.append(this)
        except: self.this = this
    def Set(*args): return _csgfx.csRGBcolor_Set(*args)
    def __eq__(*args): return _csgfx.csRGBcolor___eq__(*args)
    def __ne__(*args): return _csgfx.csRGBcolor___ne__(*args)
    def __add__(*args): return _csgfx.csRGBcolor___add__(*args)
    def UnsafeAdd(*args): return _csgfx.csRGBcolor_UnsafeAdd(*args)
    def SafeAdd(*args): return _csgfx.csRGBcolor_SafeAdd(*args)
    __swig_destroy__ = _csgfx.delete_csRGBcolor
    __del__ = lambda self : None;
csRGBcolor_swigregister = _csgfx.csRGBcolor_swigregister
csRGBcolor_swigregister(csRGBcolor)

class csRGBpixel(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, csRGBpixel, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, csRGBpixel, name)
    __repr__ = _swig_repr
    __swig_setmethods__["red"] = _csgfx.csRGBpixel_red_set
    __swig_getmethods__["red"] = _csgfx.csRGBpixel_red_get
    if _newclass:red = _swig_property(_csgfx.csRGBpixel_red_get, _csgfx.csRGBpixel_red_set)
    __swig_setmethods__["green"] = _csgfx.csRGBpixel_green_set
    __swig_getmethods__["green"] = _csgfx.csRGBpixel_green_get
    if _newclass:green = _swig_property(_csgfx.csRGBpixel_green_get, _csgfx.csRGBpixel_green_set)
    __swig_setmethods__["blue"] = _csgfx.csRGBpixel_blue_set
    __swig_getmethods__["blue"] = _csgfx.csRGBpixel_blue_get
    if _newclass:blue = _swig_property(_csgfx.csRGBpixel_blue_get, _csgfx.csRGBpixel_blue_set)
    __swig_setmethods__["alpha"] = _csgfx.csRGBpixel_alpha_set
    __swig_getmethods__["alpha"] = _csgfx.csRGBpixel_alpha_get
    if _newclass:alpha = _swig_property(_csgfx.csRGBpixel_alpha_get, _csgfx.csRGBpixel_alpha_set)
    def __init__(self, *args): 
        this = _csgfx.new_csRGBpixel(*args)
        try: self.this.append(this)
        except: self.this = this
    def __eq__(*args): return _csgfx.csRGBpixel___eq__(*args)
    def __ne__(*args): return _csgfx.csRGBpixel___ne__(*args)
    def asRGBcolor(*args): return _csgfx.csRGBpixel_asRGBcolor(*args)
    def eq(*args): return _csgfx.csRGBpixel_eq(*args)
    def Intensity(*args): return _csgfx.csRGBpixel_Intensity(*args)
    def Luminance(*args): return _csgfx.csRGBpixel_Luminance(*args)
    def Set(*args): return _csgfx.csRGBpixel_Set(*args)
    def __iadd__(*args): return _csgfx.csRGBpixel___iadd__(*args)
    def UnsafeAdd(*args): return _csgfx.csRGBpixel_UnsafeAdd(*args)
    def SafeAdd(*args): return _csgfx.csRGBpixel_SafeAdd(*args)
    __swig_destroy__ = _csgfx.delete_csRGBpixel
    __del__ = lambda self : None;
csRGBpixel_swigregister = _csgfx.csRGBpixel_swigregister
csRGBpixel_swigregister(csRGBpixel)

R_COEF = _csgfx.R_COEF
G_COEF = _csgfx.G_COEF
B_COEF = _csgfx.B_COEF
R_COEF_SQ = _csgfx.R_COEF_SQ
G_COEF_SQ = _csgfx.G_COEF_SQ
B_COEF_SQ = _csgfx.B_COEF_SQ
class iShaderVariableAccessor(core.iBase):
    __swig_setmethods__ = {}
    for _s in [core.iBase]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, iShaderVariableAccessor, name, value)
    __swig_getmethods__ = {}
    for _s in [core.iBase]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, iShaderVariableAccessor, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def PreGetValue(*args): return _csgfx.iShaderVariableAccessor_PreGetValue(*args)
    __swig_destroy__ = _csgfx.delete_iShaderVariableAccessor
    __del__ = lambda self : None;
iShaderVariableAccessor_swigregister = _csgfx.iShaderVariableAccessor_swigregister
iShaderVariableAccessor_swigregister(iShaderVariableAccessor)

class csShaderVariable(core.csRefCount):
    __swig_setmethods__ = {}
    for _s in [core.csRefCount]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csShaderVariable, name, value)
    __swig_getmethods__ = {}
    for _s in [core.csRefCount]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csShaderVariable, name)
    __repr__ = _swig_repr
    UNKNOWN = _csgfx.csShaderVariable_UNKNOWN
    INT = _csgfx.csShaderVariable_INT
    FLOAT = _csgfx.csShaderVariable_FLOAT
    TEXTURE = _csgfx.csShaderVariable_TEXTURE
    RENDERBUFFER = _csgfx.csShaderVariable_RENDERBUFFER
    VECTOR2 = _csgfx.csShaderVariable_VECTOR2
    VECTOR3 = _csgfx.csShaderVariable_VECTOR3
    VECTOR4 = _csgfx.csShaderVariable_VECTOR4
    MATRIX = _csgfx.csShaderVariable_MATRIX
    TRANSFORM = _csgfx.csShaderVariable_TRANSFORM
    ARRAY = _csgfx.csShaderVariable_ARRAY
    COLOR = _csgfx.csShaderVariable_COLOR
    def __init__(self, *args): 
        this = _csgfx.new_csShaderVariable(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _csgfx.delete_csShaderVariable
    __del__ = lambda self : None;
    def assign(*args): return _csgfx.csShaderVariable_assign(*args)
    def GetType(*args): return _csgfx.csShaderVariable_GetType(*args)
    def SetType(*args): return _csgfx.csShaderVariable_SetType(*args)
    def SetAccessor(*args): return _csgfx.csShaderVariable_SetAccessor(*args)
    def SetName(*args): return _csgfx.csShaderVariable_SetName(*args)
    def GetName(*args): return _csgfx.csShaderVariable_GetName(*args)
    def SetValue(*args): return _csgfx.csShaderVariable_SetValue(*args)
    def AddVariableToArray(*args): return _csgfx.csShaderVariable_AddVariableToArray(*args)
    def RemoveFromArray(*args): return _csgfx.csShaderVariable_RemoveFromArray(*args)
    def SetArraySize(*args): return _csgfx.csShaderVariable_SetArraySize(*args)
    def GetArraySize(*args): return _csgfx.csShaderVariable_GetArraySize(*args)
    def GetArrayElement(*args): return _csgfx.csShaderVariable_GetArrayElement(*args)
    def SetArrayElement(*args): return _csgfx.csShaderVariable_SetArrayElement(*args)
    def GetValue(*args): return _csgfx.csShaderVariable_GetValue(*args)
csShaderVariable_swigregister = _csgfx.csShaderVariable_swigregister
csShaderVariable_swigregister(csShaderVariable)

class csShaderVariableArrayReadOnly(core.iBase):
    __swig_setmethods__ = {}
    for _s in [core.iBase]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csShaderVariableArrayReadOnly, name, value)
    __swig_getmethods__ = {}
    for _s in [core.iBase]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csShaderVariableArrayReadOnly, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def GetSize(*args): return _csgfx.csShaderVariableArrayReadOnly_GetSize(*args)
    def Get(*args): return _csgfx.csShaderVariableArrayReadOnly_Get(*args)
    def Top(*args): return _csgfx.csShaderVariableArrayReadOnly_Top(*args)
    def Find(*args): return _csgfx.csShaderVariableArrayReadOnly_Find(*args)
    def GetIndex(*args): return _csgfx.csShaderVariableArrayReadOnly_GetIndex(*args)
    def IsEmpty(*args): return _csgfx.csShaderVariableArrayReadOnly_IsEmpty(*args)
    def GetAll(*args): return _csgfx.csShaderVariableArrayReadOnly_GetAll(*args)
    __swig_destroy__ = _csgfx.delete_csShaderVariableArrayReadOnly
    __del__ = lambda self : None;
csShaderVariableArrayReadOnly_swigregister = _csgfx.csShaderVariableArrayReadOnly_swigregister
csShaderVariableArrayReadOnly_swigregister(csShaderVariableArrayReadOnly)

class csShaderVariableArrayChangeElements(csShaderVariableArrayReadOnly):
    __swig_setmethods__ = {}
    for _s in [csShaderVariableArrayReadOnly]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csShaderVariableArrayChangeElements, name, value)
    __swig_getmethods__ = {}
    for _s in [csShaderVariableArrayReadOnly]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csShaderVariableArrayChangeElements, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def Get(*args): return _csgfx.csShaderVariableArrayChangeElements_Get(*args)
    def Top(*args): return _csgfx.csShaderVariableArrayChangeElements_Top(*args)
    __swig_destroy__ = _csgfx.delete_csShaderVariableArrayChangeElements
    __del__ = lambda self : None;
csShaderVariableArrayChangeElements_swigregister = _csgfx.csShaderVariableArrayChangeElements_swigregister
csShaderVariableArrayChangeElements_swigregister(csShaderVariableArrayChangeElements)

class csShaderVariableArrayChangeAll(csShaderVariableArrayChangeElements):
    __swig_setmethods__ = {}
    for _s in [csShaderVariableArrayChangeElements]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csShaderVariableArrayChangeAll, name, value)
    __swig_getmethods__ = {}
    for _s in [csShaderVariableArrayChangeElements]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csShaderVariableArrayChangeAll, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetSize(*args): return _csgfx.csShaderVariableArrayChangeAll_SetSize(*args)
    def GetExtend(*args): return _csgfx.csShaderVariableArrayChangeAll_GetExtend(*args)
    def Put(*args): return _csgfx.csShaderVariableArrayChangeAll_Put(*args)
    def Push(*args): return _csgfx.csShaderVariableArrayChangeAll_Push(*args)
    def PushSmart(*args): return _csgfx.csShaderVariableArrayChangeAll_PushSmart(*args)
    def Pop(*args): return _csgfx.csShaderVariableArrayChangeAll_Pop(*args)
    def Insert(*args): return _csgfx.csShaderVariableArrayChangeAll_Insert(*args)
    def DeleteAll(*args): return _csgfx.csShaderVariableArrayChangeAll_DeleteAll(*args)
    def Truncate(*args): return _csgfx.csShaderVariableArrayChangeAll_Truncate(*args)
    def Empty(*args): return _csgfx.csShaderVariableArrayChangeAll_Empty(*args)
    def DeleteIndex(*args): return _csgfx.csShaderVariableArrayChangeAll_DeleteIndex(*args)
    def DeleteIndexFast(*args): return _csgfx.csShaderVariableArrayChangeAll_DeleteIndexFast(*args)
    def Delete(*args): return _csgfx.csShaderVariableArrayChangeAll_Delete(*args)
    __swig_destroy__ = _csgfx.delete_csShaderVariableArrayChangeAll
    __del__ = lambda self : None;
csShaderVariableArrayChangeAll_swigregister = _csgfx.csShaderVariableArrayChangeAll_swigregister
csShaderVariableArrayChangeAll_swigregister(csShaderVariableArrayChangeAll)

class csImageBaseBase(core.iImage):
    __swig_setmethods__ = {}
    for _s in [core.iImage]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csImageBaseBase, name, value)
    __swig_getmethods__ = {}
    for _s in [core.iImage]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csImageBaseBase, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def IncRef(*args): return _csgfx.csImageBaseBase_IncRef(*args)
    def DecRef(*args): return _csgfx.csImageBaseBase_DecRef(*args)
    def GetRefCount(*args): return _csgfx.csImageBaseBase_GetRefCount(*args)
    def QueryInterface(*args): return _csgfx.csImageBaseBase_QueryInterface(*args)
    def AddRefOwner(*args): return _csgfx.csImageBaseBase_AddRefOwner(*args)
    def RemoveRefOwner(*args): return _csgfx.csImageBaseBase_RemoveRefOwner(*args)
    def GetInterfaceMetadata(*args): return _csgfx.csImageBaseBase_GetInterfaceMetadata(*args)
csImageBaseBase_swigregister = _csgfx.csImageBaseBase_swigregister
csImageBaseBase_swigregister(csImageBaseBase)

class csImageBase(csImageBaseBase):
    __swig_setmethods__ = {}
    for _s in [csImageBaseBase]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csImageBase, name, value)
    __swig_getmethods__ = {}
    for _s in [csImageBaseBase]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csImageBase, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    __swig_destroy__ = _csgfx.delete_csImageBase
    __del__ = lambda self : None;
    def GetDepth(*args): return _csgfx.csImageBase_GetDepth(*args)
    def SetName(*args): return _csgfx.csImageBase_SetName(*args)
    def GetName(*args): return _csgfx.csImageBase_GetName(*args)
    def GetPalette(*args): return _csgfx.csImageBase_GetPalette(*args)
    def GetAlpha(*args): return _csgfx.csImageBase_GetAlpha(*args)
    def HasKeyColor(*args): return _csgfx.csImageBase_HasKeyColor(*args)
    def GetKeyColor(*args): return _csgfx.csImageBase_GetKeyColor(*args)
    def HasMipmaps(*args): return _csgfx.csImageBase_HasMipmaps(*args)
    def GetMipmap(*args): return _csgfx.csImageBase_GetMipmap(*args)
    def GetRawFormat(*args): return _csgfx.csImageBase_GetRawFormat(*args)
    def GetRawData(*args): return _csgfx.csImageBase_GetRawData(*args)
    def GetImageType(*args): return _csgfx.csImageBase_GetImageType(*args)
    def HasSubImages(*args): return _csgfx.csImageBase_HasSubImages(*args)
    def GetSubImage(*args): return _csgfx.csImageBase_GetSubImage(*args)
csImageBase_swigregister = _csgfx.csImageBase_swigregister
csImageBase_swigregister(csImageBase)

class csImageMemoryBase(csImageBase):
    __swig_setmethods__ = {}
    for _s in [csImageBase]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csImageMemoryBase, name, value)
    __swig_getmethods__ = {}
    for _s in [csImageBase]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csImageMemoryBase, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def IncRef(*args): return _csgfx.csImageMemoryBase_IncRef(*args)
    def DecRef(*args): return _csgfx.csImageMemoryBase_DecRef(*args)
    def GetRefCount(*args): return _csgfx.csImageMemoryBase_GetRefCount(*args)
    def QueryInterface(*args): return _csgfx.csImageMemoryBase_QueryInterface(*args)
    def AddRefOwner(*args): return _csgfx.csImageMemoryBase_AddRefOwner(*args)
    def RemoveRefOwner(*args): return _csgfx.csImageMemoryBase_RemoveRefOwner(*args)
    def GetInterfaceMetadata(*args): return _csgfx.csImageMemoryBase_GetInterfaceMetadata(*args)
csImageMemoryBase_swigregister = _csgfx.csImageMemoryBase_swigregister
csImageMemoryBase_swigregister(csImageMemoryBase)

class csImageMemory(csImageMemoryBase):
    __swig_setmethods__ = {}
    for _s in [csImageMemoryBase]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, csImageMemory, name, value)
    __swig_getmethods__ = {}
    for _s in [csImageMemoryBase]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, csImageMemory, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _csgfx.new_csImageMemory(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _csgfx.delete_csImageMemory
    __del__ = lambda self : None;
    def GetImagePtr(*args): return _csgfx.csImageMemory_GetImagePtr(*args)
    def GetPalettePtr(*args): return _csgfx.csImageMemory_GetPalettePtr(*args)
    def GetAlphaPtr(*args): return _csgfx.csImageMemory_GetAlphaPtr(*args)
    def GetImageData(*args): return _csgfx.csImageMemory_GetImageData(*args)
    def GetWidth(*args): return _csgfx.csImageMemory_GetWidth(*args)
    def GetHeight(*args): return _csgfx.csImageMemory_GetHeight(*args)
    def GetDepth(*args): return _csgfx.csImageMemory_GetDepth(*args)
    def GetRawFormat(*args): return _csgfx.csImageMemory_GetRawFormat(*args)
    def GetRawData(*args): return _csgfx.csImageMemory_GetRawData(*args)
    def GetFormat(*args): return _csgfx.csImageMemory_GetFormat(*args)
    def GetPalette(*args): return _csgfx.csImageMemory_GetPalette(*args)
    def GetAlpha(*args): return _csgfx.csImageMemory_GetAlpha(*args)
    def HasKeyColor(*args): return _csgfx.csImageMemory_HasKeyColor(*args)
    def GetKeyColor(*args): return _csgfx.csImageMemory_GetKeyColor(*args)
    def Clear(*args): return _csgfx.csImageMemory_Clear(*args)
    def CheckAlpha(*args): return _csgfx.csImageMemory_CheckAlpha(*args)
    def SetFormat(*args): return _csgfx.csImageMemory_SetFormat(*args)
    def SetKeyColor(*args): return _csgfx.csImageMemory_SetKeyColor(*args)
    def SetKeycolor(*args): return _csgfx.csImageMemory_SetKeycolor(*args)
    def ClearKeyColor(*args): return _csgfx.csImageMemory_ClearKeyColor(*args)
    def ClearKeycolor(*args): return _csgfx.csImageMemory_ClearKeycolor(*args)
    def ApplyKeyColor(*args): return _csgfx.csImageMemory_ApplyKeyColor(*args)
    def ApplyKeycolor(*args): return _csgfx.csImageMemory_ApplyKeycolor(*args)
    def GetImageType(*args): return _csgfx.csImageMemory_GetImageType(*args)
    def SetImageType(*args): return _csgfx.csImageMemory_SetImageType(*args)
    def HasMipmaps(*args): return _csgfx.csImageMemory_HasMipmaps(*args)
    def GetMipmap(*args): return _csgfx.csImageMemory_GetMipmap(*args)
    def SetMipmap(*args): return _csgfx.csImageMemory_SetMipmap(*args)
    def Copy(*args): return _csgfx.csImageMemory_Copy(*args)
    def CopyScale(*args): return _csgfx.csImageMemory_CopyScale(*args)
    def CopyTile(*args): return _csgfx.csImageMemory_CopyTile(*args)
    def ConvertFromRGBA(*args): return _csgfx.csImageMemory_ConvertFromRGBA(*args)
    def ConvertFromPal8(*args): return _csgfx.csImageMemory_ConvertFromPal8(*args)
csImageMemory_swigregister = _csgfx.csImageMemory_swigregister
csImageMemory_swigregister(csImageMemory)



