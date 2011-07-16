/*

	DXTCGen.cpp - Bounds & variance based codebook generator class

*/

#include "common.h"
#include "DXTCGen.h"

CS_PLUGIN_NAMESPACE_BEGIN(DDSImageIO)
{
namespace ImageLib
{

const long ScanRange = 16;		// Uses a range of +/- ScanRange/2 for brute force
const long ScanStep = 4;		// Step increment for the brute force scan

DXTCGen::DXTCGen()
{
	Vects.SetCount(8);
	pVects = &Vects[0];
}

// ----------------------------------------------------------------------------
// Build palette for a 3 color + traparent black block
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes3(cbVector *pVects, cbVector &v1, cbVector &v2)
{
	pVects[0] = v1;
	pVects[2] = v2;
	pVects[1][0] = v1[0];
	pVects[1][1] = (BYTE)( ((long)v1[1] + (long)v2[1]) / 2 );
	pVects[1][2] = (BYTE)( ((long)v1[2] + (long)v2[2]) / 2 );
	pVects[1][3] = (BYTE)( ((long)v1[3] + (long)v2[3]) / 2 );

	/*
	__asm {
		mov			ecx, dword ptr pVects
		mov			eax, dword ptr v1
		mov			ebx, dword ptr v2

		movd		mm0, [eax]
		movd		mm1, [ebx]
		pxor		mm2, mm2
		nop

		movd		[ecx], mm0
		movd		[ecx+8], mm1

		punpcklbw	mm0, mm2
		punpcklbw	mm1, mm2

		paddw		mm0, mm1
		psrlw		mm0, 1

		packuswb	mm0, mm0
		movd		[ecx+4], mm0
	}
	*/
	//*(long *)&pVects[1] = r1;
}

// ----------------------------------------------------------------------------
// Build palette for a 4 color block
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes4(cbVector *pVects, cbVector &v1, cbVector &v2)
{
	
	pVects[0] = v1;
	pVects[3] = v2;

	pVects[1][0] = v1[0];
	pVects[1][1] = (BYTE)( ((long)v1[1] * 2 + (long)v2[1]) / 3 );
	pVects[1][2] = (BYTE)( ((long)v1[2] * 2 + (long)v2[2]) / 3 );
	pVects[1][3] = (BYTE)( ((long)v1[3] * 2 + (long)v2[3]) / 3 );

	pVects[2][0] = v1[0];
	pVects[2][1] = (BYTE)( ((long)v2[1] * 2 + (long)v1[1]) / 3 );
	pVects[2][2] = (BYTE)( ((long)v2[2] * 2 + (long)v1[2]) / 3 );
	pVects[2][3] = (BYTE)( ((long)v2[3] * 2 + (long)v1[3]) / 3 );
	
	/*
	__asm {
		mov			ecx, dword ptr pVects
		mov			eax, dword ptr v1
		mov			ebx, dword ptr v2

		movd		mm0, [eax]
		movd		mm1, [ebx]

		pxor		mm2, mm2
		movd		[ecx], mm0
		movd		[ecx+12], mm1

		punpcklbw	mm0, mm2
		punpcklbw	mm1, mm2
		movq		mm3, mm0		// mm3 = v0

		paddw		mm0, mm1		// mm0 = v0 + v1
		paddw		mm3, mm3		// mm3 = v0*2

		paddw		mm0, mm1		// mm0 = v0 + v1*2
		paddw		mm1, mm3		// mm1 = v0*2 + v1

		pmulhw		mm0, ScaleOneThird
		pmulhw		mm1, ScaleOneThird
		packuswb	mm1, mm0

		movq		[ecx+4], mm1
	}
	*/
}

// ----------------------------------------------------------------------------
// Build palette for a 6-interpolant alpha block
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes8(cbVector *pVects, cbVector &v1, cbVector &v2)
{
	
	pVects[0] = v1;
	pVects[1] = v2;

	pVects[2][0] = (BYTE)( ((long)v1[0] * 6 + (long)v2[0] * 1) / 7 );
	pVects[2][1] = pVects[2][2] = pVects[2][3] = 0;

	pVects[3][0] = (BYTE)( ((long)v1[0] * 5 + (long)v2[0] * 2) / 7 );
	pVects[3][1] = pVects[3][2] = pVects[3][3] = 0;

	pVects[4][0] = (BYTE)( ((long)v1[0] * 4 + (long)v2[0] * 3) / 7 );
	pVects[4][1] = pVects[4][2] = pVects[4][3] = 0;

	pVects[5][0] = (BYTE)( ((long)v1[0] * 3 + (long)v2[0] * 4) / 7 );
	pVects[5][1] = pVects[5][2] = pVects[5][3] = 0;

	pVects[6][0] = (BYTE)( ((long)v1[0] * 2 + (long)v2[0] * 5) / 7 );
	pVects[6][1] = pVects[6][2] = pVects[6][3] = 0;

	pVects[7][0] = (BYTE)( ((long)v1[0] * 1 + (long)v2[0] * 6) / 7 );
	pVects[7][1] = pVects[7][2] = pVects[7][3] = 0;
}

// ----------------------------------------------------------------------------
// Build palette for a 4-interpolant alpha block
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes6(cbVector *pVects, cbVector &v1, cbVector &v2)
{
	
	pVects[0] = v1;
	pVects[1] = v2;

	pVects[2][0] = (BYTE)( ((long)v1[0] * 4 + (long)v2[0] * 1) / 5 );
	pVects[2][1] = pVects[2][2] = pVects[2][3] = 0;

	pVects[3][0] = (BYTE)( ((long)v1[0] * 3 + (long)v2[0] * 2) / 5 );
	pVects[3][1] = pVects[3][2] = pVects[3][3] = 0;

	pVects[4][0] = (BYTE)( ((long)v1[0] * 2 + (long)v2[0] * 3) / 5 );
	pVects[4][1] = pVects[4][2] = pVects[4][3] = 0;

	pVects[5][0] = (BYTE)( ((long)v1[0] * 1 + (long)v2[0] * 4) / 5 );
	pVects[5][1] = pVects[5][2] = pVects[5][3] = 0;

	pVects[6][0] = 0;
	pVects[6][1] = pVects[6][2] = pVects[6][3] = 0;

	pVects[7][0] = 255;
	pVects[7][1] = pVects[7][2] = pVects[7][3] = 0;
}

// ----------------------------------------------------------------------------
// Rebuild a single channel of the current palette (3 color)
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes3(cbVector *pVects, long Channel, cbVector &v1, cbVector &v2)
{
	pVects[0][Channel] = v1[Channel];
	pVects[2][Channel] = v2[Channel];

	pVects[1][Channel] = (BYTE)( ((long)v1[Channel] + (long)v2[Channel]) / 2 );
}

// ----------------------------------------------------------------------------
// Rebuild a single channel of the current palette (4 color)
// ----------------------------------------------------------------------------
void DXTCGen::BuildCodes4(cbVector *pVects, long Channel, cbVector &v1, cbVector &v2)
{
	pVects[0][Channel] = v1[Channel];
	pVects[3][Channel] = v2[Channel];

	pVects[1][Channel] = (BYTE)( ((long)v1[Channel] * 2 + (long)v2[Channel]) / 3 );
	pVects[2][Channel] = (BYTE)( ((long)v2[Channel] * 2 + (long)v1[Channel]) / 3 );
}

// ----------------------------------------------------------------------------
// Compute the error Sum(dist^2) of a block when palettized
// ----------------------------------------------------------------------------
long DXTCGen::ComputeError3(CodeBook &Pixels)
{
long Error = 0, i;
long Count = Pixels.NumCodes();

	for(i=0; i<Count; i++)
		Error += Vects.ClosestError(3, Pixels[i]);

	return Error;
}


// ----------------------------------------------------------------------------
// Compute the error Sum(dist^2) of a block when palettized
// ----------------------------------------------------------------------------
long DXTCGen::ComputeError4(CodeBook &Pixels)
{
long Error = 0, i;
long Count = Pixels.NumCodes();

	for(i=0; i<Count; i++)
		Error += Vects.ClosestError(4, Pixels[i]);

	return Error;
}


// ----------------------------------------------------------------------------
// Compute the error Sum(dist^2) of a block when palettized
// ----------------------------------------------------------------------------
long DXTCGen::ComputeError8(CodeBook &Pixels)
{
long Error = 0, i;
long Count = Pixels.NumCodes();

	for(i=0; i<Count; i++)
		Error += Vects.ClosestError(8, Pixels[i]);

	return Error;
}


// ----------------------------------------------------------------------------
// Map a 4x4 pixel block to a 3 color w/transparent black DXT1 block
//
// (in)  Source = unique colors in "Pixels"  (1 - 16 codes)
// (in)  Pixels = 4 x 4 pixels to be remapped
// (out) Dest = 3 output colors that best approximate the input (0, 2 are the endpoints)
// ----------------------------------------------------------------------------
long DXTCGen::Execute3(CodeBook &Source, CodeBook &Pixels, CodeBook &Dest)
{
long Count, i, j, c;
long BestIndex[2], BestError;
long Error, Start0, Start1, End0, End1;

	// Do the initial selection of end points based on colors in the block
	BestIndex[0] = BestIndex[1] = 0;
	BestError = 1 << 30;

	Count = Source.NumCodes();
	for(i=0; i<(Count-1); i++)
	{
		for(j=i+1; j<Count; j++)
		{
			BuildCodes3(pVects, Source[i], Source[j]);		// Build the interpolants
			Error = ComputeError3(Pixels);			// Compute the RMS error for Pixels

			if(Error < BestError)
			{
				BestError = Error;
				BestIndex[0] = i;
				BestIndex[1] = j;
			}
		}
	}

	Best[0] = Source[BestIndex[0]];
	Best[1] = Source[BestIndex[1]];
	Test[0] = Best[0];
	Test[1] = Best[1];

	// Now move the two end points around (do a brute force search) to see if we can
	// improve the quality of the match
	for(c=1; c<4; c++)
	{
		Test[0] = Best[0];
		Test[1] = Best[1];
		if(Test[0][c] == Test[1][c])
			continue;

		BuildCodes3(pVects, Test[0], Test[1]);				// Build the full set of interpolants

		Start0 = (long)Test[0][c] - ScanRange/2;
		End0 = Start0 + ScanRange;

		Start0 = __max(0, Start0);
		End0 = __min(255, End0);

		Start1 = (long)Test[1][c] - ScanRange/2;
		End1 = Start1 + ScanRange;

		Start1 = __max(0, Start1);
		End1 = __min(255, End1);

		for(i=Start0; i<=End0; i+=ScanStep)
		{
			Test[0][c] = (BYTE)i;
			for(j=Start1; j<=End1; j+=ScanStep)
			{
				Test[1][c] = (BYTE)j;
				BuildCodes3(pVects, c, Test[0], Test[1]);	// Build the channel interpolants
				Error = ComputeError3(Pixels);		// Compute the RMS error for Pixels

				if(Error < BestError)
				{
					BestError = Error;
					Best[0][c] = (BYTE)i;
					Best[1][c] = (BYTE)j;
				}
			}
		}
	}

	// Build the palette for the best match
	BuildCodes3(pVects, Best[0], Best[1]);

	// Store the results
	Dest[0] = pVects[0];
	Dest[1] = pVects[1];
	Dest[2] = pVects[2];

	//__asm emms;
	return BestError;
}

// ----------------------------------------------------------------------------
// Map a 4x4 pixel block to a 4 color DXT1 block
//
// (in)  Source = unique colors in "Pixels"  (1 - 16 codes)
// (in)  Pixels = 4 x 4 pixels to be remapped
// (out) Dest = 4 output colors that best approximate the input (0, 3 are the endpoints)
// ----------------------------------------------------------------------------
long DXTCGen::Execute4(CodeBook &Source, CodeBook &Pixels, CodeBook &Dest)
{
long Count, i, j, c;
long BestIndex[2], BestError;
long Error, Start0, Start1, End0, End1;

	BestIndex[0] = BestIndex[1] = 0;
	BestError = 1 << 30;

	// Do the initial selection of end points based on colors in the block
	Count = Source.NumCodes();
	for(i=0; i<(Count-1); i++)
	{
		for(j=i+1; j<Count; j++)
		{
			BuildCodes4(pVects, Source[i], Source[j]);		// Build the interpolants
			Error = ComputeError4(Pixels);			// Compute the RMS error for Pixels

			if(Error < BestError)
			{
				BestError = Error;
				BestIndex[0] = i;
				BestIndex[1] = j;
			}
		}
	}

	Best[0] = Source[BestIndex[0]];
	Best[1] = Source[BestIndex[1]];
	Test[0] = Best[0];
	Test[1] = Best[1];

	// Now move the two end points around (do a brute force search) to see if we can
	// improve the quality of the match
	for(c=1; c<4; c++)
	{
		Test[0] = Best[0];
		Test[1] = Best[1];
		if(Test[0][c] == Test[1][c])
			continue;

		BuildCodes4(pVects, Test[0], Test[1]);				// Build the full set of interpolants

		Start0 = (long)Test[0][c] - ScanRange/2;
		End0 = Start0 + ScanRange;

		Start0 = __max(0, Start0);
		End0 = __min(255, End0);

		Start1 = (long)Test[1][c] - ScanRange/2;
		End1 = Start1 + ScanRange;

		Start1 = __max(0, Start1);
		End1 = __min(255, End1);

		for(i=Start0; i<=End0; i+=ScanStep)
		{
			Test[0][c] = (BYTE)i;
			for(j=Start1; j<=End1; j+=ScanStep)
			{
				Test[1][c] = (BYTE)j;
				BuildCodes4(pVects, c, Test[0], Test[1]);	// Build the channel interpolants
				Error = ComputeError4(Pixels);		// Compute the RMS error for Pixels

				if(Error < BestError)
				{
					BestError = Error;
					Best[0][c] = (BYTE)i;
					Best[1][c] = (BYTE)j;
				}
			}
		}
	}

	BuildCodes4(pVects, Best[0], Best[1]);

	Dest[0] = pVects[0];
	Dest[1] = pVects[1];
	Dest[2] = pVects[2];
	Dest[3] = pVects[3];
	//__asm emms;
	return BestError;
}

// ----------------------------------------------------------------------------
// Map a 4x4 pixel block to a 6 interpolant DXT5 alpha block
//
// (in)  Source = unique colors in "Pixels"  (1 - 16 codes)
// (in)  Pixels = 4 x 4 pixels to be remapped
// (out) Dest = 8 output colors that best approximate the input (0, 1 are the endpoints)
// ----------------------------------------------------------------------------
long DXTCGen::Execute8(CodeBook &Source, CodeBook &Pixels, CodeBook &Dest)
{
long Count, i, j, c;
long BestIndex[2], BestError;
long Error, Start0, Start1, End0, End1;

	BestIndex[0] = BestIndex[1] = 0;
	BestError = 1 << 30;

	// Do the initial selection of end points based on colors in the block
	Count = Source.NumCodes();
	for(i=0; i<(Count-1); i++)
	{
		for(j=i+1; j<Count; j++)
		{
			BuildCodes8(pVects, Source[i], Source[j]);		// Build the interpolants
			Error = ComputeError8(Pixels);			// Compute the RMS error for Pixels

			if(Error < BestError)
			{
				BestError = Error;
				BestIndex[0] = i;
				BestIndex[1] = j;
			}
		}
	}

	Best[0] = Source[BestIndex[0]];
	Best[1] = Source[BestIndex[1]];
	Test[0] = Best[0];
	Test[1] = Best[1];

	// Now move the two end points around (do a brute force search) to see if we can
	// improve the quality of the match
	//for(c=1; c<4; c++)
	for (c = 0; c < 1; c++)
	{
		Test[0] = Best[0];
		Test[1] = Best[1];
		if(Test[0][c] == Test[1][c])
			continue;

		BuildCodes8(pVects, Test[0], Test[1]);				// Build the full set of interpolants

		Start0 = (long)Test[0][c] - ScanRange/2;
		End0 = Start0 + ScanRange;

		Start0 = __max(0, Start0);
		End0 = __min(255, End0);

		Start1 = (long)Test[1][c] - ScanRange/2;
		End1 = Start1 + ScanRange;

		Start1 = __max(0, Start1);
		End1 = __min(255, End1);

		for(i=Start0; i<=End0; i+=ScanStep)
		{
			Test[0][c] = (BYTE)i;
			for(j=Start1; j<=End1; j+=ScanStep)
			{
				Test[1][c] = (BYTE)j;
				BuildCodes8(pVects, /*c,*/ Test[0], Test[1]);	// Build the channel interpolants
				Error = ComputeError8(Pixels);		// Compute the RMS error for Pixels

				if(Error < BestError)
				{
					BestError = Error;
					Best[0][c] = (BYTE)i;
					Best[1][c] = (BYTE)j;
				}
			}
		}
	}

	BuildCodes8(pVects, Best[0], Best[1]);

	Dest[0] = pVects[0];
	Dest[1] = pVects[1];
	Dest[2] = pVects[2];
	Dest[3] = pVects[3];
	Dest[4] = pVects[4];
	Dest[5] = pVects[5];
	Dest[6] = pVects[6];
	Dest[7] = pVects[7];
	//__asm emms;
	return BestError;
}

// ----------------------------------------------------------------------------
// Map a 4x4 pixel block to a 6 interpolant DXT5 alpha block
//
// (in)  Source = unique colors in "Pixels"  (1 - 16 codes)
// (in)  Pixels = 4 x 4 pixels to be remapped
// (out) Dest = 8 output colors that best approximate the input (0, 1 are the endpoints)
// ----------------------------------------------------------------------------
long DXTCGen::Execute6(CodeBook &Source, CodeBook &Pixels, CodeBook &Dest)
{
long Count, i, j, c;
long BestIndex[2], BestError;
long Error, Start0, Start1, End0, End1;

	BestIndex[0] = BestIndex[1] = 0;
	BestError = 1 << 30;

	// Do the initial selection of end points based on colors in the block
	Count = Source.NumCodes();
	for(i=0; i<(Count-1); i++)
	{
		for(j=i+1; j<Count; j++)
		{
			BuildCodes6(pVects, Source[i], Source[j]);		// Build the interpolants
			Error = ComputeError8(Pixels);			// Compute the RMS error for Pixels

			if(Error < BestError)
			{
				BestError = Error;
				BestIndex[0] = i;
				BestIndex[1] = j;
			}
		}
	}

	Best[0] = Source[BestIndex[0]];
	Best[1] = Source[BestIndex[1]];
	Test[0] = Best[0];
	Test[1] = Best[1];

	// Now move the two end points around (do a brute force search) to see if we can
	// improve the quality of the match
	//for(c=1; c<4; c++)
	for (c = 0; c < 1; c++)
	{
		Test[0] = Best[0];
		Test[1] = Best[1];
		if(Test[0][c] == Test[1][c])
			continue;

		BuildCodes8(pVects, Test[0], Test[1]);				// Build the full set of interpolants

		Start0 = (long)Test[0][c] - ScanRange/2;
		End0 = Start0 + ScanRange;

		Start0 = __max(0, Start0);
		End0 = __min(255, End0);

		Start1 = (long)Test[1][c] - ScanRange/2;
		End1 = Start1 + ScanRange;

		Start1 = __max(0, Start1);
		End1 = __min(255, End1);

		for(i=Start0; i<=End0; i+=ScanStep)
		{
			Test[0][c] = (BYTE)i;
			for(j=Start1; j<=End1; j+=ScanStep)
			{
				Test[1][c] = (BYTE)j;
				BuildCodes6(pVects, /*c,*/ Test[0], Test[1]);	// Build the channel interpolants
				Error = ComputeError8(Pixels);		// Compute the RMS error for Pixels

				if(Error < BestError)
				{
					BestError = Error;
					Best[0][c] = (BYTE)i;
					Best[1][c] = (BYTE)j;
				}
			}
		}
	}

	BuildCodes6(pVects, Best[0], Best[1]);

	Dest[0] = pVects[0];
	Dest[1] = pVects[1];
	Dest[2] = pVects[2];
	Dest[3] = pVects[3];
	Dest[4] = pVects[4];
	Dest[5] = pVects[5];
	Dest[6] = pVects[6];
	Dest[7] = pVects[7];
	//__asm emms;
	return BestError;
}

} // end of namespace ImageLib
}
CS_PLUGIN_NAMESPACE_END(DDSImageIO)
