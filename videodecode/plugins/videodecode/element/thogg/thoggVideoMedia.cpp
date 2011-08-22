#include <cssysdef.h>
#include "thoggVideoMedia.h"
#include <iutil/objreg.h>
#include <iutil/plugin.h>


SCF_IMPLEMENT_FACTORY (csTheoraVideoMedia)


csTheoraVideoMedia::csTheoraVideoMedia (iBase* parent) :
scfImplementationType (this, parent),
object_reg (0)
{
}

csTheoraVideoMedia::~csTheoraVideoMedia ()
{
}

void csTheoraVideoMedia::CleanMedia ()
{
  ogg_stream_clear (&_streamState);

  if (_decodersStarted)
  {
    th_decode_free (_decodeControl);
  }

  th_comment_clear (&_streamComments);
  th_info_clear (&_streamInfo);
  th_setup_free (_setupInfo);
  printf ("video stream is clean\n");
}

bool csTheoraVideoMedia::Initialize (iObjectRegistry* r)
{
  object_reg = r;

  // initialize the decoders
  if (_theora_p)
  {
    //Clear the theora state in case it contains previous data
    _decodeControl=NULL;

    _decodeControl=th_decode_alloc (&_streamInfo,_setupInfo);

    //Initialize the decoders and print the info on the stream
    printf ("Ogg logical stream %ld is Theora %dx%d %.02f fps video\n"
      "  Frame content is %dx%d with offset (%d,%d).\n",
      _streamState.serialno,_streamInfo.pic_width,_streamInfo.pic_height, (double)_streamInfo.fps_numerator/_streamInfo.fps_denominator,
      _streamInfo.frame_width, _streamInfo.frame_height, _streamInfo.pic_x, _streamInfo.pic_y);

    _aspectRatio = (float)_streamInfo.aspect_numerator/ (float)_streamInfo.aspect_denominator;
    cout<<_aspectRatio<<endl;

    _FPS = (double)_streamInfo.fps_numerator/_streamInfo.fps_denominator;

    _decodersStarted=true;
    _videobuf_granulepos=-1;
    _videobufTime=0;
    _frameToSkip=-1;
    cacheSize=1;
  }
  else
  {
    /* tear down the partial theora setup */
    th_info_clear (&_streamInfo);
    th_comment_clear (&_streamComments);

    _decodersStarted=false;
  }

  return 0;
}

const char* csTheoraVideoMedia::GetType () const
{
  return "TheoraVideo";
}

unsigned long csTheoraVideoMedia::GetFrameCount() const
{
  return frameCount;
}

float csTheoraVideoMedia::GetLength () const
{
  return length;
}

void csTheoraVideoMedia::GetVideoTarget (csRef<iTextureHandle> &texture)
{
  // We want "texture" to point to its internal representation in the stream
  texture = _texture;
}

double csTheoraVideoMedia::GetPosition () const
{
  return _videobufTime;
}

bool csTheoraVideoMedia::Update ()
{
  //Convert ();
  if (cache.GetSize ()>=cacheSize)
    return false;

  _videobufReady=false;

  while (_theora_p && !_videobufReady)
  {
    if (ogg_stream_packetout (&_streamState,&_oggPacket)>0)
    {
      if (th_decode_packetin (_decodeControl,&_oggPacket,&_videobuf_granulepos)>=0)
      {
        _videobufTime=th_granule_time (_decodeControl,_videobuf_granulepos);

        if (th_granule_frame (_decodeControl,_videobuf_granulepos)<_frameToSkip)
        {
          //_videobufReady=false;
          return false;
        }
        else
        {
          _videobufReady=true;
          _frameToSkip = -1;


          th_ycbcr_buffer yuv;
          th_decode_ycbcr_out (_decodeControl,yuv);
          //currentData.yuv = &yuv;
          memcpy (&currentData.yuv, &yuv, sizeof (yuv));
          Convert ();
          cachedData data;
          data.pixels = currentData.pixels;
          cache.Push (data);
        }
      }
    }
    else
      break;
  }


  if (!_videobufReady)
    return true;

  return false;
}

