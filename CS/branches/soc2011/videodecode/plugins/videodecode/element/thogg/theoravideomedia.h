/*
Copyright (C) 2011 by Alin Baciu

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

#ifndef __CS_THOGGVIDEOMEDIA_H__
#define __CS_THOGGVIDEOMEDIA_H__

/**\file
* Video Player: media stream 
*/

#include <iutil/comp.h>
#include <ivideodecode/media.h>
#include <csutil/scf_implementation.h>

// Theora headers
#include "csutil/custom_new_disable.h"
#include "theora/theoradec.h"
#include "csutil/custom_new_enable.h"

#include <iostream>
using namespace std;
using namespace CS::Threading;

#define QUALIFIED_PLUGIN_NAME "crystalspace.vpl.element.thogg"

struct iTextureWrapper; 
struct csVPLvideoFormat;

/**
  * Video stream
  */
class csTheoraVideoMedia : public scfImplementation2
    < csTheoraVideoMedia, iVideoMedia, scfFakeInterface<iMedia> >
{
private:
  iObjectRegistry* _object_reg;
  char*            _name;
  float            _length;
  unsigned long    _frameCount;

  ogg_int64_t      _videobuf_granulepos;
  ogg_int64_t      _frameToSkip;

  csRef<iTextureHandle>       _texture;
  csRefArray<iTextureHandle>  _buffers;

  int                         _activeBuffer;
  bool                        _canSwap;

  // Theora-related stuff
  ogg_stream_state  _streamState;
  th_info           _streamInfo;
  th_comment        _streamComments;
  th_dec_ctx       *_decodeControl;
  ogg_packet        _oggPacket;
  th_setup_info    *_setupInfo;
  int               _theora_p;
  FILE             *_infile;

  bool    _decodersStarted;
  bool    _videobufReady;
  double  _videobufTime;

  typedef uint8* cachedData;

  csFIFO<cachedData>  _cache;
  size_t              _cacheSize;

  double              _FPS;
  float               _aspectRatio;

  // Stuff for conversion on the other thread
  uint8*  _rgbBuff;
  void    Convert ();
  
  th_ycbcr_buffer _currentYUVBuffer;
  uint8*          _currentPixels;

  // Look-up tables for conversion
  int Ylut[256],
      GUlut[256],
      GVlut[256],
      RVlut[256],
      BUlut[256];

  // Mutex
  Mutex       _writeMutex;
  bool        _isWrite;
  Condition   _isWriting;

public:

  // Provide access to the Theora specific members
  // Inline because it's faster, although a bit slow
  inline ogg_stream_state* StreamState ()     { return &_streamState; }
  inline th_info*          StreamInfo ()      { return &_streamInfo; }
  inline th_comment*       StreamComments ()  { return &_streamComments; }
  inline th_dec_ctx *      DecodeControl ()   { return _decodeControl; }
  inline th_setup_info**   SetupInfo ()       { return &_setupInfo; }
  inline int&              Theora_p ()        { return _theora_p; }

public:
  csTheoraVideoMedia (iBase* parent);
  virtual ~csTheoraVideoMedia ();

  // From iComponent
  virtual bool Initialize (iObjectRegistry*);

  // From iMedia
  virtual const char* GetName () const;  
  virtual const char* GetType () const;
  virtual unsigned long GetFrameCount () const;
  virtual float GetLength () const;
  virtual double GetPosition () const;
  virtual void CleanMedia () ;
  virtual bool Update () ;
  virtual void WriteData () ;
  virtual void SwapBuffers () ;
  virtual void SetCacheSize (size_t size) ;
  virtual bool HasDataReady () ;
  virtual bool IsCacheFull () ;
  virtual void DropFrame () ;

  // From iVideoMedia
  virtual float GetAspectRatio () ;
  virtual iTextureHandle* GetVideoTarget ();
  virtual double GetTargetFPS () ;

  inline void SetFrameCount (unsigned long count)  { _frameCount=count; }
  inline void SetLength (float length)  { this->_length=length; }
  long SeekPage (long targetFrame, long frameCount, bool return_keyframe, 
                 ogg_sync_state *oy, unsigned  long fileSize);
  // An easy way to initialize the stream
  void InitializeStream (const char* name, ogg_stream_state &state, th_info &info, 
                         th_comment &comments, th_setup_info *setupInfo,
                         FILE *source, csRef<iTextureManager> texManager);
};


#endif // __CS_THOGGVIDEOMEDIA_H__
