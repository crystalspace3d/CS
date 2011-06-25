#include <cssysdef.h>
#include "vplPlayer.h"
#include <iutil/objreg.h>
#include <iutil/plugin.h>

#include <iostream>
using namespace std;


SCF_IMPLEMENT_FACTORY (vplPlayer)

vplPlayer::vplPlayer (iBase* parent) :
scfImplementationType (this, parent),
object_reg(0)
{
}

vplPlayer::~vplPlayer ()
{
}

bool vplPlayer::Initialize (iObjectRegistry* r)
{
	object_reg = r;

	_mediaFile=NULL;
	playing = false;
	return true;
}

void vplPlayer::InitializePlayer (csRef<iMediaContainer> media)
{
	if (!media.IsValid())
		csReport(object_reg, CS_REPORTER_SEVERITY_WARNING, QUALIFIED_PLUGIN_NAME,
		"Media container is not valid!");
	else
		_mediaFile = media;
}

/// Activates a stream from inside the iMediaContainer
void vplPlayer::SetActiveStream (int index) 
{
	if (_mediaFile.IsValid ())
	{
		if(index==-1)
			_mediaFile->AutoActivateStreams ();
		else
			_mediaFile->SetActiveStream (index);
	}
}

/// Deactivates a stream from inside the iMediaContainer
void vplPlayer::RemoveActiveAudioStream (int index) 
{
	if (_mediaFile.IsValid ())
		_mediaFile->RemoveActiveStream (index);
}

/// Set the target texture
void vplPlayer::SetTargetTexture (csRef<iTextureHandle> target) 
{
	_target = target;
}

/// Called continuously to update the player
void vplPlayer::Update ()
{
	if (playing)
		if( _mediaFile.IsValid ())
		{
			if( _mediaFile->eof ())
				playing=false;
			else
				_mediaFile->Update ();
		}
}

/// Starts playing the media
void vplPlayer::Play () 
{
	playing=true;
}

/// Pauses the media
void vplPlayer::Pause() 
{
	playing=false;
}

/// Stops the media and seeks to the beginning
void vplPlayer::Stop () 
{
	// Stop playing
	playing=false;
	// Seek back to the beginning of the stream
	_mediaFile->Seek (0.0f);
}

/// Seeks the media
void vplPlayer::Seek (float time)
{
	if (_mediaFile.IsValid ())
		_mediaFile->Seek (time);
}

/// Get the position of the media
csTicks vplPlayer::GetPosition () 
{
	return 0;
}

/// Returns if the media is playing or not
bool vplPlayer::IsPlaying () 
{
	return playing;
}