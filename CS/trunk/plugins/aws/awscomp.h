 #ifndef __AWS_COMPONENT_H__
 #define __AWS_COMPONENT_H__
/**************************************************************************
    Copyright (C) 2000-2001 by Christopher Nelson 
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*****************************************************************************/
#include "ivaria/aws.h"
#include "isys/plugin.h"
#include "csgeom/csrect.h"
#include "awsprefs.h"
#include "awsslot.h"

class awsCanvas;

/** ************************************************************************************************************************
*   The general idea for a component's initialization stage is like this:                                                 *
*       1. construction - any internal structures should be created and intialized.                                       *
*       2. setup - the window manager calls Setup() on the component being created when it is added into the window       *
*       manager's hierarchy.  During setup the component should get a handle to the preference manager (via GetPrefMgr()) *
*       and ask the preference manager about things like it's default texture, colors, etc; using the Lookup* functions.  *
*       The component can then get texture handles from the preference manager using the values of the keys that it has   *
*       looked up.
*           e.g. 
*            
*            iAwsPrefManager *pm = wmgr->GetPrefMgr();
*            
*            pm->LookupStringKey("Texture", texturename);
*            
*            SetTexture(pm->GetPixmapFor(texturename));
*                                                                                                                   
*                                                                              
*                                                                                                                         *
**************************************************************************************************************************/
class awsComponent : public awsSigSrc
{
   /// The stored handle to the window manager, in case a component needs it.
   iAws  *wmgr;

   /// The rectangle marking the frame of this component
   csRect frame;

   /// Every component will have a name, which is translated to an id
   unsigned long id;

public:
    awsComponent();
    virtual ~awsComponent();
    
public:
    /**
     *  This is the function that components use to set themselves up.  All components MUST implement this function.  
     *  You should also call awsComponent::Setup() so that it can perform some default initialization work.
     */
    virtual bool Setup(iAws *wmgr, awsComponentNode *settings);
   
    /// Event dispatcher, demultiplexes events and sends them off to the proper event handler
    virtual bool HandleEvent();

    /// Invalidation routine: allow the component to be redrawn when you call this
    virtual void Invalidate();

    /// Returns the named TYPE of the component, like "Radio Button", etc.
    virtual char *Type()=0;

protected:
    /// Get's this components idea of the window manager.  Should be used internally by the component ONLY.
    iAws *WindowManager()
    { return wmgr; }

public:
    /// Triggered when the component needs to draw
    virtual void OnDraw()=0;

    /// Triggered when the user presses a mouse button down
    virtual bool OnMouseDown(int button, int x, int y)=0;
    
    /// Triggered when the user unpresses a mouse button 
    virtual bool OnMouseUp(int button, int x, int y)=0;
    
    /// Triggered when the user moves the mouse
    virtual bool OnMouseMove(int button, int x, int y)=0;

    /// Triggered when this component loses mouse focus
    virtual bool OnMouseExit()=0;

    /// Triggered when this component gains mouse focus
    virtual bool OnMouseEnter()=0;

    /// Triggered when the user presses a key
    virtual bool OnKeypress(int key)=0;
    
    /// Triggered when the keyboard focus is lost
    virtual bool OnLostFocus()=0;

    /// Triggered when the keyboard focus is gained
    virtual bool OnGainFocus()=0;

};

class awsComponentFactory : public iBase
{
public:
    /// Calls register to register the component that it builds with the window manager
    awsComponentFactory(iAws *wmgr);

    /// Does nothing
    virtual ~awsComponentFactory();

    /// Returns a newly created component of the type this factory handles. 
    virtual awsComponent *Create()=0;

    /// Registers this factory with the window manager
    void Register(iAws *wmgr, char *type);
};

#endif

