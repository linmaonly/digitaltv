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

#ifndef __BIT_PSIPARSER_H__
#define __BIT_PSIPARSER_H__

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


#include "bittype.h"
#include "bitstream.h"
#include "descriptor.h"

/*
	private_section() {
	table_id 8 uimsbf
	section_syntax_indicator 1 bslbf
	private_indicator 1 bslbf
	reserved 2 bslbf
	private_section_length 12 uimsbf
	if (section_syntax_indicator = = '0') {
	for (i = 0; i < N; i++) {
	private_data_byte 8 bslbf
	}
	}
	else {
	table_id_extension 16 uimsbf
	reserved 2 bslbf
	version_number 5 uimsbf
	current_next_indicator 1 bslbf
	section_number 8 uimsbf
	last_section_number 8 uimsbf
	for (i = 0; i < private_section_length-9; i++) {
	private_data_byte 8 bslbf
	}
	CRC_32 32 rpchof
	}
	}
 */

class CShortSection
{
public:
	BIT_U32 table_id;
	BIT_U32 section_length;

protected:
	BIT_U32 section_syntax_indicator;

protected:
	CShortSection();
	BIT_RETURNTYPE Load(BitStream *pBitStream);
};

class CLongSection : public CShortSection
{
public:
	BIT_U32 table_id_extension;
	BIT_U32 version_number;
	BIT_U32 current_next_indicator;
	BIT_U32 section_number;
	BIT_U32 last_section_number;

protected:
	CLongSection();
	/* Returns the first byte of payload (after the last_section_number byte)
	   Returning null indicates failed
	 */
	BIT_BYTE* Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen);
};

class CProgramAssociationSection : public CLongSection
{
public:
	CProgramAssociationSection();
	~CProgramAssociationSection();
	BIT_RETURNTYPE Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen);

public:
	BIT_U32 *program_number; // The length of this array depends on program_count
	BIT_U32 *program_map_PID; // or network_PID when program_number[n] equals to 0
	BIT_U32 nProgramMapNum;
};

#define MAX_ES_NUMBER_IN_PAT (0x0f) // Maximum 15 strems, discard if the total number exceeds 15

typedef struct ESInfo
{
	BIT_U8 stream_type;
	BIT_U16 elementary_PID;
	SL_descriptor sL_descriptor; 
} ESInfo;

class CProgramMapSection : public CLongSection
{
public:
	CProgramMapSection();
	~CProgramMapSection();
	BIT_RETURNTYPE Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen);

public:
	BIT_U32 PCR_PID;
	BIT_U32 nElementaryStreamCount;
	ESInfo pESInfo[MAX_ES_NUMBER_IN_PAT];
	BIT_U32 nActualESNum;
	IOD_descriptor *pIod_descriptor;
};

class ISO_IEC_14496_section : public CLongSection
{
public:
	ISO_IEC_14496_section();
	~ISO_IEC_14496_section();
	BIT_RETURNTYPE Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferLen, SLConfigDescriptor *pSLConfig);

public:
	BIT_BYTE *pbSLOrFlexMuxBuffer;
	BIT_U32 nSLOrFlexMuxBufferLen;
	ObjectDescriptorStream objDescrStream;
};

#endif /* __BIT_PSIPARSER_H__ */
