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

#ifndef __BIT_TSDEMUXIMPL_H__
#define __BIT_TSDEMUXIMPL_H__

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


/**
 Sync byte 8-bit 0x47 
 Transport Error Indicator (TEI) 1-bit Set by demodulator if can't correct errors in the stream[4] 
 Payload unit start 1-bit 1 means start of PES data or PSI otherwise zero only . 
 Priority 1-bit One means higher priority, if more than one packets having same PID value. 
 PID 13-bit Packet ID 
 Scrambling control 2-bit '00' = Not scrambled.   The following per DVB spec [2]:   '01' = Reserved for future use,   '10' = Scrambled with even key,   '11' = Scrambled with odd key 
 Adaptation field exist 1-bit 1 means presence of the adaptation field 
 Payload data exist 1-bit 1 means presence of data 
 Continuity counter 4-bit  
 Note: the total number of bits above is 32 and is called the transport stream 4-byte prefix. 
 
 Adaption field 0 or more Depends on flags 
 Payload Data 0 or more Depends on flags 
 **/

#define MP2TS_SYNC_BYTE 0x47

#define MAX_PID_VALUE 8192
#define MAX_SECTION_LENGTH 4096 // Includes the first 3 bytes

typedef struct TsSection 
{
	BIT_BYTE pbData[MAX_SECTION_LENGTH]; 
	BIT_U32 nSectionLength; // Includes the 3 bytes for table_id and section length 
	BIT_U32 nCurrentLength;
	BIT_BYTE pbTsPktHead[4]; // The head of the first TS packet of this section
} TsSection;

typedef struct TsPES
{
	BIT_U32 PES_packet_length;
} TsPES;

typedef struct TsListener
{
	PFNLISTENER pfnCallback;
	BIT_PTR pvContext; // the context passed to the user every time we call pfnCallback
	BIT_U32 continuity_counter; // continuity_counter of last packet
	TsDmxDataType eDataType;
	BIT_BOOL bStartIndicatorFound;
	TsPES* pPES;
	TsSection* pSection; // Set to null if this is not a section listener
} TsListener;

class CTsDmx
{
public:

	BIT_RETURNTYPE Open(TsDmxOpenParameters *pOpenParameters);

	BIT_RETURNTYPE Reset(void);

	BIT_RETURNTYPE ListenPID(TsDmxListenParameters *pListenPara);

	BIT_RETURNTYPE RemovePID(BIT_U32 dwCookie);

	BIT_RETURNTYPE Process(BIT_PTR pData, BIT_U32 nDataLen);

	BIT_RETURNTYPE Close(void);

protected:
	BIT_RETURNTYPE ProcessPacket(BIT_BYTE *pbPacket);
	inline BIT_RETURNTYPE ProcessSection(TsListener *pListener, BIT_BYTE *pbData, BIT_U32 nDataLen, BIT_BYTE *pbPktHead);

protected:
	BIT_U32 m_nPacketLen;
	BIT_U32 m_nPktHeadLen;

	/* In case of segment (the process function is called with a piece of
	   packet). Store the payload in the temporary buffer 
	 */
	BIT_BYTE m_pbTmpPacket[208];
	BIT_U32 m_nLastFragmentLen;

	/* 13-bit PID, maximum value is 8192 */
	TsListener *m_pListener[MAX_PID_VALUE];
	BIT_U32 m_nListenerFlags[MAX_PID_VALUE];
};

#endif /* __BIT_TSDEMUXIMPL_H__ */