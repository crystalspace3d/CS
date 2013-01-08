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
#ifndef __CS_VIDEO_VIDEODECODE_H__
#define __CS_VIDEO_VIDEODECODE_H__

/**\file
 * Video decoding
 */

/**
 * \addtogroup gfx3d
 * @{ */

#include "csutil/ref.h"
#include "csutil/scf.h"
#include "csutil/threadmanager.h"

struct iTextureHandle;
struct iSndSysStream;

namespace CS {
namespace Media {

  /**
   * Used to store languages available for a media file.
   */
  struct MediaLanguage
  {
    /**
     * The name of the language
     */
    const char* name;

    /**
     * The path of the audio file
     */
    const char* path;

    /// Constructor
    MediaLanguage (const char* name, const char* path)
    : name (name), path (path) {}
  };

  /**
   * Base of media streams
   */
  struct iMedia : public virtual iBase
  {
    SCF_INTERFACE (iMedia,0,1,0);

    /**
     * Get the name of this stream.
     */
    virtual const char* GetName () const = 0;  

    /**
     * Get the media type of this stream.
     */
    virtual const char* GetType () const = 0;

    /**
     * Get size of this stream in frames.
     */
    virtual unsigned long GetFrameCount () const = 0;

    /**
     * Return the stream length in seconds.
     */
    virtual float GetLength () const = 0;

    /**
     * Get the position of the stream in seconds.
     */
    virtual double GetPosition () const = 0 ;

    /**
     * Clear all the decoders of the stream. Done when destroying 
     * the object by the container.
     */
    virtual void CleanMedia () = 0 ;

    /**
     * Perform frame-specific updates on the stream.
     */
    virtual bool Update () = 0 ;

    /**
     * Get data from the prefetch queue and write it to the active buffer.
     */
    virtual void WriteData () = 0 ;
  
    /**
     * Swap the active buffer for the last one that was written to.
     */
    virtual void SwapBuffers () = 0;
  
    /**
     * Set the number of frames to be cached.
     * \param[in] size Number of frames
     */
    virtual void SetCacheSize (size_t size) = 0;

    /**
     * Return true if there is data ready to be used.
     */
    virtual bool HasDataReady () = 0;

    /**
     * Return true if the cache is full.
     */
    virtual bool IsCacheFull () = 0;

    /**
     * Trigger a frame drop.
     */
    virtual void DropFrame () = 0;
  };

  /**
   * Video stream
   */
  struct iVideoMedia : public iMedia
  {
    SCF_INTERFACE (iVideoMedia,0,1,0);

    /**
     * Return the aspect ratio to use with the image.
     */
    virtual float GetAspectRatio () = 0;

    /**
     * Get a reference to the internal texture buffer.
     * \return the target texture
     */
    virtual iTextureHandle* GetVideoTarget () = 0;

    /**
     * Return the target FPS of the video stream.
     */
    virtual double GetTargetFPS () = 0;
  };

  /**
   * Audio stream
   */
  struct iAudioMedia : public iMedia
  {
    SCF_INTERFACE (iAudioMedia,0,1,0);

    /**
     * Get a reference to the internal audio stream.
     * \param[out] stream Target audio stream
     */
    virtual void GetAudioTarget (csRef<iSndSysStream> &stream) = 0;
  };

  /**
   * Container for the different streams inside a video file
   */
  struct iMediaContainer : public virtual iBase
  {
    SCF_INTERFACE (iMediaContainer,0,1,0);

    /**
     * Return the number of iMedia objects inside the container.
     */
    virtual size_t GetMediaCount () const = 0;

    /**
     * Get an iMedia object specified by its index.
     * \param[in] index Index of a media stream
     */
    virtual csRef<iMedia> GetMedia (size_t index) = 0;

    /**
     * Get the description of the media container.
     */
    virtual const char* GetDescription () const = 0;

    /**
     * Activate a stream. 
     * In case there's already an activated stream of that type, it is replaced.
     * \param[in] index Index of a media stream
     */
    virtual void SetActiveStream (size_t index) = 0;

    /**
     * Deactivate a stream.
     * \param[in] index Index of an active media stream
     */
    virtual bool RemoveActiveStream (size_t index) = 0;

    /**
     * Automatically activate the first stream of every kind 
     * from inside the container.
     */
    virtual void AutoActivateStreams () = 0;

    /**
     * Get a reference to the internal texture buffer.
     * \return the target texture
     */
    virtual iTextureHandle* GetTargetTexture () = 0;

    /**
     * Get a reference to the internal audio stream.
     * \return the target audio stream
     */
    virtual iSndSysStream* GetTargetAudio () = 0;

    /**
     * Update the active streams.
     */
    virtual void Update () = 0;

    /**
     * Check if end of file has been reached.
     */
    virtual bool Eof () const = 0;

    /**
     * Trigger a seek for the active iMedia streams, resolved at the next update.
     */
    virtual void SetPosition (float time) = 0;

    /**
     * Get the position of the media in seconds.
     */
    virtual float GetPosition () const = 0;

    /**
     * Get the length of the media in seconds.
     */
    virtual float GetLength () const = 0;

