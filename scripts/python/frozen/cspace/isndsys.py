# This file was automatically generated by SWIG (http://www.swig.org).
# Version 2.0.4
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.



from sys import version_info
if version_info >= (2,6,0):
    def swig_import_helper():
        from os.path import dirname
        import imp
        fp = None
        try:
            fp, pathname, description = imp.find_module('_isndsys', [dirname(__file__)])
        except ImportError:
            import _isndsys
            return _isndsys
        if fp is not None:
            try:
                _mod = imp.load_module('_isndsys', fp, pathname, description)
            finally:
                fp.close()
            return _mod
    _isndsys = swig_import_helper()
    del swig_import_helper
else:
    import _isndsys
del version_info
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'SwigPyObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError(name)

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

try:
    _object = object
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0


def _swig_setattr_nondynamic_method(set):
    def set_attr(self,name,value):
        if (name == "thisown"): return self.this.own(value)
        if hasattr(self,name) or (name == "this"):
            set(self,name,value)
        else:
            raise AttributeError("You cannot add attributes to %s" % self)
    return set_attr


import core

def _SetSCFPointer(*args):
  return _isndsys._SetSCFPointer(*args)
_SetSCFPointer = _isndsys._SetSCFPointer

def _GetSCFPointer():
  return _isndsys._GetSCFPointer()
_GetSCFPointer = _isndsys._GetSCFPointer
if not "core" in dir():
    core = __import__("cspace").__dict__["core"]
core.AddSCFLink(_SetSCFPointer)
CSMutableArrayHelper = core.CSMutableArrayHelper

CS_SNDSYS_DATA_UNKNOWN_SIZE = _isndsys.CS_SNDSYS_DATA_UNKNOWN_SIZE
class iSndSysData(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def GetFormat(self): return _isndsys.iSndSysData_GetFormat(self)
    def GetFrameCount(self): return _isndsys.iSndSysData_GetFrameCount(self)
    def GetDataSize(self): return _isndsys.iSndSysData_GetDataSize(self)
    def CreateStream(self, *args): return _isndsys.iSndSysData_CreateStream(self, *args)
    def SetDescription(self, *args): return _isndsys.iSndSysData_SetDescription(self, *args)
    def GetDescription(self): return _isndsys.iSndSysData_GetDescription(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysData_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysData
    __del__ = lambda self : None;
iSndSysData_swigregister = _isndsys.iSndSysData_swigregister
iSndSysData_swigregister(iSndSysData)

def iSndSysData_scfGetVersion():
  return _isndsys.iSndSysData_scfGetVersion()
iSndSysData_scfGetVersion = _isndsys.iSndSysData_scfGetVersion

class iSndSysSoftwareFilter3DProperties(object):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    clean_buffer = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_clean_buffer_get, _isndsys.iSndSysSoftwareFilter3DProperties_clean_buffer_set)
    work_buffer = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_work_buffer_get, _isndsys.iSndSysSoftwareFilter3DProperties_work_buffer_set)
    buffer_samples = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_buffer_samples_get, _isndsys.iSndSysSoftwareFilter3DProperties_buffer_samples_set)
    source_parameters = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_source_parameters_get, _isndsys.iSndSysSoftwareFilter3DProperties_source_parameters_set)
    listener_parameters = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_listener_parameters_get, _isndsys.iSndSysSoftwareFilter3DProperties_listener_parameters_set)
    sound_format = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_sound_format_get, _isndsys.iSndSysSoftwareFilter3DProperties_sound_format_set)
    closest_speaker_distance = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_closest_speaker_distance_get, _isndsys.iSndSysSoftwareFilter3DProperties_closest_speaker_distance_set)
    speaker_distance = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_speaker_distance_get, _isndsys.iSndSysSoftwareFilter3DProperties_speaker_distance_set)
    speaker_direction_cos = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_speaker_direction_cos_get, _isndsys.iSndSysSoftwareFilter3DProperties_speaker_direction_cos_set)
    channel = _swig_property(_isndsys.iSndSysSoftwareFilter3DProperties_channel_get, _isndsys.iSndSysSoftwareFilter3DProperties_channel_set)
    def __init__(self): 
        this = _isndsys.new_iSndSysSoftwareFilter3DProperties()
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareFilter3DProperties
    __del__ = lambda self : None;
