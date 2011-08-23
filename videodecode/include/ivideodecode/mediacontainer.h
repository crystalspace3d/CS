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

#ifndef __CS_MEDIACONTAINER_H__
#define __CS_MEDIACONTAINER_H__

/**\file
  * Video Player: media container 
  */
/**
 * \addtogroup videoplay
 * @{ */

#include "csutil/scf.h"
#include "csutil/ref.h"
#include "crystalspace.h"

struct iMedia;

/**
  * Container for the different streams inside a video file
  */
struct iMediaContainer : public virtual iBase
{
  SCF_INTERFACE (iMediaContainer,0,1,0);

  /**
    * Returns the number of iMedia objects inside the iMediaContainer
    */
  virtual size_t GetMediaCount () const = 0;

  /**
    * Get the iMedia object at an index
    * \param[in] index Object index
    */
  virtual csRef<iMedia> GetMedia (size_t index) = 0;

  /**
    * Get the description of the media container
    */
  virtual const char* GetDescription () const = 0;

  /**
    * Activate a stream. In case there's already a stream of that type, it's replaced
    * \param[in] index Object index
    */
  virtual void SetActiveStream (size_t index) = 0;

  /**
    * Removes an active stream.
    * \param[in] index Object index
    */
  virtual bool RemoveActiveStream (size_t index) = 0;

  /**
    * Automatically pick the first stream of every kind from inside the container
    */
  virtual void AutoActivateStreams () = 0;

  /**
    * Get a reference to the internal texture buffer
    * \param[out] texture Target texture
    */
  virtual void GetTargetTexture (csRef<iTextureHandle> &target) = 0;

  /**
    *  Get a reference to the internal audio stream
    * \param[out] target Target audio stream
    */
  virtual void GetTargetAudio (csRef<iSndSysStream> &target) = 0;

  /**
    * Update the active streams
    */
  virtual void Update () = 0;

  /**
    * Check if end of file has been reached
    */
  virtual bool Eof () const = 0;

  /**
    * Trigger a seek for the active iMedia streams, resolved at the next update
    */
  virtual void Seek (float time) = 0 ;

  /**
    * Get the position of the media
    */
  virtual float GetPosition () const = 0 ;

  /**
    * Get the length of the media
    */
  virtual float GetLength () const = 0 ;

  /**
    * Swaps the active buffer for the one that was written to last inside the active iMedia
    */
  virtual void SwapBuffers () = 0;

  /**
    * Gets data from the prefetch queue and writes it to the active buffer
    */
  virtual void WriteData () = 0 ;

  /**
    * Set how many frames will be cached
    */
  virtual void SetCacheSize (size_t size) = 0;

  /**
    * Get the aspect ratio associated to the active video stream
    */
  virtual float GetAspectRatio () = 0;

  /**
    * Trigger a frame drop
    */
  virtual void DropFrame () = 0;

  /**
    * Select a language from the available ones
    * \param[in] identifier A string identifier for the target language
    */
  virtual void SelectLanguage (const char* identifier) = 0;

  /**
    * Callback for the "Pause" command
    */
  virtual void OnPause () = 0;

  /**
    * Callback for the "Play" commandy
    */
  virtual void OnPlay () = 0;

  /**
    * Callback for the "Stop" command
    */
  virtual void OnStop () = 0;
};

/** @} */

#endif // __CS_MEDIACONTAINER_H__