    /**
     * Swap the active buffer for the last one that was written to 
     * inside the active iMedia.
     */
    virtual void SwapBuffers () = 0;

    /**
     * Get data from the prefetching queue and write it to the active buffer.
     */
    virtual void WriteData () = 0 ;

    /**
     * Set the number of frames to be cached.
     */
    virtual void SetCacheSize (size_t size) = 0;

    /**
     * Get the aspect ratio associated to the active video stream.
     */
    virtual float GetAspectRatio () = 0;

    /**
     * Trigger a frame drop.
     */
    virtual void DropFrame () = 0;

    /**
     * Get the number of languages in the container.
     */
    virtual size_t GetLanguageCount () const = 0;

    /**
     * Get the language at the given index
     * \param index Index of the language, 
     *            defined in the interval [0,GetLanguageCount ()-1]
     */
    virtual const MediaLanguage& GetLanguage (size_t index) const = 0;

    /**
     * Set the current language to be played
     * \param identifier The name of the target language
     */
    virtual void SetCurrentLanguage (const char* identifier) = 0;

    /**
     * Callback for the "Pause" command.
     */
    virtual void OnPause () = 0;

    /**
     * Callback for the "Play" command.
     */
    virtual void OnPlay () = 0;

    /**
     * Callback for the "Stop" command.
     */
    virtual void OnStop () = 0;
  };

  /**
   * The video loader is used to initialize the decoder
   */
  struct iMediaLoader : public virtual iBase
  {
    SCF_INTERFACE (iMediaLoader,0,1,0);

    /**
     * Create an iMediaContainer from raw input data.
     *
     * Optional pDescription may point to a brief description that will follow this data
     *   through any streams or sources created from it, and may be useful for display or
     *   diagnostic purposes.
     *
     * \param[in] pFileName Path to the file that needs to be loaded
     * \param[in] pDescription A description for the resulting media stream (optional)
     */
    virtual csRef<iMediaContainer> LoadMedia (const char * pFileName, const char *pDescription = 0) = 0;

    /**
     * Initialize a loader. Should never be called directly.
     * \param[in] path Path to the media file
     * \param[in] languages List of available languages
     */
    virtual void Create (csString path, csArray<MediaLanguage> languages) = 0;
  };

  /**
   * The video player
   */
  struct iMediaPlayer : public virtual iBase
  {
    SCF_INTERFACE (iMediaPlayer,0,1,0);

    /**
     * Initialize the video player.
     * \param[in] media The media container to be used by the player
     * \param[in] cacheSize The number of frames to cache. If caching is not needed, use 1.
     */
    virtual void InitializePlayer (csRef<iMediaContainer> media, size_t cacheSize = 1) = 0;

    /**
     * Set the size of the cache, that is the number of frames in the cache. If caching is
     * not needed, then use 1 (default value).
     */
    //virtual void SetCacheSize (size_t size) = 0;

    /**
     * Activate a stream from the media container.
     * \param[in] index Index of a media stream
     */
    virtual void SetActiveStream (int index) = 0;

    /**
     * Deactivate a stream from the media container.
     * \param[in] index Index of a media stream
     */
    virtual void RemoveActiveStream (int index) = 0;

    /**
     * Get a reference to the internal texture buffer.
     * \return the target texture
     */
    virtual iTextureHandle* GetTargetTexture () = 0;

    /**
     * Get a reference to the internal audio stream.
     * \return the target audio stream
     */
    virtual iSndSysStream* GetTargetAudio () = 0;

    /**
     * Called continuously to update the player. 
     * The user shouldn't call this method directly.
     * To start and stop the player, use StartPlayer () and StopPlayer ()
     */
    THREADED_INTERFACE ( Update );

    /**
     * Start the Update thread for the media player. In order to run the player
     * properly, this method must be called when starting the player.
     */
    virtual void StartPlayer () = 0;

    /**
     * Stop the Update thread for the media player. In order to close the player
     * properly, this method must be called when shutting down the player.
     */
    virtual void StopPlayer () = 0;

    /// Set whether or not the media has to be played cyclically. The default value is \a true.
    virtual void SetCyclic (bool cyclic) = 0;

    /// Get whether or not the media has to be played cyclically.
    virtual bool GetCyclic () const = 0;

    /**
     * Start playing the media.
     */
    virtual void Play () = 0;

    /**
     * Pause the media.
     */
    virtual void Pause () = 0;

    /**
     * Stop the media and seek to the beginning.
     */
    virtual void Stop () = 0;

    /**
     * Seek the media to a specified time.
     */
    virtual void SetPosition (float time) = 0;

    /**
     * Get the position of the media in seconds.
     */
    virtual float GetPosition () const = 0;

    /**
     * Get the length of the media in seconds.
     */
    virtual float GetLength () const = 0;

    /**
     * Check if the media is playing or not.
     */
    virtual bool IsPlaying () = 0;

    /**
     * Get the aspect ratio of the active video stream.
     */  
    virtual float GetAspectRatio () = 0;

    /**
     * Select a language from the available ones.
     */
    virtual void SetLanguage (const char* identifier) = 0;
  };

} // namespace Media
} // namespace CS

/** @} */

#endif // __CS_VIDEO_VIDEODECODE_H__
