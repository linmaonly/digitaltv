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

#include "psiparser.h"
#include "string.h"

#include <stdio.h>

CShortSection::CShortSection()
{

}

CLongSection::CLongSection()
{

}

BIT_RETURNTYPE CShortSection::Load(BitStream *pBitStream)
{
	if (pBitStream == BIT_NULL)
		return BIT_ErrorBadParameter;
	table_id = BSReadBits(pBitStream, 8);
	section_syntax_indicator = BSReadBits(pBitStream, 1);
	BIT_U32 private_indicator = BSReadBits(pBitStream, 1);
	BIT_U32 reserved = BSReadBits(pBitStream, 2);
	section_length = BSReadBits(pBitStream, 12);
	return BIT_OK; 
}

BIT_BYTE* CLongSection::Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen)
{
	// Short Section header: 3 bytes, Long Section header: 3(short) + 5 = 8 bytes
	// CRC32: 4 bytes; The long section should have at least 8 + 4 = 12 bytes
	if (!pbBuffer || nBufferLen <= 12)
		return BIT_NULL;
	BitStream bitStream;
	bitStream.pbCurPointer = pbBuffer;
	bitStream.pbBufferEnd = pbBuffer + nBufferLen;
	bitStream.nByteInUse = *pbBuffer;
	bitStream.nAvailableBits = 8;
	BIT_RETURNTYPE ret = CShortSection::Load(&bitStream);
	if (ret != BIT_OK)
		return BIT_NULL;
	// Returns failure if this is not a long section
	if (section_syntax_indicator != 1)
		return BIT_NULL;
	if (section_length + 3 > nBufferLen || section_length < 9)
		return BIT_NULL;
	table_id_extension = BSReadBits(&bitStream, 16);
	BIT_U32 reserved = BSReadBits(&bitStream, 2);
	version_number = BSReadBits(&bitStream, 5);
	current_next_indicator = BSReadBits(&bitStream, 1);
	section_number = BSReadBits(&bitStream, 8);
	last_section_number = BSReadBits(&bitStream, 8);
	BIT_BYTE *pCRCByte = pbBuffer + 3 + section_length - 4;
	// TODO: check CRC here --lin
	return pbBuffer + 8; // The first byte that follows the last_section_number byte
}

CProgramAssociationSection::CProgramAssociationSection()
{
	program_number = 0;
	program_map_PID = 0;
};


CProgramAssociationSection::~CProgramAssociationSection()
{
	if (program_number)
		delete program_number;
	if (program_map_PID)
		delete program_map_PID;

};

