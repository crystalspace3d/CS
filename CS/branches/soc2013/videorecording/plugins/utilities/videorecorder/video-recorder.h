
#ifndef __CS_VIDEORECORDER_H__
#define __CS_VIDEORECORDER_H__

#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "ivaria/movierecorder.h"
#include "ivaria/reporter.h"
#include "ivideo/graph2d.h"
#include "csutil/eventhandlers.h"
#include "csutil/scf_implementation.h"
#include "csutil/cfgacc.h"
#include "csutil/threading/thread.h"
#include "cstool/numberedfilenamehelper.h"	

class VideoEncoder;

CS_PLUGIN_NAMESPACE_BEGIN(VideoRecorder)
{

/**
* Video recording plugin.
*/
class csVideoRecorder : public scfImplementation2<csVideoRecorder, iMovieRecorder, iComponent>
{
  CS_DECLARE_EVENT_SHORTCUTS;
  
  enum Mode
  {
	MODE_VFR,
	MODE_CFR_DROP,
	MODE_FORCED_CFR,
  };

  iObjectRegistry* object_reg;
  csConfigAccess config;
  csRef<iReporter> reporter;
  csRef<iGraphics2D> g2d;
  csRef<iVFS> VFS;

  bool initialized;
  bool paused;
  bool recording;
  Mode mode;
  
  VideoEncoder* encoder;
  
  /// format of the movie filename (e.g. "/this/crystal000")
  CS::NumberedFilenameHelper filenameHelper;
  csString filenameFormat;
  csString filename;
  /// If this is set then it will be used instead of filenameFormat
  csString forcedFilename;

  /// Encoding parameters
  int forcedWidth, forcedHeight;
  int width, height;
  int framerate;
  csString extension;
  csString videoCodecName;
  csString audioCodecName;

  int queueLength;
  Threading::ThreadPriority priority;

  csMicroTicks startTick;
  csMicroTicks pauseTick;
  csMicroTicks prevTick;

  struct KeyCode
  {
    utf32_char code;
    bool shift, alt, ctrl;

	bool operator==(KeyCode other)
	{
		return (code == other.code) && (shift == other.shift) && (alt == other.alt)  && (ctrl == other.ctrl);
	}
  };
  KeyCode keyRecord, keyPause;

  /// Handle keydown event
  bool OnKeyDown(KeyCode key);
  /// Handle frame end event
  void OnFrameEnd();
  
  /// Parse keyname and return key code + modifier status (from bugplug).
  KeyCode GetKeyCode (const char* keystring);

  bool InitPlugin();
  
  /// Report message
  void Report (int severity, const char* msg, ...);
  
  iObjectRegistry* GetObjectRegistry() const
  { return object_reg; }

  /**
   * Embedded iEventHandler interface that handles keyboard events
   */
  class KeyEventHandler : 
    public scfImplementation1<KeyEventHandler, iEventHandler>
  {
    csVideoRecorder* parent;

  public:
    KeyEventHandler (csVideoRecorder* parent) : scfImplementationType (this), parent (parent) {}
    virtual ~KeyEventHandler () {}
    virtual bool HandleEvent (iEvent& event);

    CS_EVENTHANDLER_NAMES("crystalspace.videorecorder.keyboard")
    CS_EVENTHANDLER_NIL_CONSTRAINTS
  };
  KeyEventHandler keyEventHandler;

  /**
  * Embedded iEventHandler interface that handles frame end events.
  */
  class FrameEventHandler : 
    public scfImplementation1<FrameEventHandler, iEventHandler>
  {
    csVideoRecorder* parent;

  public:
    FrameEventHandler (csVideoRecorder* parent) : scfImplementationType (this), parent (parent) {}
    virtual ~FrameEventHandler () {}
    virtual bool HandleEvent (iEvent& event)
    {
      if (event.Name == parent->Frame)
        parent->OnFrameEnd();
      return false;
    }
    CS_EVENTHANDLER_PHASE_FRAME("crystalspace.videorecorder.frame")
  };
  FrameEventHandler frameEventHandler;

public:
  csVideoRecorder (iBase* parent);
  virtual ~csVideoRecorder ();
  
  //-- iComponent
  virtual bool Initialize(iObjectRegistry *iobject_reg);
  
  //-- iMovieRecorder
  virtual bool 	IsPaused () const;
  virtual bool 	IsRecording () const;
  virtual void 	Pause ();
  virtual void 	SetFilenameFormat (const char *format);
  virtual void 	SetRecordingFile (const char *filename);
  virtual void 	Start ();
  virtual void 	Stop ();
  virtual void 	UnPause ();
};

}
CS_PLUGIN_NAMESPACE_END(VideoRecorder)

#endif //__CS_VIDEORECORDER_H__