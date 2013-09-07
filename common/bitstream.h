/*
* Copyright (c) 2006 BIT Everest, 
* Author: Lin Ma, 
* linmaonly@gmail.com
* htpp://biteverest.googlepages.com
* 
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use,
* copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following
* conditions:
* 
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __BIT_BITSTREAM_H__
#define __BIT_BITSTREAM_H__

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

//
// C style bit stream functions
//
typedef struct BitStream 
{
	BIT_BYTE *pbCurPointer;		// Always points to the current byte
	BIT_BYTE *pbBufferEnd;		// End of the buffer
	BIT_BYTE nByteInUse;		// Please set to the first byte value of the buffer in initialization
	BIT_U32 nAvailableBits;		// Please set to "8" in initialization
} BitStream;

static inline BIT_RETURNTYPE BSInit(BitStream *pStream, BIT_BYTE *pbBuffer, BIT_U32 nBufferSize);

static inline BIT_RETURNTYPE BSSkipBits(BitStream *pStream, BIT_U32 nBits);

static inline BIT_RETURNTYPE BSAlignToNextByte(BitStream *pStream);

/* Read bits from the bit stream, maximum 32 bits. This function 
will NOT check the parameters' validity due to performance consideration.
Please make sure you pass the correct handle (hBitStream, returned by BSCreate)
and nBits (must be <= 32) parameters
*/
static inline BIT_U64 BSReadBits(BitStream *pStream, BIT_U32 nBits);

static inline BIT_U32 BSGetLengthInBit(BitStream *pStream);

//
// C++ style 
//

class CBitStream
{
public:

	/* Initializes the bit stream. */
	BIT_RETURNTYPE Init(BIT_BYTE *pbBuffer, BIT_U32 nLength);

	BIT_RETURNTYPE SkipBits(BIT_U32 nBits);

	BIT_RETURNTYPE AlignByte(void);

	/* Read bits from the bit stream, maximum 32 bits. This function 
	will NOT check the parameters' validity due to performance consideration.
	Please make sure you pass the correct handle (hBitStream, returned by BSCreate)
	and nBits (must be <= 32) parameters
	*/
	BIT_U32 ReadBits(BIT_U32 nBits);

	BIT_U32 GetLengthInBit(void);

protected:
	BitStream m_BitStream;
};


//
// Implementations (static inline functions)
//
static BIT_U8 g_Mask[] =
{
	0x0,
	0x80,
	0xc0,
	0xe0,
	0xf0,
	0xf8,
	0xfc,
	0xfe,
	0xff,
};


static inline BIT_U64 BSReadBits(BitStream *pStream, BIT_U32 nBits)
{
	BIT_U64 nReturn = 0;
	if (nBits == 0)
		return 0;
	while (nBits > 0)
	{
		if (pStream->nAvailableBits == 0)
		{
			pStream->nByteInUse = *(++pStream->pbCurPointer);
			pStream->nAvailableBits = 8;
		}
		if (nBits <= pStream->nAvailableBits)
		{
			nReturn |= (pStream->nByteInUse&g_Mask[nBits]) >> (8 - nBits);
			pStream->nAvailableBits -= nBits;
			pStream->nByteInUse = pStream->nByteInUse << nBits;
			return nReturn;
		}
		// nBits is bigger than pStream->nAvailableBits
		BIT_U32 nRead = pStream->nAvailableBits;
		nReturn |= (BIT_U32)((pStream->nByteInUse&g_Mask[nRead])>>(8-nRead)) << (nBits - nRead);
		nBits -= nRead;
		pStream->nAvailableBits = 0;
	}
	return nReturn;
}

static inline BIT_U32 BSGetLengthInBit(BitStream *pStream)
{
	if (pStream->pbCurPointer >= pStream->pbBufferEnd)
		return 0;
	BIT_S32 n = (BIT_S32)(pStream->pbBufferEnd - pStream->pbCurPointer) - 1;
	BIT_S32 nLengthInBit = pStream->nAvailableBits + n * 8;
	if (nLengthInBit < 0)
		nLengthInBit = 0;
	return nLengthInBit;
}

static inline BIT_RETURNTYPE BSInit(BitStream *pStream, BIT_BYTE *pbBuffer, BIT_U32 nBufferSize)
{
	if (!pStream || !pbBuffer || nBufferSize == 0)
		return BIT_ErrorBadParameter;
	pStream->pbCurPointer = pbBuffer;
	pStream->pbBufferEnd = pbBuffer + nBufferSize;
	pStream->nByteInUse = *pbBuffer;
	pStream->nAvailableBits = 8;
	return BIT_OK;
}

static inline BIT_RETURNTYPE BSSkipBits(BitStream *pStream, BIT_U32 nBits)
{
	while (nBits > 0)
	{
		if (pStream->nAvailableBits == 0)
		{
			pStream->nByteInUse = *(++pStream->pbCurPointer);
			pStream->nAvailableBits = 8;
		}
		if (nBits <= pStream->nAvailableBits)
		{
			pStream->nAvailableBits -= nBits;
			pStream->nByteInUse = pStream->nByteInUse << nBits;
			return BIT_OK;
		}
		// nBits is bigger than nAvailableBits
		BIT_U32 nRead = pStream->nAvailableBits;
		nBits -= nRead;
		pStream->nAvailableBits = 0;
	}
	return BIT_OK;
}

static inline BIT_RETURNTYPE BSAlignToNextByte(BitStream *pStream)
{
	if (pStream->nAvailableBits < 8)
	{
		pStream->nByteInUse = *(++pStream->pbCurPointer);
		pStream->nAvailableBits = 8;
	}
	return BIT_OK;
}



#endif /* __BIT_BITSTREAM_H__ */