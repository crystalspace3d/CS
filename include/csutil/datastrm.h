/*
    Copyright (C) 2001 by Martin Geisse <mgeisse@gmx.net>

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

#ifndef __DATASTRM_H__
#define __DATASTRM_H__

struct iDataBuffer;

/**
 * This class can be used as a wrapper around a data buffer for easy
 * stream-like access.
 */
class csDataStream
{
private:
  uint8 *Data;
  int Position, Size;
  bool DeleteBuffer;

public:
  /// constructor
  csDataStream (void *buf, int Size, bool DeleteBuffer = true);
  /// destructor
  ~csDataStream ();

  /// Return the current position
  int GetPosition ();
  /// Set the current position
  void SetPosition (int pos);
  /// Return the length of the stream
  int GetLength ();
  /// Returns true if the stream has finished
  bool Finished ();

  /// Read a buffer of data. Returns the number of bytes actually read.
  int Read (void *buf, int NumBytes);

  /// Read a one-byte value. Returns false on EOF.
  bool ReadInt8 (int8 &val);
  /// Read an unsigned one-byte value. Returns false on EOF.
  bool ReadUInt8 (uint8 &val);
  /// Read a two-byte value
  bool ReadInt16 (int16 &val);
  /// Read an unsigned two-byte value. Returns false on EOF.
  bool ReadUInt16 (uint16 &val);
  /// Read a four-byte value
  bool ReadInt32 (int32 &val);
  /// Read an unsigned four-byte value. Returns false on EOF.
  bool ReadUInt32 (uint32 &val);

  /// Read a single character. Returns EOF if the stream has finished.
  int GetChar ();
  /**
   * Read a line of text. Returns false if the stream has finished. If
   * 'OmitNewline' is true then the newline character will be thrown away.
   */
  bool GetString (char* buf, int len, bool OmitNewline = true);
};

#endif // __DATASTRM_H__
