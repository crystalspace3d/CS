#include "cssysdef.h"
#include "iutil/objreg.h"
#include "csgfx/imagemanipulate.h"

#include "video-encoder.h"

// these functions are necessary to redirect libavformat output to iFile
static int iFile_write(void *opaque, uint8_t *buf, int buf_size)
{
	iFile* file = (iFile*)opaque;
	return file->Write((char*)buf, buf_size);
}

static int64_t iFile_seek(void *opaque, int64_t offset, int whence)
{
	iFile* file = (iFile*)opaque;
	switch (whence)
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += file->GetPos();
		break;
	case SEEK_END:
		offset += file->GetSize();
		break;
	default:
		return AVERROR(EINVAL);
	}
	if (!file->SetPos(offset))
		return AVERROR(EIO);
	return file->GetPos();
}

csString VideoEncoder::GetError()
{
  MutexScopedLock lock(mutex);
  return error;
}

void VideoEncoder::SetError(const char* fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  error.FormatV(fmt, args);
  va_end (args);
}

VideoEncoder::VideoEncoder(csRef<iVFS> VFS,
	                       ThreadPriority priority,
	                       int width, int height, int framerate,
			               csConfigAccess* config,
		                   const csString& filename,
		                   const csString& videoCodecName,
		                   const csString& audioCodecName,
				           int queueLength)
{
  this->width = width;
  this->height = height;
  this->framerate = framerate;
  this->filename = filename;
  this->videoCodecName = videoCodecName;
  this->audioCodecName = audioCodecName;
  this->queueLength = queueLength;

  this->config = config;
  this->VFS = VFS;

  recording = false;

  avFormat = NULL;
  avContainer = NULL;
  videoStream = NULL;
  video = NULL;
  videoFrame = NULL;
  videoCodec = NULL;
  audioStream = NULL;
  audio = NULL;
  audioFrame = NULL;
  audioCodec = NULL;
   
  // allocate buffers for holding color planes
  Y = new unsigned char[width*height];
  U = new unsigned char[width*height/4];
  V = new unsigned char[width*height/4];

  queue = new csRef<csImageMemory>[queueLength];
  queueTicks = new csMicroTicks[queueLength];
 
  queueWritten = 0;
  queueRead = -1;
  
  thread.AttachNew (new Thread(this));
  thread->SetPriority(priority);
  thread->Start();
}

VideoEncoder::~VideoEncoder()
{
  delete[] Y;
  delete[] U;
  delete[] V;

  delete[] queue;
  delete[] queueTicks;

  // free everything
  if (videoStream)
  {
     avcodec_close(video);
     av_free(video);
     av_free(videoStream);
     av_free(videoFrame);
  }
  if (audioStream)
  {
     avcodec_close(audio);
     av_free(audio);
     av_free(audioStream);
     av_free(audioFrame);
  //   av_free(g_pSamples);
  }

  if (avContainer)
	av_free(avContainer);
}

bool VideoEncoder::NeedFrame()
{
  // check if queue is full
  MutexScopedLock lock (mutex);
  return queueWritten - queueRead < queueLength;
}

void VideoEncoder::Wait()
{
  MutexScopedLock lock (mutex);
  if (queueWritten - queueRead < queueLength)
	return;
  eventReady.Wait(mutex);
}

void VideoEncoder::AddFrame(csMicroTicks Time, csRef<csImageMemory> Frame)
{
  {
    // Make sure we lock the queue before trying to access it
    MutexScopedLock lock (mutex);
	
    // Put image in queue
    queue[queueWritten % queueLength] = Frame;
	queueTicks[queueWritten % queueLength] = Time;
	queueWritten++;
  }
  eventAdd.NotifyOne();
}

void VideoEncoder::Stop()
{
  {
    MutexScopedLock lock (mutex);
    recording = false;
  }
  eventAdd.NotifyOne();
  thread->Wait(); // FIXME: we shouldn't wait
  thread.Invalidate();
}

