/*
    Copyright (C) 1998 by Jorrit Tyberghein
    File Created by Gary Haussmann 1/30/2000

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

// place your code here to detect the OpenGL extensions.
// the current code is a sample lifted from the Mesa extension detection.
// The code here should set boolean values such as this->ARB_multitexture
// if it detects the appropriate extension.  You also need to add
// some code to ext_auto.cpp in order to include this file for appropriate
// platforms. -GJH

void csGraphics3DOpenGL::DetectExtensions()
{
	SysPrintf (MSG_INITIALIZATION, "Detecting yer gnarly OpenGL extensions.\n");
	const unsigned char * extensions;

	extensions=glGetString(GL_EXTENSIONS);
	if(!extensions) return;	//No luck, no extensions on this machine.

	// check the extension string for each extension in turn
	for (int i=0; i < NUM_EXTENSIONS; i++)
	{
		const char * searchresult = strstr((const char *)extensions, knownExtensions[i]);

		while (searchresult)
		{
			// make sure we didn't accidently catch the
			// substring of some other extension
			if ( ( *(searchresult+strlen(knownExtensions[i])) == ' ') ||
			     ( *(searchresult+strlen(knownExtensions[i])) == '\0') )
			{
				SysPrintf(MSG_INITIALIZATION,"Found extension: %s\n",knownExtensions[i]);

				// i is now an index, so use it
				switch (i)
				{
				case 0:
					ARB_multitexture=true;	//Flag that should be checked in renderer

				default:
					break;
				}
			}

			// find next occurance -- we could have multiple matches if we match the
			// substring of another extension, but only one will trigger the if statement
			// above
			searchresult = strstr(searchresult+1,knownExtensions[i]);
		}
	}
}

