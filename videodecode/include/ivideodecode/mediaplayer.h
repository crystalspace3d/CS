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

#ifndef __CS_MEDIAPLAYER_H__
#define __CS_MEDIAPLAYER_H__

/**\file
  * Video Player: media container 
  */

#include "csutil/scf.h"
#include "csutil/ref.h"
#include "crystalspace.h" 	

#include "iutil/threadmanager.h"

struct iMediaContainer;
struct iTextureHandle;

/**
  * The video player
  */
struct iMediaPlayer : public virtual iBase
{
  SCF_INTERFACE (iMediaPlayer,0,1,0);

  /// Initialize the video player
  /// cacheSize refers to the number of frames will be cached
  /// If you don't want to use caching, use 1
  virtual void InitializePlayer (csRef<iMediaContainer> media, size_t cacheSize = 1) = 0;

  /// Activates a stream from inside the iMediaContainer
  virtual void SetActiveStream (int index) = 0;

  /// Deactivates a stream from inside the iMediaContainer
  virtual void RemoveActiveStream (int index) = 0;

  /// Makes "Target" point to the internal iTextureHandle of the active stream
  virtual void GetTargetTexture (csRef<iTextureHandle> &target) = 0;

  /// Called continuously to update the player. The user shouldn't call this method directly
  /// To start and stop the player, use StartPlayer() and StopPlayer()
  THREADED_INTERFACE( Update );

  /// Start the update thread for the media player
  virtual void StartPlayer() = 0;

  /// Stops the update thread for the media player. In order to close the application
  /// properly, this must be called when shutting down the application.
  virtual void StopPlayer() = 0;

  /// Enable/disable looping
  virtual void Loop (bool shouldLoop) = 0 ;

  /// Starts playing the media
  virtual void Play () = 0 ;

  /// Pauses the media
  virtual void Pause() = 0 ;

  /// Stops the media and seeks to the beginning
  virtual void Stop () = 0 ;

  /// Seeks the media
  virtual void Seek (float time) = 0 ;

  /// Get the position of the media
  virtual float GetPosition () const = 0 ;

  /// Get the length of the media
  virtual float GetLength () const = 0 ;

  /// Returns if the media is playing or not
  virtual bool IsPlaying () = 0 ;
};

/** @} */

#endif // __CS_MEDIAPLAYER_H__
