/*
    Copyright (C) 2002 by Jorrit Tyberghein, Daniel Duhprey

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

#include "cssysdef.h"
#include "csutil/scf.h"
#include "ivaria/reporter.h"

#include "alsndrdr.h"
#include "alsndsrc.h"
#include "alsndhdl.h"

SCF_IMPLEMENT_IBASE(csSoundSourceOpenAL)
  SCF_IMPLEMENTS_INTERFACE(iSoundSource)
SCF_IMPLEMENT_IBASE_END;


csSoundSourceOpenAL::csSoundSourceOpenAL(csSoundRenderOpenAL *rdr, 
                                         csSoundHandleOpenAL *hdl)
{
  SCF_CONSTRUCT_IBASE(hdl);

  SoundRender = rdr;
  SoundHandle = hdl;
  const csSoundFormat *f = hdl->Data->GetFormat ();
  frequency=f->Freq;


  // Lock the OpenAL mutex
  SoundRender->mutex_OpenAL->LockWait();

  // Create the OpenAL source
  alGenSources (1, &source);

  // Set looping to false by default
  alSourcei (source, AL_LOOPING, AL_FALSE);

  // Initialize the format
  if (f->Bits == 8) {
    if (f->Channels == 2) {
      format = AL_FORMAT_STEREO8;
    } else {
      format = AL_FORMAT_MONO8;
    }
  } else {
    if (f->Channels == 2) {
      format = AL_FORMAT_STEREO16;
    } else {
      format = AL_FORMAT_MONO16;
    }
  }


  // Generate and fill the buffer if this is static data
  if (hdl->Data->IsStatic ()) {
    ALuint buffer;
    alGenBuffers (1, &buffer);
    int datalen = hdl->Data->GetStaticSampleCount () * f->Bits/8 * f->Channels;
    alBufferData (buffer, format, hdl->Data->GetStaticData (), datalen, frequency);
    alSourceQueueBuffers (source, 1, &buffer);
  } 
  // Otherwise buffers are generated on the fly

  // Set static bool
  Static = SoundHandle->Data->IsStatic();

  // 
  SourcePlaying=false;

  // Set 3d Mode
  mode = SOUND3D_ABSOLUTE;
  alSourcei (source, AL_SOURCE_RELATIVE, AL_FALSE);

  // Unlock the OpenAL mutex
  SoundRender->mutex_OpenAL->Release();

}

csSoundSourceOpenAL::~csSoundSourceOpenAL() 
{
  // Stop the source if it's playing
  Stop();

  SoundRender->mutex_OpenAL->LockWait();

  // Unqueue any queued buffers
  ALuint last_buffer,buffer;
  ALint LastError=AL_NO_ERROR;
  // Clear the last error value - I don't know if this is necessary, the OpenAL spec isn't clear
  alGetError ();

  /*  Under windows at least, the OpenAL driver appears to be able to get stuck in a loop here.
  *    It unqueues the same buffer infinite times.  To protect against this, if the buffer unqueued is
  *    the same on two successive calls, we're done.
  */
  last_buffer=1;
  buffer=0;

  while (LastError==AL_NO_ERROR && buffer!=last_buffer)
  {
    last_buffer=buffer;
    alSourceUnqueueBuffers(source,1,&buffer);
    LastError=alGetError ();
    if (LastError==AL_NO_ERROR && buffer!=last_buffer)
    {
      // Delete buffer
      alDeleteBuffers(1,&buffer);
#ifdef OPENAL_DEBUG_BUFFERS
      Report (CS_REPORTER_SEVERITY_WARNING,
        "Source destructing. Deleted buffer %d!",buffer);
#endif
    }
  }

#ifdef OPENAL_DEBUG_BUFFERS
  ALint queued,processed;
  alGetSourcei (source, AL_BUFFERS_QUEUED, &queued);
  alGetSourcei (source, AL_BUFFERS_PROCESSED, &processed);
  if (queued)
  {
    Report (CS_REPORTER_SEVERITY_WARNING,
      "Source destructing. There are still %d buffers queued!",queued,processed);
  }
#endif

  SoundRender->mutex_OpenAL->Release();

}


