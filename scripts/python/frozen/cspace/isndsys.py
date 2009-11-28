# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.33
#
# Don't modify this file, modify the SWIG interface instead.

import _isndsys
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


def _swig_setattr_nondynamic_method(set):
    def set_attr(self,name,value):
        if (name == "thisown"): return self.this.own(value)
        if hasattr(self,name) or (name == "this"):
            set(self,name,value)
        else:
            raise AttributeError("You cannot add attributes to %s" % self)
    return set_attr


import core
_SetSCFPointer = _isndsys._SetSCFPointer
_GetSCFPointer = _isndsys._GetSCFPointer
if not "core" in dir():
    core = __import__("cspace").__dict__["core"]
core.AddSCFLink(_SetSCFPointer)
CSMutableArrayHelper = core.CSMutableArrayHelper

CS_SNDSYS_DATA_UNKNOWN_SIZE = _isndsys.CS_SNDSYS_DATA_UNKNOWN_SIZE
class iSndSysData(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def GetFormat(*args): return _isndsys.iSndSysData_GetFormat(*args)
    def GetFrameCount(*args): return _isndsys.iSndSysData_GetFrameCount(*args)
    def GetDataSize(*args): return _isndsys.iSndSysData_GetDataSize(*args)
    def CreateStream(*args): return _isndsys.iSndSysData_CreateStream(*args)
    def SetDescription(*args): return _isndsys.iSndSysData_SetDescription(*args)
    def GetDescription(*args): return _isndsys.iSndSysData_GetDescription(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysData_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysData
    __del__ = lambda self : None;
iSndSysData_swigregister = _isndsys.iSndSysData_swigregister
iSndSysData_swigregister(iSndSysData)
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
    def __init__(self, *args): 
        this = _isndsys.new_iSndSysSoftwareFilter3DProperties(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareFilter3DProperties
    __del__ = lambda self : None;
iSndSysSoftwareFilter3DProperties_swigregister = _isndsys.iSndSysSoftwareFilter3DProperties_swigregister
iSndSysSoftwareFilter3DProperties_swigregister(iSndSysSoftwareFilter3DProperties)

class iSndSysSoftwareFilter3D(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def Apply(*args): return _isndsys.iSndSysSoftwareFilter3D_Apply(*args)
    def AddSubFilter(*args): return _isndsys.iSndSysSoftwareFilter3D_AddSubFilter(*args)
    def GetSubFilter(*args): return _isndsys.iSndSysSoftwareFilter3D_GetSubFilter(*args)
    def GetPtr(*args): return _isndsys.iSndSysSoftwareFilter3D_GetPtr(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysSoftwareFilter3D_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareFilter3D
    __del__ = lambda self : None;
iSndSysSoftwareFilter3D_swigregister = _isndsys.iSndSysSoftwareFilter3D_swigregister
iSndSysSoftwareFilter3D_swigregister(iSndSysSoftwareFilter3D)
iSndSysSoftwareFilter3D_scfGetVersion = _isndsys.iSndSysSoftwareFilter3D_scfGetVersion

class iSndSysSoftwareOutputFilter(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def FormatNotify(*args): return _isndsys.iSndSysSoftwareOutputFilter_FormatNotify(*args)
    def DeliverData(*args): return _isndsys.iSndSysSoftwareOutputFilter_DeliverData(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareOutputFilter
    __del__ = lambda self : None;
iSndSysSoftwareOutputFilter_swigregister = _isndsys.iSndSysSoftwareOutputFilter_swigregister
iSndSysSoftwareOutputFilter_swigregister(iSndSysSoftwareOutputFilter)

SS_FILTER_LOC_RENDEROUT = _isndsys.SS_FILTER_LOC_RENDEROUT
SS_FILTER_LOC_SOURCEOUT = _isndsys.SS_FILTER_LOC_SOURCEOUT
SS_FILTER_LOC_SOURCEIN = _isndsys.SS_FILTER_LOC_SOURCEIN
class iSndSysListener(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetDirection(*args): return _isndsys.iSndSysListener_SetDirection(*args)
    def SetPosition(*args): return _isndsys.iSndSysListener_SetPosition(*args)
    def SetDistanceFactor(*args): return _isndsys.iSndSysListener_SetDistanceFactor(*args)
    def SetRollOffFactor(*args): return _isndsys.iSndSysListener_SetRollOffFactor(*args)
    def GetDirection(*args): return _isndsys.iSndSysListener_GetDirection(*args)
    def GetPosition(*args): return _isndsys.iSndSysListener_GetPosition(*args)
    def GetDistanceFactor(*args): return _isndsys.iSndSysListener_GetDistanceFactor(*args)
    def GetRollOffFactor(*args): return _isndsys.iSndSysListener_GetRollOffFactor(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysListener_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysListener
    __del__ = lambda self : None;
iSndSysListener_swigregister = _isndsys.iSndSysListener_swigregister
iSndSysListener_swigregister(iSndSysListener)
iSndSysListener_scfGetVersion = _isndsys.iSndSysListener_scfGetVersion

class iSndSysListenerDoppler(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetVelocity(*args): return _isndsys.iSndSysListenerDoppler_SetVelocity(*args)
    def SetDopplerFactor(*args): return _isndsys.iSndSysListenerDoppler_SetDopplerFactor(*args)
    def SetSpeedOfSound(*args): return _isndsys.iSndSysListenerDoppler_SetSpeedOfSound(*args)
    def GetVelocity(*args): return _isndsys.iSndSysListenerDoppler_GetVelocity(*args)
    def GetDopplerFactor(*args): return _isndsys.iSndSysListenerDoppler_GetDopplerFactor(*args)
    def GetSpeedOfSound(*args): return _isndsys.iSndSysListenerDoppler_GetSpeedOfSound(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysListenerDoppler
    __del__ = lambda self : None;
iSndSysListenerDoppler_swigregister = _isndsys.iSndSysListenerDoppler_swigregister
iSndSysListenerDoppler_swigregister(iSndSysListenerDoppler)

class iSndSysLoader(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def LoadSound(*args): return _isndsys.iSndSysLoader_LoadSound(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysLoader_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysLoader
    __del__ = lambda self : None;
iSndSysLoader_swigregister = _isndsys.iSndSysLoader_swigregister
iSndSysLoader_swigregister(iSndSysLoader)
iSndSysLoader_scfGetVersion = _isndsys.iSndSysLoader_scfGetVersion

class iSndSysWrapper(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def QueryObject(*args): return _isndsys.iSndSysWrapper_QueryObject(*args)
    def GetData(*args): return _isndsys.iSndSysWrapper_GetData(*args)
    def SetData(*args): return _isndsys.iSndSysWrapper_SetData(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysWrapper_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysWrapper
    __del__ = lambda self : None;
iSndSysWrapper_swigregister = _isndsys.iSndSysWrapper_swigregister
iSndSysWrapper_swigregister(iSndSysWrapper)
iSndSysWrapper_scfGetVersion = _isndsys.iSndSysWrapper_scfGetVersion

class iSndSysManager(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def CreateSound(*args): return _isndsys.iSndSysManager_CreateSound(*args)
    def RemoveSound(*args): return _isndsys.iSndSysManager_RemoveSound(*args)
    def RemoveSounds(*args): return _isndsys.iSndSysManager_RemoveSounds(*args)
    def GetSoundCount(*args): return _isndsys.iSndSysManager_GetSoundCount(*args)
    def GetSound(*args): return _isndsys.iSndSysManager_GetSound(*args)
    def FindSoundByName(*args): return _isndsys.iSndSysManager_FindSoundByName(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysManager_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysManager
    __del__ = lambda self : None;
iSndSysManager_swigregister = _isndsys.iSndSysManager_swigregister
iSndSysManager_swigregister(iSndSysManager)
iSndSysManager_scfGetVersion = _isndsys.iSndSysManager_scfGetVersion

CS_SNDSYS_SOURCE_DISTANCE_INFINITE = _isndsys.CS_SNDSYS_SOURCE_DISTANCE_INFINITE
class iSndSysSource(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetVolume(*args): return _isndsys.iSndSysSource_SetVolume(*args)
    def GetVolume(*args): return _isndsys.iSndSysSource_GetVolume(*args)
    def GetStream(*args): return _isndsys.iSndSysSource_GetStream(*args)
    def GetPtr(*args): return _isndsys.iSndSysSource_GetPtr(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysSource_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSource
    __del__ = lambda self : None;
iSndSysSource_swigregister = _isndsys.iSndSysSource_swigregister
iSndSysSource_swigregister(iSndSysSource)
iSndSysSource_scfGetVersion = _isndsys.iSndSysSource_scfGetVersion

class iSndSysSourceSoftware(iSndSysSource):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def MergeIntoBuffer(*args): return _isndsys.iSndSysSourceSoftware_MergeIntoBuffer(*args)
    def ProcessOutputFilters(*args): return _isndsys.iSndSysSourceSoftware_ProcessOutputFilters(*args)
    def AddOutputFilter(*args): return _isndsys.iSndSysSourceSoftware_AddOutputFilter(*args)
    def RemoveOutputFilter(*args): return _isndsys.iSndSysSourceSoftware_RemoveOutputFilter(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysSourceSoftware
    __del__ = lambda self : None;
iSndSysSourceSoftware_swigregister = _isndsys.iSndSysSourceSoftware_swigregister
iSndSysSourceSoftware_swigregister(iSndSysSourceSoftware)

class iSndSysSourceOpenAL(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    __swig_destroy__ = _isndsys.delete_iSndSysSourceOpenAL
    __del__ = lambda self : None;
iSndSysSourceOpenAL_swigregister = _isndsys.iSndSysSourceOpenAL_swigregister
iSndSysSourceOpenAL_swigregister(iSndSysSourceOpenAL)

class iSndSysSource3D(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetPosition(*args): return _isndsys.iSndSysSource3D_SetPosition(*args)
    def GetPosition(*args): return _isndsys.iSndSysSource3D_GetPosition(*args)
    def SetMinimumDistance(*args): return _isndsys.iSndSysSource3D_SetMinimumDistance(*args)
    def SetMaximumDistance(*args): return _isndsys.iSndSysSource3D_SetMaximumDistance(*args)
    def GetMinimumDistance(*args): return _isndsys.iSndSysSource3D_GetMinimumDistance(*args)
    def GetMaximumDistance(*args): return _isndsys.iSndSysSource3D_GetMaximumDistance(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysSource3D_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3D
    __del__ = lambda self : None;
iSndSysSource3D_swigregister = _isndsys.iSndSysSource3D_swigregister
iSndSysSource3D_swigregister(iSndSysSource3D)
iSndSysSource3D_scfGetVersion = _isndsys.iSndSysSource3D_scfGetVersion

class iSndSysSource3DDirectionalSimple(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetDirection(*args): return _isndsys.iSndSysSource3DDirectionalSimple_SetDirection(*args)
    def GetDirection(*args): return _isndsys.iSndSysSource3DDirectionalSimple_GetDirection(*args)
    def SetDirectionalRadiation(*args): return _isndsys.iSndSysSource3DDirectionalSimple_SetDirectionalRadiation(*args)
    def GetDirectionalRadiation(*args): return _isndsys.iSndSysSource3DDirectionalSimple_GetDirectionalRadiation(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3DDirectionalSimple
    __del__ = lambda self : None;
iSndSysSource3DDirectionalSimple_swigregister = _isndsys.iSndSysSource3DDirectionalSimple_swigregister
iSndSysSource3DDirectionalSimple_swigregister(iSndSysSource3DDirectionalSimple)

class iSndSysSource3DDirectional(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetDirection(*args): return _isndsys.iSndSysSource3DDirectional_SetDirection(*args)
    def GetDirection(*args): return _isndsys.iSndSysSource3DDirectional_GetDirection(*args)
    def SetDirectionalRadiationInnerCone(*args): return _isndsys.iSndSysSource3DDirectional_SetDirectionalRadiationInnerCone(*args)
    def SetDirectionalRadiationOuterCone(*args): return _isndsys.iSndSysSource3DDirectional_SetDirectionalRadiationOuterCone(*args)
    def SetDirectionalRadiationOuterGain(*args): return _isndsys.iSndSysSource3DDirectional_SetDirectionalRadiationOuterGain(*args)
    def GetDirectionalRadiationInnerCone(*args): return _isndsys.iSndSysSource3DDirectional_GetDirectionalRadiationInnerCone(*args)
    def GetDirectionalRadiationOuterCone(*args): return _isndsys.iSndSysSource3DDirectional_GetDirectionalRadiationOuterCone(*args)
    def GetDirectionalRadiationOuterGain(*args): return _isndsys.iSndSysSource3DDirectional_GetDirectionalRadiationOuterGain(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3DDirectional
    __del__ = lambda self : None;
iSndSysSource3DDirectional_swigregister = _isndsys.iSndSysSource3DDirectional_swigregister
iSndSysSource3DDirectional_swigregister(iSndSysSource3DDirectional)

class iSndSysSource3DDoppler(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetVelocity(*args): return _isndsys.iSndSysSource3DDoppler_SetVelocity(*args)
    def GetVelocity(*args): return _isndsys.iSndSysSource3DDoppler_GetVelocity(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysSource3DDoppler
    __del__ = lambda self : None;
iSndSysSource3DDoppler_swigregister = _isndsys.iSndSysSource3DDoppler_swigregister
iSndSysSource3DDoppler_swigregister(iSndSysSource3DDoppler)

class iSndSysSourceSoftware3D(iSndSysSourceSoftware):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetPosition(*args): return _isndsys.iSndSysSourceSoftware3D_SetPosition(*args)
    def GetPosition(*args): return _isndsys.iSndSysSourceSoftware3D_GetPosition(*args)
    def SetDirection(*args): return _isndsys.iSndSysSourceSoftware3D_SetDirection(*args)
    def GetDirection(*args): return _isndsys.iSndSysSourceSoftware3D_GetDirection(*args)
    def SetDirectionalRadiation(*args): return _isndsys.iSndSysSourceSoftware3D_SetDirectionalRadiation(*args)
    def GetDirectionalRadiation(*args): return _isndsys.iSndSysSourceSoftware3D_GetDirectionalRadiation(*args)
    def SetMinimumDistance(*args): return _isndsys.iSndSysSourceSoftware3D_SetMinimumDistance(*args)
    def SetMaximumDistance(*args): return _isndsys.iSndSysSourceSoftware3D_SetMaximumDistance(*args)
    def GetMinimumDistance(*args): return _isndsys.iSndSysSourceSoftware3D_GetMinimumDistance(*args)
    def GetMaximumDistance(*args): return _isndsys.iSndSysSourceSoftware3D_GetMaximumDistance(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysSourceSoftware3D
    __del__ = lambda self : None;
iSndSysSourceSoftware3D_swigregister = _isndsys.iSndSysSourceSoftware3D_swigregister
iSndSysSourceSoftware3D_swigregister(iSndSysSourceSoftware3D)

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
    def __init__(self, *args): 
        this = _isndsys.new_csSndSysSoundFormat(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _isndsys.delete_csSndSysSoundFormat
    __del__ = lambda self : None;
csSndSysSoundFormat_swigregister = _isndsys.csSndSysSoundFormat_swigregister
csSndSysSoundFormat_swigregister(csSndSysSoundFormat)

CS_SNDSYS_STREAM_PAUSED = _isndsys.CS_SNDSYS_STREAM_PAUSED
CS_SNDSYS_STREAM_UNPAUSED = _isndsys.CS_SNDSYS_STREAM_UNPAUSED
CS_SNDSYS_STREAM_DONTLOOP = _isndsys.CS_SNDSYS_STREAM_DONTLOOP
CS_SNDSYS_STREAM_LOOP = _isndsys.CS_SNDSYS_STREAM_LOOP
CS_SND3D_DISABLE = _isndsys.CS_SND3D_DISABLE
CS_SND3D_RELATIVE = _isndsys.CS_SND3D_RELATIVE
CS_SND3D_ABSOLUTE = _isndsys.CS_SND3D_ABSOLUTE
class iSndSysStream(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def GetDescription(*args): return _isndsys.iSndSysStream_GetDescription(*args)
    def GetRenderedFormat(*args): return _isndsys.iSndSysStream_GetRenderedFormat(*args)
    def Get3dMode(*args): return _isndsys.iSndSysStream_Get3dMode(*args)
    def GetFrameCount(*args): return _isndsys.iSndSysStream_GetFrameCount(*args)
    def GetPosition(*args): return _isndsys.iSndSysStream_GetPosition(*args)
    def ResetPosition(*args): return _isndsys.iSndSysStream_ResetPosition(*args)
    def SetPosition(*args): return _isndsys.iSndSysStream_SetPosition(*args)
    def Pause(*args): return _isndsys.iSndSysStream_Pause(*args)
    def Unpause(*args): return _isndsys.iSndSysStream_Unpause(*args)
    def GetPauseState(*args): return _isndsys.iSndSysStream_GetPauseState(*args)
    def SetLoopState(*args): return _isndsys.iSndSysStream_SetLoopState(*args)
    def GetLoopState(*args): return _isndsys.iSndSysStream_GetLoopState(*args)
    def SetPlayRatePercent(*args): return _isndsys.iSndSysStream_SetPlayRatePercent(*args)
    def GetPlayRatePercent(*args): return _isndsys.iSndSysStream_GetPlayRatePercent(*args)
    def SetAutoUnregister(*args): return _isndsys.iSndSysStream_SetAutoUnregister(*args)
    def GetAutoUnregister(*args): return _isndsys.iSndSysStream_GetAutoUnregister(*args)
    def GetAutoUnregisterRequested(*args): return _isndsys.iSndSysStream_GetAutoUnregisterRequested(*args)
    def AdvancePosition(*args): return _isndsys.iSndSysStream_AdvancePosition(*args)
    def GetDataPointers(*args): return _isndsys.iSndSysStream_GetDataPointers(*args)
    def InitializeSourcePositionMarker(*args): return _isndsys.iSndSysStream_InitializeSourcePositionMarker(*args)
    def ProcessNotifications(*args): return _isndsys.iSndSysStream_ProcessNotifications(*args)
    def RegisterCallback(*args): return _isndsys.iSndSysStream_RegisterCallback(*args)
    def UnregisterCallback(*args): return _isndsys.iSndSysStream_UnregisterCallback(*args)
    def RegisterFrameNotification(*args): return _isndsys.iSndSysStream_RegisterFrameNotification(*args)
    def AlwaysStream(*args): return _isndsys.iSndSysStream_AlwaysStream(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysStream_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysStream
    __del__ = lambda self : None;
iSndSysStream_swigregister = _isndsys.iSndSysStream_swigregister
iSndSysStream_swigregister(iSndSysStream)
cvar = _isndsys.cvar
CS_SNDSYS_STREAM_UNKNOWN_LENGTH = cvar.CS_SNDSYS_STREAM_UNKNOWN_LENGTH
iSndSysStream_scfGetVersion = _isndsys.iSndSysStream_scfGetVersion

class iSndSysStreamCallback(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def StreamLoopNotification(*args): return _isndsys.iSndSysStreamCallback_StreamLoopNotification(*args)
    def StreamPauseNotification(*args): return _isndsys.iSndSysStreamCallback_StreamPauseNotification(*args)
    def StreamUnpauseNotification(*args): return _isndsys.iSndSysStreamCallback_StreamUnpauseNotification(*args)
    def StreamFrameNotification(*args): return _isndsys.iSndSysStreamCallback_StreamFrameNotification(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysStreamCallback
    __del__ = lambda self : None;
iSndSysStreamCallback_swigregister = _isndsys.iSndSysStreamCallback_swigregister
iSndSysStreamCallback_swigregister(iSndSysStreamCallback)

CS_SNDSYS_SOURCE_STOPPED = _isndsys.CS_SNDSYS_SOURCE_STOPPED
CS_SNDSYS_SOURCE_PLAYING = _isndsys.CS_SNDSYS_SOURCE_PLAYING
class iSndSysRenderer(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def SetVolume(*args): return _isndsys.iSndSysRenderer_SetVolume(*args)
    def GetVolume(*args): return _isndsys.iSndSysRenderer_GetVolume(*args)
    def CreateStream(*args): return _isndsys.iSndSysRenderer_CreateStream(*args)
    def CreateSource(*args): return _isndsys.iSndSysRenderer_CreateSource(*args)
    def RemoveStream(*args): return _isndsys.iSndSysRenderer_RemoveStream(*args)
    def RemoveSource(*args): return _isndsys.iSndSysRenderer_RemoveSource(*args)
    def GetListener(*args): return _isndsys.iSndSysRenderer_GetListener(*args)
    def RegisterCallback(*args): return _isndsys.iSndSysRenderer_RegisterCallback(*args)
    def UnregisterCallback(*args): return _isndsys.iSndSysRenderer_UnregisterCallback(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysRenderer_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysRenderer
    __del__ = lambda self : None;
iSndSysRenderer_swigregister = _isndsys.iSndSysRenderer_swigregister
iSndSysRenderer_swigregister(iSndSysRenderer)
iSndSysRenderer_scfGetVersion = _isndsys.iSndSysRenderer_scfGetVersion

class iSndSysRendererCallback(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def StreamAddNotification(*args): return _isndsys.iSndSysRendererCallback_StreamAddNotification(*args)
    def StreamRemoveNotification(*args): return _isndsys.iSndSysRendererCallback_StreamRemoveNotification(*args)
    def SourceAddNotification(*args): return _isndsys.iSndSysRendererCallback_SourceAddNotification(*args)
    def SourceRemoveNotification(*args): return _isndsys.iSndSysRendererCallback_SourceRemoveNotification(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysRendererCallback
    __del__ = lambda self : None;
iSndSysRendererCallback_swigregister = _isndsys.iSndSysRendererCallback_swigregister
iSndSysRendererCallback_swigregister(iSndSysRendererCallback)

class iSndSysRendererSoftware(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def AddOutputFilter(*args): return _isndsys.iSndSysRendererSoftware_AddOutputFilter(*args)
    def RemoveOutputFilter(*args): return _isndsys.iSndSysRendererSoftware_RemoveOutputFilter(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysRendererSoftware
    __del__ = lambda self : None;
iSndSysRendererSoftware_swigregister = _isndsys.iSndSysRendererSoftware_swigregister
iSndSysRendererSoftware_swigregister(iSndSysRendererSoftware)

class iSndSysRendererOpenAL(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def LockWait(*args): return _isndsys.iSndSysRendererOpenAL_LockWait(*args)
    def Release(*args): return _isndsys.iSndSysRendererOpenAL_Release(*args)
    __swig_destroy__ = _isndsys.delete_iSndSysRendererOpenAL
    __del__ = lambda self : None;
iSndSysRendererOpenAL_swigregister = _isndsys.iSndSysRendererOpenAL_swigregister
iSndSysRendererOpenAL_swigregister(iSndSysRendererOpenAL)

class iSndSysSoftwareDriver(core.iBase):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    def Open(*args): return _isndsys.iSndSysSoftwareDriver_Open(*args)
    def Close(*args): return _isndsys.iSndSysSoftwareDriver_Close(*args)
    def StartThread(*args): return _isndsys.iSndSysSoftwareDriver_StartThread(*args)
    def StopThread(*args): return _isndsys.iSndSysSoftwareDriver_StopThread(*args)
    scfGetVersion = staticmethod(_isndsys.iSndSysSoftwareDriver_scfGetVersion)
    __swig_destroy__ = _isndsys.delete_iSndSysSoftwareDriver
    __del__ = lambda self : None;
iSndSysSoftwareDriver_swigregister = _isndsys.iSndSysSoftwareDriver_swigregister
iSndSysSoftwareDriver_swigregister(iSndSysSoftwareDriver)
iSndSysSoftwareDriver_scfGetVersion = _isndsys.iSndSysSoftwareDriver_scfGetVersion



