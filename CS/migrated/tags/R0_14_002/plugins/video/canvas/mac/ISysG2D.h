
#include <QDOffscreen.h>
#include <Events.h>
#include "cscom/com.h"

#ifndef __ISYSG2D_H__
#define __ISYSG2D_H__

extern const IID IID_IMacGraphicsInfo;

/// IMacGraphicsInfo interface -- for Mac-specific properties.
interface IMacGraphicsInfo : public IUnknown
{
    ///
    STDMETHOD(Open)(char* szTitle) = 0;
    ///
    STDMETHOD(Close)() = 0;
    ///
    STDMETHOD(ActivateWindow)( WindowPtr theWindow, bool active ) = 0;
    ///
    STDMETHOD(UpdateWindow)( WindowPtr theWindow, bool *updated ) = 0;
    ///
    STDMETHOD(PointInWindow)( Point *thePoint, bool *inWindow ) = 0;
    ///
    STDMETHOD(DoesDriverNeedEvent)( bool *enabled ) = 0;
    ///
    STDMETHOD(SetColorPalette)( void ) = 0;
    ///
    STDMETHOD(WindowChanged)( void ) = 0;
    ///
    STDMETHOD(HandleEvent)( EventRecord *inEvent, bool *outEventWasProcessed ) = 0;
};

#endif