iSndSysSoftwareFilter3DProperties_swigregister = _isndsys.iSndSysSoftwareFilter3DProperties_swigregister
iSndSysSoftwareFilter3DProperties_swigregister(iSndSysSoftwareFilter3DProperties)

class iSndSysSoftwareFilter3D(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def Apply(self, *args): return _isndsys.iSndSysSoftwareFilter3D_Apply(self, *args)
    def AddSubFilter(self, *args): return _isndsys.iSndSysSoftwareFilter3D_AddSubFilter(self, *args)
    def GetSubFilter(self, chain_idx = 0): return _isndsys.iSndSysSoftwareFilter3D_GetSubFilter(self, chain_idx)
    def GetPtr(self): return _isndsys.iSndSysSoftwareFilter3D_GetPtr(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysSoftwareFilter3D_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareFilter3D
    __del__ = lambda self : None;
iSndSysSoftwareFilter3D_swigregister = _isndsys.iSndSysSoftwareFilter3D_swigregister
iSndSysSoftwareFilter3D_swigregister(iSndSysSoftwareFilter3D)

def iSndSysSoftwareFilter3D_scfGetVersion():
  return _isndsys.iSndSysSoftwareFilter3D_scfGetVersion()
iSndSysSoftwareFilter3D_scfGetVersion = _isndsys.iSndSysSoftwareFilter3D_scfGetVersion

class iSndSysSoftwareOutputFilter(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def FormatNotify(self, *args): return _isndsys.iSndSysSoftwareOutputFilter_FormatNotify(self, *args)
    def DeliverData(self, *args): return _isndsys.iSndSysSoftwareOutputFilter_DeliverData(self, *args)
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareOutputFilter
    __del__ = lambda self : None;
iSndSysSoftwareOutputFilter_swigregister = _isndsys.iSndSysSoftwareOutputFilter_swigregister
iSndSysSoftwareOutputFilter_swigregister(iSndSysSoftwareOutputFilter)

SS_FILTER_LOC_RENDEROUT = _isndsys.SS_FILTER_LOC_RENDEROUT
SS_FILTER_LOC_SOURCEOUT = _isndsys.SS_FILTER_LOC_SOURCEOUT
SS_FILTER_LOC_SOURCEIN = _isndsys.SS_FILTER_LOC_SOURCEIN
class iSndSysListener(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetDirection(self, *args): return _isndsys.iSndSysListener_SetDirection(self, *args)
    def SetPosition(self, *args): return _isndsys.iSndSysListener_SetPosition(self, *args)
    def SetDistanceFactor(self, *args): return _isndsys.iSndSysListener_SetDistanceFactor(self, *args)
    def SetRollOffFactor(self, *args): return _isndsys.iSndSysListener_SetRollOffFactor(self, *args)
    def GetDirection(self, *args): return _isndsys.iSndSysListener_GetDirection(self, *args)
    def GetPosition(self): return _isndsys.iSndSysListener_GetPosition(self)
    def GetDistanceFactor(self): return _isndsys.iSndSysListener_GetDistanceFactor(self)
    def GetRollOffFactor(self): return _isndsys.iSndSysListener_GetRollOffFactor(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysListener_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysListener
    __del__ = lambda self : None;
iSndSysListener_swigregister = _isndsys.iSndSysListener_swigregister
iSndSysListener_swigregister(iSndSysListener)

def iSndSysListener_scfGetVersion():
  return _isndsys.iSndSysListener_scfGetVersion()
iSndSysListener_scfGetVersion = _isndsys.iSndSysListener_scfGetVersion

class iSndSysListenerDoppler(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetVelocity(self, *args): return _isndsys.iSndSysListenerDoppler_SetVelocity(self, *args)
    def SetDopplerFactor(self, *args): return _isndsys.iSndSysListenerDoppler_SetDopplerFactor(self, *args)
    def SetSpeedOfSound(self, *args): return _isndsys.iSndSysListenerDoppler_SetSpeedOfSound(self, *args)
    def GetVelocity(self): return _isndsys.iSndSysListenerDoppler_GetVelocity(self)
    def GetDopplerFactor(self): return _isndsys.iSndSysListenerDoppler_GetDopplerFactor(self)
    def GetSpeedOfSound(self): return _isndsys.iSndSysListenerDoppler_GetSpeedOfSound(self)
    __swig_destroy__ = _isndsys.delete_iSndSysListenerDoppler
    __del__ = lambda self : None;
iSndSysListenerDoppler_swigregister = _isndsys.iSndSysListenerDoppler_swigregister
iSndSysListenerDoppler_swigregister(iSndSysListenerDoppler)

class iSndSysLoader(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def LoadSound(self, *args): return _isndsys.iSndSysLoader_LoadSound(self, *args)
    scfGetVersion = staticmethod(_isndsys.iSndSysLoader_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysLoader
    __del__ = lambda self : None;
iSndSysLoader_swigregister = _isndsys.iSndSysLoader_swigregister
iSndSysLoader_swigregister(iSndSysLoader)

def iSndSysLoader_scfGetVersion():
  return _isndsys.iSndSysLoader_scfGetVersion()
iSndSysLoader_scfGetVersion = _isndsys.iSndSysLoader_scfGetVersion

class iSndSysWrapper(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def QueryObject(self): return _isndsys.iSndSysWrapper_QueryObject(self)
    def GetData(self): return _isndsys.iSndSysWrapper_GetData(self)
    def SetData(self, *args): return _isndsys.iSndSysWrapper_SetData(self, *args)
    scfGetVersion = staticmethod(_isndsys.iSndSysWrapper_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysWrapper
    __del__ = lambda self : None;
iSndSysWrapper_swigregister = _isndsys.iSndSysWrapper_swigregister
iSndSysWrapper_swigregister(iSndSysWrapper)

def iSndSysWrapper_scfGetVersion():
  return _isndsys.iSndSysWrapper_scfGetVersion()
iSndSysWrapper_scfGetVersion = _isndsys.iSndSysWrapper_scfGetVersion

class iSndSysManager(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def CreateSound(self, *args): return _isndsys.iSndSysManager_CreateSound(self, *args)
    def RemoveSound(self, *args): return _isndsys.iSndSysManager_RemoveSound(self, *args)
    def RemoveSounds(self): return _isndsys.iSndSysManager_RemoveSounds(self)
    def GetSoundCount(self): return _isndsys.iSndSysManager_GetSoundCount(self)
    def GetSound(self, *args): return _isndsys.iSndSysManager_GetSound(self, *args)
    def FindSoundByName(self, *args): return _isndsys.iSndSysManager_FindSoundByName(self, *args)
    scfGetVersion = staticmethod(_isndsys.iSndSysManager_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysManager
    __del__ = lambda self : None;
iSndSysManager_swigregister = _isndsys.iSndSysManager_swigregister
iSndSysManager_swigregister(iSndSysManager)

def iSndSysManager_scfGetVersion():
  return _isndsys.iSndSysManager_scfGetVersion()
iSndSysManager_scfGetVersion = _isndsys.iSndSysManager_scfGetVersion

CS_SNDSYS_SOURCE_DISTANCE_INFINITE = _isndsys.CS_SNDSYS_SOURCE_DISTANCE_INFINITE
class iSndSysSource(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetVolume(self, *args): return _isndsys.iSndSysSource_SetVolume(self, *args)
    def GetVolume(self): return _isndsys.iSndSysSource_GetVolume(self)
    def GetStream(self): return _isndsys.iSndSysSource_GetStream(self)
    def GetPtr(self): return _isndsys.iSndSysSource_GetPtr(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysSource_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSource
    __del__ = lambda self : None;
iSndSysSource_swigregister = _isndsys.iSndSysSource_swigregister
iSndSysSource_swigregister(iSndSysSource)

def iSndSysSource_scfGetVersion():
  return _isndsys.iSndSysSource_scfGetVersion()
iSndSysSource_scfGetVersion = _isndsys.iSndSysSource_scfGetVersion

class iSndSysSourceSoftware(iSndSysSource):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def MergeIntoBuffer(self, *args): return _isndsys.iSndSysSourceSoftware_MergeIntoBuffer(self, *args)
    def ProcessOutputFilters(self): return _isndsys.iSndSysSourceSoftware_ProcessOutputFilters(self)
    def AddOutputFilter(self, *args): return _isndsys.iSndSysSourceSoftware_AddOutputFilter(self, *args)
    def RemoveOutputFilter(self, *args): return _isndsys.iSndSysSourceSoftware_RemoveOutputFilter(self, *args)
    __swig_destroy__ = _isndsys.delete_iSndSysSourceSoftware
    __del__ = lambda self : None;
iSndSysSourceSoftware_swigregister = _isndsys.iSndSysSourceSoftware_swigregister
iSndSysSourceSoftware_swigregister(iSndSysSourceSoftware)

class iSndSysSourceOpenAL(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    __swig_destroy__ = _isndsys.delete_iSndSysSourceOpenAL
    __del__ = lambda self : None;
iSndSysSourceOpenAL_swigregister = _isndsys.iSndSysSourceOpenAL_swigregister
iSndSysSourceOpenAL_swigregister(iSndSysSourceOpenAL)

class iSndSysSource3D(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetPosition(self, *args): return _isndsys.iSndSysSource3D_SetPosition(self, *args)
    def GetPosition(self): return _isndsys.iSndSysSource3D_GetPosition(self)
    def SetMinimumDistance(self, *args): return _isndsys.iSndSysSource3D_SetMinimumDistance(self, *args)
    def SetMaximumDistance(self, *args): return _isndsys.iSndSysSource3D_SetMaximumDistance(self, *args)
    def GetMinimumDistance(self): return _isndsys.iSndSysSource3D_GetMinimumDistance(self)
    def GetMaximumDistance(self): return _isndsys.iSndSysSource3D_GetMaximumDistance(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysSource3D_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3D
    __del__ = lambda self : None;
iSndSysSource3D_swigregister = _isndsys.iSndSysSource3D_swigregister
iSndSysSource3D_swigregister(iSndSysSource3D)

def iSndSysSource3D_scfGetVersion():
  return _isndsys.iSndSysSource3D_scfGetVersion()
iSndSysSource3D_scfGetVersion = _isndsys.iSndSysSource3D_scfGetVersion

class iSndSysSource3DDirectionalSimple(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetDirection(self, *args): return _isndsys.iSndSysSource3DDirectionalSimple_SetDirection(self, *args)
    def GetDirection(self): return _isndsys.iSndSysSource3DDirectionalSimple_GetDirection(self)
    def SetDirectionalRadiation(self, *args): return _isndsys.iSndSysSource3DDirectionalSimple_SetDirectionalRadiation(self, *args)
    def GetDirectionalRadiation(self): return _isndsys.iSndSysSource3DDirectionalSimple_GetDirectionalRadiation(self)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3DDirectionalSimple
    __del__ = lambda self : None;
iSndSysSource3DDirectionalSimple_swigregister = _isndsys.iSndSysSource3DDirectionalSimple_swigregister
iSndSysSource3DDirectionalSimple_swigregister(iSndSysSource3DDirectionalSimple)

class iSndSysSource3DDirectional(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetDirection(self, *args): return _isndsys.iSndSysSource3DDirectional_SetDirection(self, *args)
    def GetDirection(self): return _isndsys.iSndSysSource3DDirectional_GetDirection(self)
    def SetDirectionalRadiationInnerCone(self, *args): return _isndsys.iSndSysSource3DDirectional_SetDirectionalRadiationInnerCone(self, *args)
    def SetDirectionalRadiationOuterCone(self, *args): return _isndsys.iSndSysSource3DDirectional_SetDirectionalRadiationOuterCone(self, *args)
    def SetDirectionalRadiationOuterGain(self, *args): return _isndsys.iSndSysSource3DDirectional_SetDirectionalRadiationOuterGain(self, *args)
    def GetDirectionalRadiationInnerCone(self): return _isndsys.iSndSysSource3DDirectional_GetDirectionalRadiationInnerCone(self)
    def GetDirectionalRadiationOuterCone(self): return _isndsys.iSndSysSource3DDirectional_GetDirectionalRadiationOuterCone(self)
    def GetDirectionalRadiationOuterGain(self): return _isndsys.iSndSysSource3DDirectional_GetDirectionalRadiationOuterGain(self)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3DDirectional
    __del__ = lambda self : None;
iSndSysSource3DDirectional_swigregister = _isndsys.iSndSysSource3DDirectional_swigregister
iSndSysSource3DDirectional_swigregister(iSndSysSource3DDirectional)

class iSndSysSource3DDoppler(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetVelocity(self, *args): return _isndsys.iSndSysSource3DDoppler_SetVelocity(self, *args)
    def GetVelocity(self): return _isndsys.iSndSysSource3DDoppler_GetVelocity(self)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3DDoppler
    __del__ = lambda self : None;
iSndSysSource3DDoppler_swigregister = _isndsys.iSndSysSource3DDoppler_swigregister
iSndSysSource3DDoppler_swigregister(iSndSysSource3DDoppler)

CSSNDSYS_SAMPLE_LITTLE_ENDIAN = _isndsys.CSSNDSYS_SAMPLE_LITTLE_ENDIAN
CSSNDSYS_SAMPLE_BIG_ENDIAN = _isndsys.CSSNDSYS_SAMPLE_BIG_ENDIAN
CSSNDSYS_SAMPLE_ENDIAN_MASK = _isndsys.CSSNDSYS_SAMPLE_ENDIAN_MASK
class csSndSysSoundFormat(object):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    Freq = _swig_property(_isndsys.csSndSysSoundFormat_Freq_get, _isndsys.csSndSysSoundFormat_Freq_set)
    Bits = _swig_property(_isndsys.csSndSysSoundFormat_Bits_get, _isndsys.csSndSysSoundFormat_Bits_set)
    Channels = _swig_property(_isndsys.csSndSysSoundFormat_Channels_get, _isndsys.csSndSysSoundFormat_Channels_set)
    Flags = _swig_property(_isndsys.csSndSysSoundFormat_Flags_get, _isndsys.csSndSysSoundFormat_Flags_set)
    def __init__(self): 
        this = _isndsys.new_csSndSysSoundFormat()
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _isndsys.delete_csSndSysSoundFormat
    __del__ = lambda self : None;
csSndSysSoundFormat_swigregister = _isndsys.csSndSysSoundFormat_swigregister
csSndSysSoundFormat_swigregister(csSndSysSoundFormat)

CS_SNDSYS_STREAM_PAUSED = _isndsys.CS_SNDSYS_STREAM_PAUSED
CS_SNDSYS_STREAM_UNPAUSED = _isndsys.CS_SNDSYS_STREAM_UNPAUSED
CS_SNDSYS_STREAM_COMPLETED = _isndsys.CS_SNDSYS_STREAM_COMPLETED
CS_SNDSYS_STREAM_DONTLOOP = _isndsys.CS_SNDSYS_STREAM_DONTLOOP
CS_SNDSYS_STREAM_LOOP = _isndsys.CS_SNDSYS_STREAM_LOOP
CS_SND3D_DISABLE = _isndsys.CS_SND3D_DISABLE
CS_SND3D_RELATIVE = _isndsys.CS_SND3D_RELATIVE
CS_SND3D_ABSOLUTE = _isndsys.CS_SND3D_ABSOLUTE
class iSndSysStream(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def GetDescription(self): return _isndsys.iSndSysStream_GetDescription(self)
    def GetRenderedFormat(self): return _isndsys.iSndSysStream_GetRenderedFormat(self)
    def Get3dMode(self): return _isndsys.iSndSysStream_Get3dMode(self)
    def GetFrameCount(self): return _isndsys.iSndSysStream_GetFrameCount(self)
    def GetPosition(self): return _isndsys.iSndSysStream_GetPosition(self)
    def ResetPosition(self): return _isndsys.iSndSysStream_ResetPosition(self)
    def SetPosition(self, *args): return _isndsys.iSndSysStream_SetPosition(self, *args)
    def Pause(self): return _isndsys.iSndSysStream_Pause(self)
    def Unpause(self): return _isndsys.iSndSysStream_Unpause(self)
    def GetPauseState(self): return _isndsys.iSndSysStream_GetPauseState(self)
    def SetLoopState(self, *args): return _isndsys.iSndSysStream_SetLoopState(self, *args)
    def GetLoopState(self): return _isndsys.iSndSysStream_GetLoopState(self)
    def SetPlayRatePercent(self, *args): return _isndsys.iSndSysStream_SetPlayRatePercent(self, *args)
    def GetPlayRatePercent(self): return _isndsys.iSndSysStream_GetPlayRatePercent(self)
    def SetAutoUnregister(self, *args): return _isndsys.iSndSysStream_SetAutoUnregister(self, *args)
    def GetAutoUnregister(self): return _isndsys.iSndSysStream_GetAutoUnregister(self)
    def GetAutoUnregisterRequested(self): return _isndsys.iSndSysStream_GetAutoUnregisterRequested(self)
    def AdvancePosition(self, *args): return _isndsys.iSndSysStream_AdvancePosition(self, *args)
    def GetDataPointers(self, *args): return _isndsys.iSndSysStream_GetDataPointers(self, *args)
    def InitializeSourcePositionMarker(self, *args): return _isndsys.iSndSysStream_InitializeSourcePositionMarker(self, *args)
    def ProcessNotifications(self): return _isndsys.iSndSysStream_ProcessNotifications(self)
    def RegisterCallback(self, *args): return _isndsys.iSndSysStream_RegisterCallback(self, *args)
    def UnregisterCallback(self, *args): return _isndsys.iSndSysStream_UnregisterCallback(self, *args)
    def RegisterFrameNotification(self, *args): return _isndsys.iSndSysStream_RegisterFrameNotification(self, *args)
    def AlwaysStream(self): return _isndsys.iSndSysStream_AlwaysStream(self)
    def GetLoopStart(self): return _isndsys.iSndSysStream_GetLoopStart(self)
    def GetLoopEnd(self): return _isndsys.iSndSysStream_GetLoopEnd(self)
    def SetLoopBoundaries(self, *args): return _isndsys.iSndSysStream_SetLoopBoundaries(self, *args)
    def PendingSeek(self): return _isndsys.iSndSysStream_PendingSeek(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysStream_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysStream
    __del__ = lambda self : None;
iSndSysStream_swigregister = _isndsys.iSndSysStream_swigregister
iSndSysStream_swigregister(iSndSysStream)
cvar = _isndsys.cvar
CS_SNDSYS_STREAM_UNKNOWN_LENGTH = cvar.CS_SNDSYS_STREAM_UNKNOWN_LENGTH

def iSndSysStream_scfGetVersion():
  return _isndsys.iSndSysStream_scfGetVersion()
iSndSysStream_scfGetVersion = _isndsys.iSndSysStream_scfGetVersion

class iSndSysStreamCallback(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def StreamLoopNotification(self): return _isndsys.iSndSysStreamCallback_StreamLoopNotification(self)
    def StreamPauseNotification(self): return _isndsys.iSndSysStreamCallback_StreamPauseNotification(self)
    def StreamUnpauseNotification(self): return _isndsys.iSndSysStreamCallback_StreamUnpauseNotification(self)
    def StreamFrameNotification(self, *args): return _isndsys.iSndSysStreamCallback_StreamFrameNotification(self, *args)
    __swig_destroy__ = _isndsys.delete_iSndSysStreamCallback
    __del__ = lambda self : None;
iSndSysStreamCallback_swigregister = _isndsys.iSndSysStreamCallback_swigregister
iSndSysStreamCallback_swigregister(iSndSysStreamCallback)

CS_SNDSYS_SOURCE_STOPPED = _isndsys.CS_SNDSYS_SOURCE_STOPPED
CS_SNDSYS_SOURCE_PLAYING = _isndsys.CS_SNDSYS_SOURCE_PLAYING
class iSndSysRenderer(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def SetVolume(self, *args): return _isndsys.iSndSysRenderer_SetVolume(self, *args)
    def GetVolume(self): return _isndsys.iSndSysRenderer_GetVolume(self)
    def CreateStream(self, *args): return _isndsys.iSndSysRenderer_CreateStream(self, *args)
    def CreateSource(self, *args): return _isndsys.iSndSysRenderer_CreateSource(self, *args)
    def RemoveStream(self, *args): return _isndsys.iSndSysRenderer_RemoveStream(self, *args)
    def RemoveSource(self, *args): return _isndsys.iSndSysRenderer_RemoveSource(self, *args)
    def GetListener(self): return _isndsys.iSndSysRenderer_GetListener(self)
    def RegisterCallback(self, *args): return _isndsys.iSndSysRenderer_RegisterCallback(self, *args)
    def UnregisterCallback(self, *args): return _isndsys.iSndSysRenderer_UnregisterCallback(self, *args)
    scfGetVersion = staticmethod(_isndsys.iSndSysRenderer_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysRenderer
    __del__ = lambda self : None;
iSndSysRenderer_swigregister = _isndsys.iSndSysRenderer_swigregister
iSndSysRenderer_swigregister(iSndSysRenderer)

def iSndSysRenderer_scfGetVersion():
  return _isndsys.iSndSysRenderer_scfGetVersion()
iSndSysRenderer_scfGetVersion = _isndsys.iSndSysRenderer_scfGetVersion

class iSndSysRendererCallback(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def StreamAddNotification(self, *args): return _isndsys.iSndSysRendererCallback_StreamAddNotification(self, *args)
    def StreamRemoveNotification(self, *args): return _isndsys.iSndSysRendererCallback_StreamRemoveNotification(self, *args)
    def SourceAddNotification(self, *args): return _isndsys.iSndSysRendererCallback_SourceAddNotification(self, *args)
    def SourceRemoveNotification(self, *args): return _isndsys.iSndSysRendererCallback_SourceRemoveNotification(self, *args)
    __swig_destroy__ = _isndsys.delete_iSndSysRendererCallback
    __del__ = lambda self : None;
iSndSysRendererCallback_swigregister = _isndsys.iSndSysRendererCallback_swigregister
iSndSysRendererCallback_swigregister(iSndSysRendererCallback)

class iSndSysRendererSoftware(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def AddOutputFilter(self, *args): return _isndsys.iSndSysRendererSoftware_AddOutputFilter(self, *args)
    def RemoveOutputFilter(self, *args): return _isndsys.iSndSysRendererSoftware_RemoveOutputFilter(self, *args)
    __swig_destroy__ = _isndsys.delete_iSndSysRendererSoftware
    __del__ = lambda self : None;
iSndSysRendererSoftware_swigregister = _isndsys.iSndSysRendererSoftware_swigregister
iSndSysRendererSoftware_swigregister(iSndSysRendererSoftware)

class iSndSysRendererOpenAL(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def LockWait(self): return _isndsys.iSndSysRendererOpenAL_LockWait(self)
    def Release(self): return _isndsys.iSndSysRendererOpenAL_Release(self)
    __swig_destroy__ = _isndsys.delete_iSndSysRendererOpenAL
    __del__ = lambda self : None;
iSndSysRendererOpenAL_swigregister = _isndsys.iSndSysRendererOpenAL_swigregister
iSndSysRendererOpenAL_swigregister(iSndSysRendererOpenAL)

class iSndSysSoftwareDriver(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self, *args, **kwargs): raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    def Open(self, *args): return _isndsys.iSndSysSoftwareDriver_Open(self, *args)
    def Close(self): return _isndsys.iSndSysSoftwareDriver_Close(self)
    def StartThread(self): return _isndsys.iSndSysSoftwareDriver_StartThread(self)
    def StopThread(self): return _isndsys.iSndSysSoftwareDriver_StopThread(self)
    scfGetVersion = staticmethod(_isndsys.iSndSysSoftwareDriver_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareDriver
    __del__ = lambda self : None;
iSndSysSoftwareDriver_swigregister = _isndsys.iSndSysSoftwareDriver_swigregister
iSndSysSoftwareDriver_swigregister(iSndSysSoftwareDriver)

def iSndSysSoftwareDriver_scfGetVersion():
  return _isndsys.iSndSysSoftwareDriver_scfGetVersion()
iSndSysSoftwareDriver_scfGetVersion = _isndsys.iSndSysSoftwareDriver_scfGetVersion



