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

#include "csutil/scf.h"
#include "csutil/ref.h"

struct iMedia;

/**
 * Container for the different streams inside a video file
 */
struct iMediaContainer : public virtual iBase
{
  SCF_INTERFACE(iMediaContainer,0,1,0);

  /// Returns the number of iMedia objects inside the iMediaContainer
  virtual size_t GetMediaCount () = 0;

  /// Gets the iMedia object at an index
  virtual csRef<iMedia> GetMedia (size_t index) = 0;

  /// Gets the description of the media container
  virtual const char* GetDescription () = 0;

  /// Sets an active stream. In case there's already a stream of that type, it's replaced
  virtual void SetActiveStream (size_t index) = 0;

  /// Removes an active stream.
  virtual bool RemoveActiveStream (size_t index) = 0;

	/// Automatically picks the first stream of every kind from inside the container
	virtual void AutoActivateStreams () = 0;

  /// Updates the active streams
  virtual void Update () = 0;

  /// Checks if end of file has been reached
	virtual bool eof () = 0;

	/// Triggers a seek for the active iMedia streams, resolved at the next update
	virtual void Seek (float time) = 0 ;
};

/** @} */

#endif // __CS_MEDIACONTAINER_H__