// try start video recording, returns false if something goes wrong
bool VideoEncoder::TryStart()
{
  // find format
  avFormat = av_guess_format(NULL, filename.GetData(), NULL);
  if (!avFormat)
  {
     SetError("Format for given extension was not found");
     return false;
  }

  // allocate the output media context
  avContainer = avformat_alloc_context();
  if (!avContainer)
  {
    SetError("Could not allocate output context");
    return false;
  }
  avContainer->oformat = avFormat;

  strncpy(avContainer->filename, filename.GetData(), sizeof(avContainer->filename));

  // find codecs
  videoCodec = avcodec_find_encoder_by_name(videoCodecName);
  audioCodec = avcodec_find_encoder_by_name(audioCodecName);

  if (!videoCodec)
  {
    SetError("Video codec %s was not found", videoCodecName.GetData());
    return false;
  }
    
  if (!InitVideoStream())
	return false;

  // write format info to log
  av_dump_format(avContainer, 0, avContainer->filename, 1);

  // open the output file, if needed
  if (!(avFormat->flags & AVFMT_NOFILE))
  {
    file = VFS->Open (filename, VFS_FILE_WRITE | VFS_FILE_UNCOMPRESSED);
    if (!file)
    {
      SetError("Couldn't open file for recording - %s", filename.GetData());
      return false;
    }
	avContainer->pb = avio_alloc_context(writeBuffer, WRITE_BUFFER_SIZE,
		1, // write flag (true)
		(iFile*)file, // user data pointer
		NULL, // read function
		iFile_write, // write function
		iFile_seek // seek function
	);
    if (!avContainer->pb)
    {
      SetError("avio_alloc_context failed");
	  return false;
	}
  }

  // write the stream header, if any
  avformat_write_header(avContainer, NULL);

  recording = true;

  return true;
}

//initialize output video stream
bool VideoEncoder::InitVideoStream()
{
  videoStream = avformat_new_stream(avContainer, videoCodec);
  if (!videoStream)
  {
     SetError("Could not allocate video stream");
     return false;
  }

  video = videoStream->codec;

  avcodec_get_context_defaults3(video, videoCodec);
  video->codec_id = videoCodec->id;

  // put parameters
  // resolution must be a multiple of two
  video->width  = width  & ~1; // make even (dimensions should be even)
  video->height = height & ~1; // make even
  /* time base: this is the fundamental unit of time (in seconds) in terms
     of which frame timestamps are represented. for fixed-fps content,
     timebase should be 1/framerate and timestamp increments should be
     identically 1. */
  video->time_base.den = framerate;
  video->time_base.num = 1;
  video->pix_fmt = PIX_FMT_YUV420P;

//  video->bit_rate = 1500*1000;
  // FIXME:
  video->flags |= CODEC_FLAG_QSCALE;
  video->global_quality = 1*FF_QP2LAMBDA;
  // set quality
  /* if (g_VQuality > 100)
       video->bit_rate = g_VQuality;
    else
    {
        video->flags |= CODEC_FLAG_QSCALE;
        video->global_quality = g_VQuality*FF_QP2LAMBDA;
    }*/

  // some formats want stream headers to be separate
  if (avFormat->flags & AVFMT_GLOBALHEADER)
      video->flags |= CODEC_FLAG_GLOBAL_HEADER;

  // set codec options
  AVDictionary* dict = NULL;
  csString codecName = videoCodec->name;
  csRef<iConfigIterator> option ((*config)->Enumerate("VideoRecorder." + codecName + "."));
  while (option->Next())
    av_dict_set(&dict, option->GetKey(true), option->GetStr(), 0);

  // open the codec
  if (avcodec_open2(video, videoCodec, &dict) < 0)
  {
	SetError("Could not open video codec %s", videoCodec->long_name);
    return false;
  }

  av_dict_free(&dict);

  videoFrame = avcodec_alloc_frame();
  if (!videoFrame)
  {
    SetError("Could not allocate video frame");
    return false;
  }

  videoFrame->linesize[0] = width; // Y
  videoFrame->linesize[1] = width/2; // Cb
  videoFrame->linesize[2] = width/2; // Cr
  videoFrame->linesize[3] = 0;
  return true;
}

void VideoEncoder::Run()
{    
  // initialize libav* and register all codecs and formats
  av_register_all();

  if (!TryStart())
	return;

  while (true)
  {
    // Get an item from queue
    csRef<iImage> screenshot;
    csMicroTicks time;

	{
      MutexScopedLock lock (mutex);
	  queueRead++;
	}
	eventReady.NotifyAll();
    {
      // Make sure we lock the queue before trying to access it
      MutexScopedLock lock (mutex);
      // Wait until we have an image
      while (queueRead >= queueWritten && recording)
        eventAdd.Wait (mutex);

	  if (!recording)
		  break;

	  //csPrintf("queue %3i  %3i\n", queueWritten, queueRead);
		 
      // Get image
      screenshot = queue[queueRead % queueLength];
	  time = queueTicks[queueRead % queueLength];
    }
   
	SaveFrame(screenshot, time);
  }

  // encode remaining frames
  while (queueRead < queueWritten)
  {
	  csRef<iImage> screenshot = queue[queueRead % queueLength];
      SaveFrame(screenshot, queueTicks[queueRead % queueLength]);
	  queueRead++;
  }

  // output buffered frames
  if (videoCodec && videoCodec->capabilities & CODEC_CAP_DELAY)
    while (WriteFrame(NULL, 0));
	/*if (g_pSoundInput)
	{
		alcCaptureStop(g_pSoundInput);
		alcCaptureCloseDevice(g_pSoundInput);
	}*/
    // output any remaining audio
   // while( WriteAudioFrame() );

  // write the trailer, if any.
  av_write_trailer(avContainer);
  file->Flush();
}

