/*
    Copyright (C) 2001 by Andreas H�fler <andreas.hoefler@gmx.at>

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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cssysdef.h"
#include "csutil/util.h"
#include "isound/loader.h"
#include "../common/soundraw.h"
#include "../common/sndload.h"

CS_IMPLEMENT_PLUGIN

// Microsoft Wav file loader
// support 8 and 16 bits PCM (RIFF)

/* ====================================
   short description of wav-file-format (from www.wotsit.org, look there for details)
   ====================================

  WAV-data is contained within the RIFF-file-format. This file format
  consists of headers and "chunks".

  A RIFF file has an 8-byte RIFF header.

  struct 
  {
    char  id[4];   // identifier string = "RIFF"
    DWORD len;     // remaining length after this header
  } riff_hdr;

  The riff_hdr is immediately followed by a 4-byte data type identifier.
  For .WAV files this is "WAVE" as follows:

  char wave_id[4]; // WAVE file identifier = "WAVE"

  The remaining file consists of "chunks". A chunk is following:

  struct
  { 
    char  id[4];   // identifier, e.g. "fmt " or "data"
    DWORD len;     // remaining chunk length after this header
  } chunk_hdr;

  The chunk is followed by the data of the chunk.

  Unexpected chunks should be allowed (and ignored) within a RIFF-file.

  For WAV-files following chunks can be expected:
  (only necessary chunks are listed)

  WAVE Format Chunk, this has to appear before the WAVE Data Chunk:

  struct _FMTchk
  {
    char chunk_id[4];  // for the format-chunk of wav-files always "fmt "
    unsigned long len; // length of this chunk after this 8 bytes of header
    unsigned short fmt_tag; // format category of file. 0x0001 = Microsoft PCM
    unsigned short channel; // number of channels (1 = mono, 2 = stereo)
    unsigned long samples_per_sec; // sampling rate
    unsigned long avg_bytes_per_sec; // for buffer estimation
    unsigned short blk_align; // data block size
    unsigned short bits_per_sample; // sample size  (only if Microsoft PCM)
  } fmtchk;

  WAVE Data Chunk:

  struct _WAVchk
  {
    char chunk_id[4];  // for the data-chunk of wav-files always "data"
    unsigned long len; // length of this chunk after this 8 bytes of header
  }

   ====================================
   End of short format-description
   ====================================

*/

class csSoundLoader_WAV : public iSoundLoader
{
public:
  DECLARE_IBASE;

  csSoundLoader_WAV(iBase *p) {
    CONSTRUCT_IBASE(p);
  }
  virtual bool Initialize(iSystem *) {
    return true;
  }
  virtual iSoundData *LoadSound(void *Buffer, unsigned long Size) const;
};

IMPLEMENT_IBASE(csSoundLoader_WAV)
  IMPLEMENTS_INTERFACE(iSoundLoader)
  IMPLEMENTS_INTERFACE(iPlugIn)
IMPLEMENT_IBASE_END;

IMPLEMENT_FACTORY(csSoundLoader_WAV);

EXPORT_CLASS_TABLE (sndwav)
EXPORT_CLASS (csSoundLoader_WAV,
  "crystalspace.sound.loader.wav", "WAV Sound Loader")
EXPORT_CLASS_TABLE_END;


// header of the RIFF-chunk
struct _RIFFchk
{
  char riff_id[4]; // for RIFF-files always "RIFF"
  unsigned long len; // length of chunk after this 8 bytes of header
  char wave_id[4]; // for wav-files always "WAVE"
} riffchk;

// header of the wav-format-chunk
struct _FMTchk
{
  char chunk_id[4]; // for the format-chunk of wav-files always "fmt "
  unsigned long len; // length of this chunk after this 8 bytes of header
  unsigned short fmt_tag;
  unsigned short channel;
  unsigned long samples_per_sec;
  unsigned long avg_bytes_per_sec;
  unsigned short blk_align;
  unsigned short bits_per_sample;
} fmtchk;

// header of the wav-data-chunk
struct _WAVchk
{
  char chunk_id[4]; // for wav-data-chunk this is always "data"
  unsigned long len; // length of chunk after this 8 bytes of header
} wavchk;