BIT_RETURNTYPE CProgramAssociationSection::Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen)
{
	if (!pbBuffer || nBufferLen == 0)
		return BIT_ErrorBadParameter;
	BIT_BYTE *pbPatBuffer = CLongSection::Load(pbBuffer, nBufferLen);
	if (pbPatBuffer == BIT_NULL || pbPatBuffer >= pbBuffer + nBufferLen)
		return BIT_ErrorUndefined;
	// Check the table_id, it should be 0x00
	if (table_id != 0x00)
		return BIT_ErrorUndefined;	
	BIT_U32 nPATBufLen = nBufferLen - (pbPatBuffer - pbBuffer);
	if (nPATBufLen < 4)
		return BIT_ErrorUndefined;
	nProgramMapNum = (nPATBufLen - 4) / 4; // Exclude the 32-bit CRC 
	if (nProgramMapNum == 4)
		return BIT_ErrorUndefined;
	BitStream bitStream;
	bitStream.pbCurPointer = pbPatBuffer;
	bitStream.pbBufferEnd = pbPatBuffer + nPATBufLen;
	bitStream.nByteInUse = *pbPatBuffer;
	bitStream.nAvailableBits = 8;
	if (program_number)
		delete program_number;
	program_number = new BIT_U32[nProgramMapNum];
	if (program_number == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	if (program_map_PID)
		delete program_map_PID;
	program_map_PID = new BIT_U32[nProgramMapNum];
	memset(program_number, 0, sizeof(BIT_U32)*nProgramMapNum);
	if (program_map_PID == BIT_NULL)
	{
		delete program_number;
		program_number = BIT_NULL;
		return BIT_ErrorOutOfMemory;
	}
	memset(program_map_PID, 0, sizeof(BIT_U32)*nProgramMapNum);
	for (BIT_U32 n = 0; n < nProgramMapNum; ++n)
	{
		program_number[n] = BSReadBits(&bitStream, 16);
		BIT_U32 reserved = BSReadBits(&bitStream, 3);
		program_map_PID[n] = BSReadBits(&bitStream, 13);
	}
	return BIT_OK;
}

CProgramMapSection::CProgramMapSection()
{
	pIod_descriptor = BIT_NULL;
}

CProgramMapSection::~CProgramMapSection()
{
	delete pIod_descriptor;
}

BIT_RETURNTYPE CProgramMapSection::Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen)
{
	BIT_U32 reserved = 0;
	BIT_U32 program_info_length = 0;
	if (!pbBuffer || nBufferLen == 0)
		return BIT_ErrorBadParameter;
	BIT_BYTE *pbPatBuffer = CLongSection::Load(pbBuffer, nBufferLen);
	if (pbPatBuffer == BIT_NULL || pbPatBuffer >= pbBuffer + nBufferLen)
		return BIT_ErrorUndefined;
	// Check the table_id, it should be 0x02
	if (table_id != 0x02)
		return BIT_ErrorUndefined;	
	if (section_length + 3 > nBufferLen || section_length < 13)
		return BIT_ErrorUndefined;
	BitStream bitStream;
	BitStream tempBS; // Used for decoding sub included descriptors 
	bitStream.pbCurPointer = pbPatBuffer;
	bitStream.pbBufferEnd = pbPatBuffer + section_length - 5 - 4; // Excluding CRC bytes
	bitStream.nByteInUse = *pbPatBuffer;
	bitStream.nAvailableBits = 8;
	reserved = BSReadBits(&bitStream, 3);
	PCR_PID = BSReadBits(&bitStream, 13);
	reserved = BSReadBits(&bitStream, 4);
	program_info_length = BSReadBits(&bitStream, 12);
	if (program_info_length + 9 > BSGetLengthInBit(&bitStream)>>3)
		return BIT_ErrorUndefined;
	// It is very important to sync to next byte of the stream since we are going to pass it to sub descriptor,
	// otherwise the sub descriptor may get a bit stream starting with an corrupt byte
	BSAlignToNextByte(&bitStream); 
	// Check and descriptor type and decode it
	if (*bitStream.pbCurPointer == Mp2DescrTag_IOD_descriptor)
	{
		if (pIod_descriptor == BIT_NULL)
		{
			pIod_descriptor = new IOD_descriptor();
			if (pIod_descriptor == BIT_NULL)
				return BIT_ErrorOutOfMemory;
		}
		tempBS = bitStream;
		tempBS.pbBufferEnd = tempBS.pbCurPointer + program_info_length;
		pIod_descriptor->Load(&tempBS);
	}
	// Skip the descriptor() bits since we use a temporary bit stream structure
	BSSkipBits(&bitStream, program_info_length*8);
	BSAlignToNextByte(&bitStream);
	BIT_U32 nESInfoLen = section_length - 13 - program_info_length;
	if (nESInfoLen > BSGetLengthInBit(&bitStream)>>3)
		return BIT_ErrorUndefined;	
	// Parse the ES_info loop
	nActualESNum = 0;
	for (BIT_U32 n = 0; n < nESInfoLen;)
	{		
		pESInfo[nActualESNum].stream_type = (BIT_U8)BSReadBits(&bitStream, 8);
		reserved = BSReadBits(&bitStream, 3);
		pESInfo[nActualESNum].elementary_PID = (BIT_U16)BSReadBits(&bitStream, 13);
		//printf("stream_type:0x%x elementary_PID: 0x%x\r\n", pESInfo[nActualESNum].stream_type, pESInfo[nActualESNum].elementary_PID);
		reserved = BSReadBits(&bitStream, 4);
		BIT_U32 ES_info_length = BSReadBits(&bitStream, 12);
		if (ES_info_length > BSGetLengthInBit(&bitStream)>>3)
			return BIT_ErrorUndefined;
		BSAlignToNextByte(&bitStream);
				// Check and descriptor type and decode it
		if (*bitStream.pbCurPointer == Mp2DescrTag_SL_descriptor)
		{
			tempBS = bitStream;
			tempBS.pbBufferEnd = tempBS.pbCurPointer + ES_info_length;
			pESInfo[nActualESNum].sL_descriptor.descriptor_tag = (BIT_U8)BSReadBits(&tempBS, 8);
			pESInfo[nActualESNum].sL_descriptor.descriptor_length = (BIT_U8)BSReadBits(&tempBS, 8);
			pESInfo[nActualESNum].sL_descriptor.ES_ID = (BIT_U16)BSReadBits(&tempBS, 16);
		}
		// Skip the descriptor bits since we use a temporary bit stream when decoding them
		BSSkipBits(&bitStream, ES_info_length*8);
		n +=  5 + ES_info_length;
		if (nActualESNum++ >= MAX_ES_NUMBER_IN_PAT)
			break;
	}
	return BIT_OK;
}

ISO_IEC_14496_section::ISO_IEC_14496_section()
{
	pbSLOrFlexMuxBuffer = NULL;
	nSLOrFlexMuxBufferLen = 0;
}

ISO_IEC_14496_section::~ISO_IEC_14496_section()
{
}

BIT_RETURNTYPE ISO_IEC_14496_section::Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen, SLConfigDescriptor *pSLConfig)
{
	pbSLOrFlexMuxBuffer = CLongSection::Load(pbBuffer, nBufferLen);
	if (pbSLOrFlexMuxBuffer == BIT_NULL)
		return BIT_ErrorUndefined;
	nSLOrFlexMuxBufferLen = section_length - 5 - 4; // Excluding the following section bytes and CRC bytes
	SL_Packet slPacket;
	BIT_RETURNTYPE ret = slPacket.Load(pbSLOrFlexMuxBuffer, nSLOrFlexMuxBufferLen, pSLConfig);
	if (ret != BIT_OK)
		return ret;
	ret = objDescrStream.Load(pbSLOrFlexMuxBuffer, nSLOrFlexMuxBufferLen, pSLConfig);
	return ret;
}