long csTheoraVideoMedia::SeekPage (long targetFrame,bool return_keyframe, ogg_sync_state *oy,unsigned long fileSize)
{
  MutexScopedLock lock (writeMutex);
  while (isWrite)
    isWriting.Wait (writeMutex);
  rgbBuff = NULL;

  cache.DeleteAll ();

  ogg_stream_reset (&_streamState);
  th_decode_free (_decodeControl);
  _decodeControl=th_decode_alloc (&_streamInfo,_setupInfo);

  int seek_min=0, seek_max=fileSize;
  long frame;
  ogg_int64_t granule=0;
  bool fineseek=false;
  ogg_page og;

  for (int i=0;i<100;i++)
  {
    ogg_sync_reset (oy);

    fseek (_infile,(seek_min+seek_max)/2,SEEK_SET);
    memset (&og, 0, sizeof (ogg_page));
    ogg_sync_pageseek (oy,&og);

    while (true)
    {
      int ret=ogg_sync_pageout ( oy, &og );
      if (ret == 1)
      {
        int serno=ogg_page_serialno (&og);
        if (serno == _streamState.serialno)
        {
          granule=ogg_page_granulepos (&og);
          if (granule >= 0)
          {
            frame= (long) th_granule_frame (_decodeControl,granule);
            if (frame < targetFrame-1 && targetFrame-frame < 10)
            {
              fineseek=true;
              if (!return_keyframe) break;
            }

            if (fineseek && frame >= targetFrame)
              break;

            if (fineseek) 
              continue;

            if (targetFrame-1 > frame) 
              seek_min= (seek_min+seek_max)/2;
            else
              seek_max= (seek_min+seek_max)/2;
            break;
          }
        }
      }
      else
      {
        char *buffer = ogg_sync_buffer ( oy, 4096);
        int bytesRead = fread (buffer,1,4096,_infile);
        if (bytesRead == 0) break;
        ogg_sync_wrote ( oy, bytesRead );
      }
    }
    if (fineseek) break;
  }

  ogg_stream_pagein (&_streamState,&og);
  //granule=frame << mInfo->TheoraInfo.keyframe_granule_shift;
  th_decode_ctl (_decodeControl,TH_DECCTL_SET_GRANPOS,&granule,sizeof (granule));

  //make sure we skip to the keyframe we need
  if (return_keyframe)
  {
    _frameToSkip = targetFrame;
    //cout<<"want to skip to :"<<_frameToSkip<<endl;

    return (long) (granule >> _streamInfo.keyframe_granule_shift);
  }
  return -1;
}

void csTheoraVideoMedia::InitializeStream (ogg_stream_state &state, th_info &info, th_comment &comments,
                                         th_setup_info *setupInfo, FILE *source, csRef<iTextureManager> texManager)
{
  memcpy (&_streamState,&state,sizeof (state));
  memcpy (&_streamInfo,&info,sizeof (info));
  memcpy (&_streamComments,&comments,sizeof (comments));
  memcpy (&_setupInfo,&setupInfo,sizeof (setupInfo));
  _theora_p=1;

  _decodersStarted = false;
  _infile = source;

  // Create the buffers needed for double buffering
  csRef<iTextureHandle> tex1 = texManager->CreateTexture 
    (_streamInfo.frame_width, _streamInfo.frame_height, 0, csimg2D, "rgb8",
     CS_TEXTURE_2D | CS_TEXTURE_NPOTS);
  _buffers.Push (tex1);

  csRef<iTextureHandle> tex2 = texManager->CreateTexture 
    (_streamInfo.frame_width, _streamInfo.frame_height, 0, csimg2D, "rgb8",
     CS_TEXTURE_2D | CS_TEXTURE_NPOTS);
  _buffers.Push (tex2);

  activeBuffer = 1;

  _texture = _buffers[0];

  size_t dstSize;
  iTextureHandle* tex = _buffers.Get (activeBuffer);
  rgbBuff = tex->QueryBlitBuffer (_streamInfo.pic_x,_streamInfo.pic_y,_streamInfo.pic_width,_streamInfo.pic_height,dstSize);


  // Initialize the LUTs
  {
    double scale = 1L << 8, temp;

    for (int i=0;i<256;i++)
    {
      temp = i - 128;

      Ylut[i] = (int) ( (1.164 * scale + 0.5) * (i - 16));
      RVlut[i] = (int) ( (1.596 * scale + 0.5) * temp);		//Calc R component
      GUlut[i] = (int) ( (0.391 * scale + 0.5) * temp);		//Calc G u & v components
      GVlut[i] = (int) ( (0.813 * scale + 0.5) * temp);
      BUlut[i] = (int) ( (2.018 * scale + 0.5) * temp);		//Calc B component
    }
  }

  canSwap=false;
}