void csSoundSourceOpenAL::SetPosition(csVector3 v)
{
  position[0] = v.x; position[1] = v.y; position[2] = v.z;
  SoundRender->mutex_OpenAL->LockWait();
  alSourcefv (source, AL_POSITION, position);
  SoundRender->mutex_OpenAL->Release();
}

void csSoundSourceOpenAL::SetVelocity(csVector3 v)
{
  velocity[0] = v.x; velocity[1] = v.y; velocity[2] = v.z;
  SoundRender->mutex_OpenAL->LockWait();
  alSourcefv (source, AL_VELOCITY, velocity);
  SoundRender->mutex_OpenAL->Release();
}

void csSoundSourceOpenAL::SetVolume(float vol)
{
  SoundRender->mutex_OpenAL->LockWait();
  alSourcef (source, AL_GAIN, vol); 
  SoundRender->mutex_OpenAL->Release();
}

float csSoundSourceOpenAL::GetVolume()
{
  float vol;
  SoundRender->mutex_OpenAL->LockWait();
  alGetSourcef (source, AL_GAIN, &vol);
  SoundRender->mutex_OpenAL->Release();
  return vol;
}

void csSoundSourceOpenAL::SetFrequencyFactor (float factor)
{
  SoundRender->mutex_OpenAL->LockWait();
  alSourcef (source, AL_PITCH, factor);
  SoundRender->mutex_OpenAL->Release();
}

float csSoundSourceOpenAL::GetFrequencyFactor ()
{
  float factor;
  SoundRender->mutex_OpenAL->LockWait();
  alGetSourcef (source, AL_PITCH, &factor);
  SoundRender->mutex_OpenAL->Release();
  return factor;
}

void csSoundSourceOpenAL::SetMode3D(int m) {
  mode = m;
  SoundRender->mutex_OpenAL->LockWait();
  switch (mode) {
  case SOUND3D_ABSOLUTE:
    alSourcei (source, AL_SOURCE_RELATIVE, AL_FALSE);
    break;
  case SOUND3D_RELATIVE:
    alSourcei (source, AL_SOURCE_RELATIVE, AL_TRUE);
    break;
  }
  SoundRender->mutex_OpenAL->Release();
}

bool csSoundSourceOpenAL::IsPlaying() {
  // For static buffers we can go straight to the OpenAL layer.
  if (Static)
  {
    ALint sourcestate;
    SoundRender->mutex_OpenAL->LockWait();
    alGetSourcei(source,AL_SOURCE_STATE,&sourcestate);
    SoundRender->mutex_OpenAL->Release();
    return (sourcestate==AL_PLAYING);
  }
  /* For streaming audio, we have to manually keep track of the playing status since OpenAL
  *  cannot be in a playing state without data queued and there are some situations where
  *  we want the source to be playing without data (underbuffering, and stopped streams).
  */

  return SourcePlaying;
}

void csSoundSourceOpenAL::Play(unsigned long PlayMethod)
{
#ifdef OPENAL_DEBUG_CALLS
  Report (CS_REPORTER_SEVERITY_NOTIFY,  	
    "csSoundSourceOpenAL::Play(%slooping)",(PlayMethod & SOUND_LOOP)? "": "not ");
#endif

  SoundRender->mutex_OpenAL->LockWait();

  // We never loop on streaming sources, and only loop statics 
  if (SoundHandle->Data->IsStatic() && (PlayMethod & SOUND_LOOP))
  {
    alSourcei (source, AL_LOOPING, AL_TRUE);
  }
  else
  {
    alSourcei (source, AL_LOOPING, AL_FALSE);
  }



  // Make sure the source is in a stopped state
  alSourceStop (source);

  SoundRender->mutex_OpenAL->Release();


  // Make sure the handle's buffer is up-to-the-moment
  SoundRender->Update();


  /*  If this is a static data source, the buffer is already filled from Initialize().
  *   In the case of a streaming source we need to be sure that we are synced up with
  *   the rest of the sources playing from this handle.
  */
  if (!Static)
  {
    if (SoundHandle->ActiveStream)
    {
      if (SoundHandle->local_buffer)
      {
        SoundHandle->mutex_WriteCursor->LockWait();
        // Fill our sound buffer with the data from the stream
        Write((char *)(SoundHandle->local_buffer) + SoundHandle->buffer_writecursor,SoundHandle->buffer_length - SoundHandle->buffer_writecursor);
        if (SoundHandle->buffer_writecursor)
          Write(SoundHandle->local_buffer , SoundHandle->buffer_writecursor);
        SoundHandle->mutex_WriteCursor->Release();
      }
      else
      {
        /* We have to fill the buffer with silence to keep sync
        *  We'll use a 10k buffer here and add it a number of times rather than playing with the heap.
        *
        */
        unsigned char Byte = (SoundHandle->Data->GetFormat()->Bits==8)?128:0;
        char SilenceBuffer[10240];
        long bytesleft=SoundHandle->buffer_length;
        long towrite;

        FillMemory(SilenceBuffer,10240, Byte);

        while (bytesleft)
        {
          towrite=(bytesleft>10240) ? 10240 : bytesleft;

          Write(SilenceBuffer,towrite);
          bytesleft-=towrite;
        }
      }
    }

  }
  else
  {
    SoundRender->mutex_OpenAL->LockWait();
    // Static buffers require an explicit play here because Write() is never called.
    alSourcePlay(source);
    SoundRender->mutex_OpenAL->Release();
  }


  // The Write() call above starts playing, or if the stream isnt started we just pretend the source is.
  SourcePlaying=true;

  // Add this source to the renderer's source list
  SoundRender->AddSource(this);

}