iSoundData* csSoundLoader_WAV::LoadSound (void* databuf, ULong size) const
{
  UByte* buf = (UByte*) databuf;
  csSoundFormat format;
  char* data = NULL;

  int index = 0;

// check if this is a valid wav-file

  // check if file has the size to be able to contain all necessary chunks
  if (size < (sizeof (riffchk) + sizeof (fmtchk) + sizeof (wavchk)))
    return NULL;

  // copy RIFF-header
  if (memcpy(&riffchk, &buf[0], sizeof (riffchk)) == NULL)
    return NULL;

  // check RIFF-header
  if (memcmp (riffchk.riff_id, "RIFF", 4) != 0)
    return NULL;

  // check WAVE-id
  if (memcmp (riffchk.wave_id, "WAVE", 4) != 0)
    return NULL;
  
// find format-chunk, copy it into struct and make corrections if necessary

  // set index after riff-header to the first chunk inside
  index += sizeof (riffchk);
  
  // find format-chunk
  bool found = false; // true, if format-chunk was found
  for ( ;
       (found == false) && ((index + sizeof (fmtchk)) < size) ;
       index += fmtchk.len + 8 // +8, because chunk_id + len are not counted in len
      )
  {
    if (memcpy(&fmtchk, &buf[index], sizeof (fmtchk)) == NULL)
      return NULL;

    if (memcmp(fmtchk.chunk_id, "fmt ", 4) == 0)
      found = true;

    // correct length of chunk on big endian system
    #ifdef CS_BIG_ENDIAN     // @@@ correct fmtchk.len on big-endian systems?
      fmtchk.len = csByteSwap32bit (fmtchk.len);
    #endif
  }

  // no format-chunk found -> no valid file
  if (found == false)
    return NULL;

  // correct the chunk, if under big-endian system
  #ifdef CS_BIG_ENDIAN // @@@ is this correct?
    fmtchk.fmt_tag = csByteSwap16bit (fmtchk.fmt_tag);
    fmtchk.channel = csByteSwap16bit (fmtchk.channel);
    fmtchk.samples_per_sec = csByteSwap32bit (fmtchk.samples_per_sec);
    fmtchk.avg_bytes_per_sec = csByteSwap32bit (fmtchk.avg_bytes_per_sec);
    fmtchk.blk_align = csByteSwap16bit (fmtchk.blk_align);
    fmtchk.bits_per_sample = csByteSwap16bit (fmtchk.bits_per_sample);
  #endif // CS_BIG_ENDIAN

  // check format of file

  // only 1 or 2 channels are valid
  if (!((fmtchk.channel == 1) || (fmtchk.channel == 2)))
     return NULL;

  // only Microsoft PCM wave files are valid
  if (!(fmtchk.fmt_tag == 0x0001))
    return NULL;

// find wav-data-chunk
  found = false; // true, if wav-data-chunk was found
  for ( ;
       (found == false) && ((index + sizeof (wavchk)) < size) ;
       index += wavchk.len + 8 // +8, because chunk_id and len are not counted in len
      )
  {
    if (memcpy(&wavchk, &buf[index], sizeof (wavchk)) == NULL)
      return NULL;

    if (memcmp(wavchk.chunk_id, "data", 4) == 0)
      found = true;

    // correct length of chunk on big endian systems
    #ifdef CS_BIG_ENDIAN     // @@@ correct wavchk.len on big-endian systems?
      wavchk.len = csByteSwap32bit (wavchk.len);
    #endif
  }

  // no wav-data-chunk found -> no valid file
  if (found == false)
    return NULL;

  // index points now after wav-data, so correct it
  index -= wavchk.len;

  // make new buffer, which contains the wav-data
  data = new char[wavchk.len];
    
  // copy the wav-data into the buffer
  if (memcpy(data, &buf[index], wavchk.len)==NULL)
  {
    delete[] data;
    return NULL;
  }
  
  #ifdef CS_BIG_ENDIAN // @@@ correct?
  if (fmtchk.bits_per_sample == 16)
    csByteSwap16bitBuffer ( (unsigned short*)data, wavchk.len / 2);
  #endif // CS_BIG_ENDIAN

  // set up format for csSoundDataRaw
  format.Freq = fmtchk.samples_per_sec;
  format.Bits = fmtchk.bits_per_sample;
  format.Channels = fmtchk.channel;

  // set up sound-buffer
  csSoundDataRaw* rawSound = new csSoundDataRaw(NULL, data,
    (fmtchk.bits_per_sample == 16) ? (wavchk.len/2)-1 : wavchk.len-1, format);

  return rawSound;
}