bool VideoEncoder::WriteFrame(AVFrame* frame, csMicroTicks time)
{
  if (!videoStream)
    return false;

  AVPacket Packet;
  av_init_packet(&Packet);
  Packet.data = NULL;
  Packet.size = 0;
 
  videoFrame->pts = time*framerate/1000000;

  if (avFormat->flags & AVFMT_RAWPICTURE)
  {
    // raw video case
    Packet.flags |= AV_PKT_FLAG_KEY;
    Packet.stream_index = videoStream->index;
    Packet.data = (uint8_t*)frame;
    Packet.size = sizeof(AVPicture);

    if (av_interleaved_write_frame(avContainer, &Packet) != 0)
	{
    //  Report(CS_REPORTER_SEVERITY_WARNING, "Error while writing video frame");
	  return false;
    }
    return false;
  }
  else
  {
#if LIBAVCODEC_VERSION_MAJOR >= 54
    int got_packet;
    if (avcodec_encode_video2(video, &Packet, frame, &got_packet) < 0)
	{
   //   Report(CS_REPORTER_SEVERITY_WARNING, "avcodec_encode_video2 failed");
	  return false;
	}
    if (!got_packet)
      return false;
		
    if (Packet.pts != AV_NOPTS_VALUE)
       Packet.pts = av_rescale_q(Packet.pts, video->time_base, videoStream->time_base);
    if (Packet.dts != AV_NOPTS_VALUE)
       Packet.dts = av_rescale_q(Packet.dts, video->time_base, videoStream->time_base);
#else 
    Packet.size = avcodec_encode_video(video, outBuffer, OUT_BUFFER_SIZE, frame);
    if (Packet.size < 0)
	{
  //    Report(CS_REPORTER_SEVERITY_WARNING, "avcodec_encode_video failed");
	  return false;
	}
    if (Packet.size == 0)
      return false;

    if (video->coded_frame->pts != AV_NOPTS_VALUE)
      Packet.pts = av_rescale_q(video->coded_frame->pts, video->time_base, videoStream->time_base);
    if (video->coded_frame->key_frame )
      Packet.flags |= AV_PKT_FLAG_KEY;
    Packet.data = outBuffer;
#endif
    // write the compressed frame in the media file
    Packet.stream_index = videoStream->index;
    if (av_interleaved_write_frame(avContainer, &Packet) != 0)
    {
    //  Report(CS_REPORTER_SEVERITY_WARNING, "Error while writing video frame");
	  return false;
	}      
    return true;
  }
}

// Convert RGB to YUV 4:2:0 and encode video frame
void VideoEncoder::SaveFrame(csRef<iImage>& img, csMicroTicks time)
{
  // rescale image if necessary
  if (img->GetWidth() != width || img->GetHeight() != height)
    img = csImageManipulate::Rescale (img, width, height);

  unsigned char* data = (unsigned char*)img->GetImageData();
  
  int pitch = width*4;

  // Y
  for (int i = 0; i < height; i++)
  for (int j = 0; j < width; j++)
  {
	int k = height - i - 1; // invert image upside-down
	uint8_t* p = data + k*pitch + j*4;
	Y[i*width + j] = 16 + ((16828*p[2] + 33038*p[1] + 6416*p[0]) >> 16);
  }
  // U and V
  for (int i = 0; i < height/2; i++)
  for (int j = 0; j < width/2; j++)
  {
	int k = height/2 - i - 1; // invert image upside-down
	uint8_t* p1 = data + 2*k*pitch + 2*j*4;
	uint8_t* p2 = data + 2*k*pitch + 2*(j+1)*4;
	uint8_t* p3 = data + (2*k+1)*pitch + 2*j*4;
	uint8_t* p4 = data + (2*k+1)*pitch + (2*j+1)*4;
	int r = p1[0] + p2[0] + p3[0] + p4[0];
	int g = p1[1] + p2[1] + p3[1] + p4[1];
	int b = p1[2] + p2[2] + p3[2] + p4[2];
	U[i*width/2 + j] = 128 + ((-2428*r - 4768*g + 7196*b) >> 16);
	V[i*width/2 + j] = 128 + (( 7196*r - 6026*g - 1170*b) >> 16);
  }

  videoFrame->data[0] = Y;
  videoFrame->data[1] = U;
  videoFrame->data[2] = V;
  WriteFrame(videoFrame, time);
}