void csSoundSourceOpenAL::Stop()
{
#ifdef OPENAL_DEBUG_CALLS
  Report (CS_REPORTER_SEVERITY_NOTIFY,  	
    "csSoundSourceOpenAL::Stop()");
#endif
  // Stop both the OpenAL source and 
  SoundRender->mutex_OpenAL->LockWait();
  alSourceStop (source);
  SoundRender->mutex_OpenAL->Release();
  SourcePlaying=false;
}

void csSoundSourceOpenAL::NotifyStreamEnd()
{
  /*  The sound handle is telling us that it handles a stream which is now done.
  *   We are given a chance to setup variables necessary for watching for the end of our playback buffer.
  */
#ifdef OPENAL_DEBUG_CALLS
  Report (CS_REPORTER_SEVERITY_NOTIFY,  	
    "csSoundSourceOpenAL::NotifyStreamEnd()");
#endif

}


//  Here we check for the last of the buffers to finish emptying.
void csSoundSourceOpenAL::WatchBufferEnd()
{
  int buffer_index;
  int bufferqueuedcount;
  ALuint use_buffer;
  ALint lasterror;

#ifdef OPENAL_DEBUG_CALLS
  Report (CS_REPORTER_SEVERITY_NOTIFY,  	
    "csSoundSourceOpenAL::WatchBufferEnd()");
#endif

  SoundRender->mutex_OpenAL->LockWait();

  /* Check to see if the source is done playing and set our local SourcePlaying variable.
  *   Do this before the released buffers are deleted, though the destructor should clear up
  *   any outstanding buffers anyway.
  */
  ALint sourcestate;

  alGetSourcei(source,AL_SOURCE_STATE,&sourcestate);
  if (sourcestate!=AL_PLAYING && sourcestate!=AL_PAUSED)
    SourcePlaying=false;

  /* Clear the last error value - I don't know if this is necessary, the OpenAL spec isn't clear on wether
  *  alGetError() is reset to AL_NO_ERROR after each OpenAL call, or if it's reset after alGetError().
  * We cover both possibilities here.  There should be no harm in doing this and no real performance hit.
  *
  */
  lasterror=alGetError ();
  lasterror=AL_NO_ERROR;


  //  Release any buffers waiting to be unqueued and delete them.
  while (lasterror==AL_NO_ERROR)
  {
    // dequeue the next buffer
    alSourceUnqueueBuffers(source,1,&use_buffer);
    // Get the error value
    lasterror=alGetError ();
    if (lasterror==AL_NO_ERROR)
    {
      /* Delete the buffer.  The only way the OpenAL spec says this can fail is if the buffer is still queued for
      *  other sources.  We don't queue buffers for more than one source, so an error shouldn't happen.
      * Still, the OpenAL API is rather vague, and we don't really care if there's an error here anyway.
      */
      alDeleteBuffers(1,&use_buffer);
    }
  }

  SoundRender->mutex_OpenAL->Release();


}