void csTheoraVideoMedia::DropFrame ()
{
  if (cache.GetSize ()!=0)
  {
    cachedData dat = cache.PopTop ();
    delete dat.pixels;
  }
}

void csTheoraVideoMedia::Convert ()
{

  int y_offset= (_streamInfo.pic_x&~1)+currentData.yuv[0].stride* (_streamInfo.pic_y&~1);

  uint8* pixels = rgbBuff; 

  int Y,U,V,R,G,B;
  // 4:2:0 pixel format
  if (_streamInfo.pixel_fmt==TH_PF_420)
  {
    int uv_offset= (_streamInfo.pic_x/2)+ (currentData.yuv[1].stride)* (_streamInfo.pic_y/2);
    uint8 * test = new uint8[_streamInfo.pic_width*_streamInfo.pic_height*4];
    int k=0;
    for (ogg_uint32_t y = 0 ; y < _streamInfo.frame_height ; y++)
      for (ogg_uint32_t x = 0 ; x < _streamInfo.frame_width ; x++)
      {
        Y = (currentData.yuv[0].data+y_offset+currentData.yuv[0].stride* ( (y)))[x] ;//-16;
        U = (currentData.yuv[1].data+uv_offset+currentData.yuv[1].stride* (y/2))[x/2] ;//- 128;
        V = (currentData.yuv[2].data+uv_offset+currentData.yuv[2].stride* (y/2))[x/2] ;//- 128;

        /* int R = ((298*Y + 409*V + 128)>>8);
        int G = ((298*Y - 100*U - 208*V + 128)>>8);
        int B = ((298*Y + 516*U + 128)>>8);*/
        R = (Ylut[Y] + RVlut[V])>>8;
        G = (Ylut[Y] - GUlut[U] - GVlut[V])>>8;
        B = (Ylut[Y] + BUlut[U])>>8;

        // Clamping the values here is faster than calling a function
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        test[k] = (uint8)R;
        k++;
        test[k] = (uint8)G;
        k++;
        test[k] = (uint8)B;
        k++;
        test[k] = 0xff;
        k++;
      }

      currentData.pixels = test;

  }
  // 4:2:2 pixel format
  else if (_streamInfo.pixel_fmt==TH_PF_422)
  {
    int uv_offset= (_streamInfo.pic_x/2)+ (currentData.yuv[1].stride)* (_streamInfo.pic_y);
    uint8 * test = new uint8[_streamInfo.pic_width*_streamInfo.pic_height*4];
    int k=0;
    for (ogg_uint32_t y = 0 ; y < _streamInfo.frame_height ; y++)
      for (ogg_uint32_t x = 0 ; x < _streamInfo.frame_width ; x++)
      {
        Y = (currentData.yuv[0].data+y_offset+currentData.yuv[0].stride*y)[x] ;//-16;
        U = (currentData.yuv[1].data+uv_offset+currentData.yuv[1].stride* (y))[x/2] ;//- 128;
        V = (currentData.yuv[2].data+uv_offset+currentData.yuv[2].stride* (y))[x/2] ;//- 128;

        R = (Ylut[Y] + RVlut[V])>>8;
        G = (Ylut[Y] - GUlut[U] - GVlut[V])>>8;
        B = (Ylut[Y] + BUlut[U])>>8;

        // Clamping the values here is faster than calling a function
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        test[k] = (uint8)R;
        k++;
        test[k] = (uint8)G;
        k++;
        test[k] = (uint8)B;
        k++;
        test[k] = 0xff;
        k++;
      }
      currentData.pixels = test;

  }
  // 4:4:4 pixel format
  else if (_streamInfo.pixel_fmt==TH_PF_444)
  {
    int uv_offset= (_streamInfo.pic_x/2)+ (currentData.yuv[1].stride)* (_streamInfo.pic_y);

    uint8 * test = new uint8[_streamInfo.pic_width*_streamInfo.pic_height*4];
    int k=0;
    for (ogg_uint32_t y = 0 ; y < _streamInfo.frame_height ; y++)
      for (ogg_uint32_t x = 0 ; x < _streamInfo.frame_width ; x++)
      {
        Y = (currentData.yuv[0].data+y_offset+currentData.yuv[0].stride*y)[x];// -16;
        U = (currentData.yuv[1].data+uv_offset+currentData.yuv[1].stride* (y))[x] ;//- 128;
        V = (currentData.yuv[2].data+uv_offset+currentData.yuv[2].stride* (y))[x] ;//- 128;

        R = (Ylut[Y] + RVlut[V])>>8;
        G = (Ylut[Y] - GUlut[U] - GVlut[V])>>8;
        B = (Ylut[Y] + BUlut[U])>>8;

        // Clamping the values here is faster than calling a function
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        test[k] = (uint8)R;
        k++;
        test[k] = (uint8)G;
        k++;
        test[k] = (uint8)B;
        k++;
        test[k] = 0xff;
        k++;
      }
      currentData.pixels = test;

  }
  else
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_WARNING, QUALIFIED_PLUGIN_NAME,
      "The Theora video stream has an unsupported pixel format.\n");
    isWrite=false;
    isWriting.NotifyOne ();
    return;
  }
}
void csTheoraVideoMedia::SwapBuffers ()
{
  if(canSwap)
  {
    if (activeBuffer==0)
    {
      _texture = _buffers[activeBuffer];
      activeBuffer = 1;
      canSwap=false;
    }
    else
    {
      _texture = _buffers[activeBuffer];
      activeBuffer = 0;
      canSwap=false;
    }
  }
}


