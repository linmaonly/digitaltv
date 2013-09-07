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

#include "bittype.h"
#include "bitstream.h"

BIT_RETURNTYPE CBitStream::Init(BIT_BYTE *pbBuffer, BIT_U32 nLength)
{
	if (!pbBuffer || nLength == 0)
		return BIT_ErrorBadParameter;
	m_BitStream.pbCurPointer = pbBuffer;
	m_BitStream.pbBufferEnd = pbBuffer + nLength;
	m_BitStream.nByteInUse = *m_BitStream.pbCurPointer;
	m_BitStream.nAvailableBits = 8;
	return BIT_OK;
}

BIT_U32 CBitStream::GetLengthInBit()
{
	return ::BSGetLengthInBit(&m_BitStream);
}

BIT_U32 CBitStream::ReadBits(BIT_U32 nBits)
{
	return ::BSReadBits(&m_BitStream, nBits);
}

BIT_RETURNTYPE CBitStream::SkipBits(BIT_U32 nBits)
{
	return ::BSSkipBits(&m_BitStream, nBits);
}

BIT_RETURNTYPE CBitStream::AlignByte()
{
	return ::BSAlignToNextByte(&m_BitStream);
}