void csSoundSourceOpenAL::Write(void *Data, unsigned long NumBytes) 
{
  int lasterror;
  ALuint use_buffer,last_buffer;

  SoundRender->mutex_OpenAL->LockWait();


#ifdef OPENAL_DEBUG_BUFFERS
  ALint queued,processed;
  alGetSourcei (source, AL_BUFFERS_QUEUED, &queued);
  alGetSourcei (source, AL_BUFFERS_PROCESSED, &processed);
  Report (CS_REPORTER_SEVERITY_WARNING,
    "There are %d buffers queued and %d buffers processed.",queued,processed);
#endif

  /* Clear the last error value - I don't know if this is necessary, the OpenAL spec isn't clear on wether
  *  alGetError() is reset to AL_NO_ERROR after each OpenAL call, or if it's reset after alGetError().
  * We cover both possibilities here.  There should be no harm in doing this and no real performance hit.
  *
  */
  lasterror=alGetError ();
  lasterror=AL_NO_ERROR;

  /*  Under windows at least, the OpenAL driver appears to be able to get stuck in a loop here.
  *    It unqueues the same buffer infinite times.  To protect against this, if the buffer unqueued is
  *    the same on two successive calls, we're done.
  */
  last_buffer=1;
  use_buffer=0;

  while (lasterror==AL_NO_ERROR && use_buffer!=last_buffer)
  {
    last_buffer=use_buffer;
    // dequeue the next buffer
    alSourceUnqueueBuffers(source,1,&use_buffer);
    lasterror=alGetError ();
    if (lasterror==AL_NO_ERROR && use_buffer!=last_buffer)
    {
      /* Delete the buffer.  The only way the OpenAL spec says this can fail is if the buffer is still queued for
      *  other sources.  We don't queue buffers for more than one source, so an error shouldn't happen.
      * Still, the OpenAL API is rather vague, and we don't really care if there's an error here anyway.
      */
      alDeleteBuffers(1,&use_buffer);
#ifdef OPENAL_DEBUG_BUFFERS
      Report (CS_REPORTER_SEVERITY_WARNING,
        "Deleted buffer %d!",use_buffer);
#endif
    }
  }





  // Create a new buffer to send this data in
  alGenBuffers (1, &use_buffer);
  // Get the error value
  lasterror=alGetError ();
  if (lasterror!=AL_NO_ERROR)
  {
    Report (CS_REPORTER_SEVERITY_ERROR,
      "Could not generate a buffer.  Error %d.",
      lasterror);
    SoundRender->mutex_OpenAL->Release();
    return;
  }


  // Fill the next free buffer
  alBufferData (use_buffer, format,Data, NumBytes, frequency);
  lasterror=alGetError ();
  if (lasterror!=AL_NO_ERROR)
  {
#ifdef OPENAL_DEBUG_BUFFERS
    Report (CS_REPORTER_SEVERITY_ERROR,
      "Could not write data to buffer.  Error %d.",
      lasterror);
#endif
    SoundRender->mutex_OpenAL->Release();
    return;
  }

  // Queue the filled buffer
  alSourceQueueBuffers (source, 1, &use_buffer);
  lasterror=alGetError ();
  if (lasterror!=AL_NO_ERROR)
  {
#ifdef OPENAL_DEBUG_BUFFERS
    Report (CS_REPORTER_SEVERITY_ERROR,
      "Could not queue buffer.  Error %d.",
      lasterror);
#endif
    SoundRender->mutex_OpenAL->Release();
    return;
  }

  // Now that data is queued, if the source is not in a playing state we need to put it there
  ALint sourcestate;
  alGetSourcei(source,AL_SOURCE_STATE,&sourcestate);
  if (sourcestate!=AL_PLAYING && SourcePlaying)
  {
#ifdef OPENAL_DEBUG_BUFFERS
    Report (CS_REPORTER_SEVERITY_ERROR,
      "Write() found source in a stopped state.  Restarting.");
#endif
    alSourcePlay(source);
  }
  SoundRender->mutex_OpenAL->Release();


}

void csSoundSourceOpenAL::Report (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep = CS_QUERY_REGISTRY (SoundRender->object_reg,
    iReporter);
  if (rep)
    rep->ReportV (severity, "crystalspace.sound.openal", msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}
