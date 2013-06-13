#ifndef __CS_VIDEOENCODER_H__
#define __CS_VIDEOENCODER_H__

#include "iutil/vfs.h"	
#include "csutil/threading/thread.h"
#include "csutil/csstring.h"
#include "csutil/cfgacc.h"
#include "csgfx/imagememory.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
}

using namespace CS::Threading;

class VideoEncoder : public Runnable
{
  static const int WRITE_BUFFER_SIZE = 4*1024;
#if LIBAVCODEC_VERSION_MAJOR < 54
  static const int OUT_BUFFER_SIZE = 200000;
#endif

  int width, height;
  int framerate;
  bool vfr;
  csString filename;
  csString videoCodecName;
  csString audioCodecName;

  csString error;
 
  csRef<iVFS> VFS;
  csRef<iFile> file;

  csConfigAccess* config;
  
  /// libavcodec, libavformat data
  AVFormatContext* avContainer;
  AVOutputFormat* avFormat;
  AVStream* audioStream;
  AVStream* videoStream;
  AVFrame* audioFrame;
  AVFrame* videoFrame;
  AVCodec* audioCodec;
  AVCodec* videoCodec;
  AVCodecContext* audio;
  AVCodecContext* video;

  /// Color planes
  unsigned char *Y, *U, *V;
  /// Write buffer (it is used for redirecting libavformat output to iFile)
  unsigned char writeBuffer[WRITE_BUFFER_SIZE];
  
#if LIBAVCODEC_VERSION_MAJOR < 54
  unsigned char outBuffer[OUT_BUFFER_SIZE];
#endif

  bool recording;
  int queueLength;
  csRef<csImageMemory>* queue; // curcullar buffer
  csMicroTicks* queueTicks;
  int queueWritten, queueRead;
  Mutex mutex; // mutex for queue, queueTicks, queueRead, queueWritten and recording
  Condition event;
 // csRef<iThreadReturn> thread;
   
  csRef<Thread> thread;

  bool TryStart();
  bool InitVideoStream();
  bool WriteFrame(AVFrame* frame, csMicroTicks time);
  void SaveFrame(csRef<iImage>& image, csMicroTicks time);

  void SetError(const char* fmt, ...);

  //-- Runnable
  void Run ();
  const char* GetName () const
  {
    return "Video encoder";
  }

public:
	
  VideoEncoder(csRef<iVFS> VFS,
	           ThreadPriority priority,
		       int width, int height, int framerate,
			   bool vfr,
			   csConfigAccess* config,
		       const csString& filename,
		       const csString& videoCodecName,
		       const csString& audioCodecName,
        	   int queueLength);
  ~VideoEncoder();
  
  /// check if queue is not full
  bool NeedFrame();

  void AddFrame(csMicroTicks Time, csRef<csImageMemory> Frame);
  void Stop();
  
  csString GetError();
};

#endif // __CS_VIDEOENCODER_H__