void csTheoraVideoMedia::WriteData ()
{
  isWrite=true;
  if(!canSwap && cache.GetSize ()!=0)
  {
    {
      MutexScopedLock lock (writeMutex);

      cachedData dat = cache.PopTop ();
      size_t dstSize;
      iTextureHandle* tex = _buffers.Get (activeBuffer);
      rgbBuff = tex->QueryBlitBuffer (_streamInfo.pic_x,_streamInfo.pic_y,_streamInfo.pic_width,_streamInfo.pic_height,dstSize);

      memcpy(rgbBuff,dat.pixels,_streamInfo.pic_width*_streamInfo.pic_height*4);
      delete dat.pixels;

      tex->ApplyBlitBuffer (rgbBuff);

      canSwap=true;
      isWrite=false;
      isWriting.NotifyOne ();
    }
  }
  isWrite=false;
}

void csTheoraVideoMedia::SetCacheSize (size_t size) 
{
  cacheSize = size;
}

bool csTheoraVideoMedia::HasDataReady ()
{
  if (cache.GetSize ()!=0)
    return true;
  return false;
}
bool csTheoraVideoMedia::IsCacheFull ()
{
  if (cache.GetSize ()>=cacheSize)
    return true;
  return false;
}

double csTheoraVideoMedia::GetTargetFPS ()
{
  return _FPS;
}

float csTheoraVideoMedia::GetAspectRatio () 
{
  return _aspectRatio;
}
