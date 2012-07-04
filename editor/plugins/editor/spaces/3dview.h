/*
    Copyright (C) 2012 by Christian Van Brussel

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
*/

#ifndef __SPACE_3DVIEW_H__
#define __SPACE_3DVIEW_H__

#include "cstool/collider.h"
#include "csutil/csbaseeventh.h"
#include "csutil/eventnames.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "ieditor/editor.h"
#include "ieditor/space.h"
#include "iutil/event.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "ivaria/cameramanager.h"

#include <wx/event.h>
#include <wx/dnd.h>

class wxWindow;

using namespace CS::EditorApp;

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{

class CS3DSpace : public scfImplementation1<CS3DSpace, iSpace>,
    public csBaseEventHandler
{
public:
  CS3DSpace (iBase* parent);
  virtual ~CS3DSpace();

  //-- iSpace
  virtual bool Initialize (iObjectRegistry* obj_reg, iEditor* editor,
			   iSpaceFactory* factory, wxWindow* parent);
  virtual iSpaceFactory* GetFactory () const { return factory; }
  virtual wxWindow* GetwxWindow ();
  virtual void DisableUpdates (bool val) { printf ("DisableUpdates %i\n", (int) val);disabled = val; }
  virtual void Update ();

  //-- iEventHandler
  bool HandleEvent (iEvent &event);

  void OnSize (wxSizeEvent& event);
  
private:
  virtual void UpdateFrame ();
  
private:
  iObjectRegistry* object_reg;
  csRef<iSpaceFactory> factory;
  wxWindow* window;
  bool disabled;
  
  csRef<iEditor> editor;
  
  csRef<iEngine> engine;
  csRef<iGraphics3D> g3d;
  csRef<iWxWindow> wxwin;
  csRef<iView> view;
  csRef<CS::Utility::iCameraManager> cameraManager;

  csEventID eventSetCollection;

  struct FrameListener : public csBaseEventHandler
  {
    FrameListener (CS3DSpace* space);

    //-- iEventHandler
    bool HandleEvent (iEvent &event);

  private:
    CS3DSpace* space;
  };
  FrameListener* frameListener;

  class Space : public wxPanel
  {
    public:
      Space (CS3DSpace* p, wxWindow* parent, wxWindowID id = wxID_ANY,
	    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize)
      : wxPanel (parent, id, pos, size), space(p)
      {}
    
      //-- wxPanel
      virtual void OnSize (wxSizeEvent& ev)
      { if (space) space->OnSize (ev); }

    private:
      CS3DSpace* space;
      
      DECLARE_EVENT_TABLE ();
  };
};

}
CS_PLUGIN_NAMESPACE_END(CSEditor)

#endif // __SPACE_3DVIEW_H__
