#ifndef __NeXT_NeXTSystemDriver_h
#define __NeXT_NeXTSystemDriver_h
//=============================================================================
//
//	Copyright (C)1999 by Eric Sunshine <sunshine@sunshineco.com>
//
// The contents of this file are copyrighted by Eric Sunshine.  This work is
// distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  You may distribute this file provided that this
// copyright notice is retained.  Send comments to <sunshine@sunshineco.com>.
//
//=============================================================================
//-----------------------------------------------------------------------------
// NeXTSystemDriver.h
//
//	NeXT-specific hardware & operating/system drivers for CrystalSpace.
//
//-----------------------------------------------------------------------------
#include "sysdef.h"
#include "cssys/common/system.h"
#include "csinput/csinput.h"
#include "NeXTSystemInterface.h"
class NeXTSystemProxy;

//-----------------------------------------------------------------------------
// NeXT-specific keyboard and mouse drivers.
//-----------------------------------------------------------------------------
class SysKeyboardDriver : public csKeyboardDriver {};
class SysMouseDriver    : public csMouseDriver    {};


//-----------------------------------------------------------------------------
// NeXT-specific subclass of csSystemDriver.  Conforms to INeXTSystemDriver
// COM-interface via composition.
//-----------------------------------------------------------------------------
class SysSystemDriver : public csSystemDriver
    {
    typedef csSystemDriver superclass;
    friend NeXTSystemProxy;

private:
    NeXTSystemProxy* proxy; // Interface to Objective-C world; see README.NeXT.
    int simulated_depth;

protected:
    virtual bool ParseArg( int argc, char* argv[], int& i );

public:
    SysSystemDriver();
    virtual ~SysSystemDriver();
    virtual bool Initialize( int argc, char *argv[], IConfig* cfg_engine );
    virtual void SetSystemDefaults();
    virtual void Help();
    virtual void Loop ();

    // Conform to INeXTSystemDriver via composition.
    class XNeXTSystemDriver : public INeXTSystemDriver
	{
	DECLARE_IUNKNOWN()
	STDMETHOD(GetSimulatedDepth)( int& );
	};
    DECLARE_IUNKNOWN()
    DECLARE_INTERFACE_TABLE(SysSystemDriver)
    DECLARE_COMPOSITE_INTERFACE_EMBEDDED(NeXTSystemDriver);
    };

#endif // __NeXT_NeXTSystemDriver_h
