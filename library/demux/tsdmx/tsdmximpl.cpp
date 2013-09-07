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

#include "tsdmx.h"
#include "tsdmximpl.h"
#include "bitstream.h"

/* Platform dependent header files */
#include <string.h>

#include <stdio.h>
#include <Windows.h>
#define DMXDP printf

BIT_RETURNTYPE CTsDmx::Open(TsDmxOpenParameters *pOpenParameters)
{
	for (BIT_U32 n = 0; n < sizeof(m_pListener) / sizeof(m_pListener[0]); ++n)
		m_pListener[n] = 0;
	memset(m_pListener, 0, sizeof(m_pListener));
	m_nPacketLen = pOpenParameters->nPacketLen;
	m_nPktHeadLen = pOpenParameters->nHeadLen;
	if (m_nPacketLen < 188)
		return BIT_ErrorBadParameter;
	/* Only 4 bytes prefix is supported */
	if (m_nPktHeadLen != 4)
		return BIT_ErrorBadParameter;
	m_nLastFragmentLen = 0;
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::Close()
{
	// Safely delete all listeners
	// Actually it is safe to delete a null pointer
	for (BIT_U32 n = 0; n < sizeof(m_pListener) / sizeof(m_pListener[0]); n++)
	{
		if (m_pListener[n])
		{
			if (m_pListener[n]->pSection)
				delete m_pListener[n]->pSection;
			if (m_pListener[n]->pPES)
				delete m_pListener[n]->pPES;
			delete m_pListener[n];
			m_pListener[n] = 0;
		}
	}
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::ListenPID(TsDmxListenParameters *pListenPara)
{
	if (!pListenPara)
		return BIT_ErrorBadParameter;
	BIT_U32 nPID = pListenPara->nPID;
	if (nPID >= 8192 || !pListenPara->pfnCallback)
		return BIT_ErrorBadParameter;
	// Not allowed to listen the same PID twice.
	if (m_pListener[nPID])
		return BIT_ErrorUndefined;
	TsListener *pInternalListener = new TsListener();
	memset(pInternalListener, 0, sizeof(TsListener));
	pInternalListener->pvContext = pListenPara->pvContext;
	pInternalListener->pfnCallback = pListenPara->pfnCallback;
	pInternalListener->continuity_counter = 0;
	pInternalListener->pSection = 0; // Default value
	pInternalListener->eDataType = pListenPara->eDataType;
	if (pInternalListener->eDataType == TSDMX_DATA_SECTION)
	{
		TsSection *pSection = new TsSection();
		if (!pSection)
			return BIT_ErrorOutOfMemory;
		// VERY IMPORTANT! Set the initial values for this structure
		memset(pSection, 0, sizeof(TsSection));
		pInternalListener->pSection = pSection;
	}
	else if (pInternalListener->eDataType == TSDMX_DATA_PES)
	{
		TsPES *pPES = new TsPES();
		if (!pPES)
			return BIT_ErrorOutOfMemory;
		memset(pPES, 0, sizeof(TsPES));
		pInternalListener->pPES = pPES;

	}
	pInternalListener->bStartIndicatorFound = BIT_FALSE;
	// Save the listener
	m_pListener[nPID] = pInternalListener;
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::Reset()
{
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::RemovePID(BIT_U32 nPID)
{
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::Process(BIT_PTR pData, BIT_U32 nDataLen)
{
	if (!pData || nDataLen == 0)
		return BIT_ErrorBadParameter;
	BIT_BYTE *pbPacket = reinterpret_cast<BIT_BYTE*>(pData);
	BIT_BYTE *pbPacketEnd = pbPacket + nDataLen;
	/* If the very last packet is not complete
	We assume that the beginning of this pData belongs to last packet
	*/
	if (m_nLastFragmentLen)
	{
		BIT_U32 nBytes = m_nPacketLen - m_nLastFragmentLen;
		if (nBytes > nDataLen)
			nBytes = nDataLen;
		memcpy(m_pbTmpPacket + m_nLastFragmentLen, pbPacket, nBytes);
		m_nLastFragmentLen += nBytes;
		pbPacket += nBytes;
		/* still need to wait for more data? */
		if (m_nLastFragmentLen != m_nPacketLen)
			return BIT_OK;
		m_nLastFragmentLen = 0;
		ProcessPacket(m_pbTmpPacket);
	}

	while (pbPacket < pbPacketEnd)
	{
		if (*pbPacket != MP2TS_SYNC_BYTE)
		{
			++pbPacket;
			continue;
		}
		BIT_U32 nBytesLeft = (BIT_U32)(pbPacketEnd - pbPacket);
		/* Not a complete packet? Store data in segment buffer and return*/
		if (nBytesLeft < m_nPacketLen)
		{
			memcpy(m_pbTmpPacket, pbPacket, nBytesLeft);
			m_nLastFragmentLen = nBytesLeft;
			return BIT_OK;
		}
		ProcessPacket(pbPacket);
		pbPacket += m_nPacketLen;
	}
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::ProcessPacket(BIT_BYTE *pbPacket)
{
	BIT_U32 nPayloadOffset = m_nPktHeadLen;
	BIT_U32 nPID = TSPKT_PID(pbPacket);
	TsListener *pListener = NULL;	
	//DMXDP("PID: 0x%x\r\n", nPID);
	pListener = m_pListener[nPID];
	// return if nobody is interested in or no payload exists
	if (!pListener)
		return BIT_OK;
	/* Check Transport Error Indicator (TEI) bit */
	if (TSPKT_TEI(pbPacket) == 0x01)
		return BIT_ErrorUndefined;
	BIT_U32 nPktFlags = (pbPacket[3]&0x30);  // Set PAYLOAD_EXIST and ADAPTATION_EXIST flag (bit 4 and 5)
	nPktFlags |= (pbPacket[1]&0xe0)<<4;		// Set TRANSPORT_PRIORITY, PAYLOAD_UNIT_START_INDICATOR and TRANSPORT_ERROR_INDICATOR flags
	// Check continuity_counter
	BIT_U32 continuity_counter = TSPKT_COUNTER(pbPacket);
	if (continuity_counter != ((pListener->continuity_counter + 1)&0xf))
	{
		nPktFlags |= PKTFLAG_DISCONTINUITY;
		//pListener->bStartIndicatorFound = BIT_FALSE;
		printf("PID:%d, packet lost\r\n", TSPKT_PID(pbPacket));
	}
	pListener->continuity_counter = continuity_counter;
	if (!(nPktFlags&PKTFLAG_PAYLOAD_EXIST))
		return BIT_OK;
	if (nPktFlags&PKTFLAG_ADAPTATION_EXIST)
	{
		BIT_BYTE *pbAdptField = pbPacket + m_nPktHeadLen;
		nPayloadOffset += 1 + pbAdptField[0];
		// TODO: parse adaptation layer here --lin
	}
	BIT_BYTE* pbPayload = pbPacket + nPayloadOffset;
	BIT_U32 nBytes = m_nPacketLen - nPayloadOffset;
	// if this is section data
	if (pListener->eDataType == TSDMX_DATA_SECTION)
	{
		TsSection *pSection = pListener->pSection;
		if (nPktFlags&PKTFLAG_DISCONTINUITY)
		{
			pSection->nSectionLength = 0;
			pSection->nCurrentLength = 0;
			pListener->bStartIndicatorFound = BIT_FALSE;
		}
		if (nPktFlags&PKTFLAG_PAYLOAD_UNIT_START_INDICATOR)
		{
			BIT_U32 pointer_field = pbPayload[0];
			// Check if the pointer_field is corrupt
			if (pointer_field > m_nPacketLen - nPayloadOffset)
				return BIT_ErrorUndefined;
			pbPayload += 1; // Skip the pointer_field byte
			// if a new section starts but the last is not complete
			// we just simply throw the last section
			if (pSection->nCurrentLength + pointer_field < pSection->nSectionLength)
			{
				pSection->nSectionLength = 0;
				pSection->nCurrentLength = 0;
				pListener->bStartIndicatorFound = BIT_FALSE;
			}
			// If the previous section is not finished
			if (pListener->bStartIndicatorFound && pSection->nSectionLength > 0 && pointer_field > 0)
				ProcessSection(pListener, pbPayload, pointer_field, pbPacket);
			// Starts parsing the new section
			pbPayload += pointer_field;
			nBytes -= (pointer_field + 1);
			// Now we must throw the data in pSection->pbData because of payload_unit_start_indicator
			pSection->nSectionLength = 0;
			pSection->nCurrentLength = 0;
			pListener->bStartIndicatorFound = BIT_TRUE;
		}
		if (pListener->bStartIndicatorFound && nBytes > 0)
			ProcessSection(pListener, pbPayload, nBytes, pbPacket);
	}
	if (pListener->eDataType == TSDMX_DATA_PES)
	{
		// Process PES
		TsPES *pPES = pListener->pPES;
		BIT_U32 tentativePts = 0;
		BIT_U32 tentativeDts = 0;
		if (nPktFlags&PKTFLAG_PAYLOAD_UNIT_START_INDICATOR)
		{
			CBitStream bitStream;
			bitStream.Init(pbPayload, nBytes);
			BIT_U32 packet_start_code_prefix = bitStream.ReadBits(24);
			BIT_U32 stream_id = bitStream.ReadBits(8);
			BIT_U32 PES_packet_length = bitStream.ReadBits(16);
			BIT_U32 bitValue =  bitStream.ReadBits(2);
			if (bitValue != 0x02) // The two-bit field should be '10b' in MPEG-2 transport stream
				return BIT_OK;
			pListener->bStartIndicatorFound = BIT_TRUE;
			BIT_U32 PES_scrambling_control =  bitStream.ReadBits(2);
			BIT_U32 PES_priority =  bitStream.ReadBits(1);
			BIT_U32 data_alignment_indicator =  bitStream.ReadBits(1);
			BIT_U32 copyright =  bitStream.ReadBits(1);
			BIT_U32 original_or_copy =  bitStream.ReadBits(1);
			BIT_U32 PTS_DTS_flags =  bitStream.ReadBits(2);
			BIT_U32 ESCR_flag =  bitStream.ReadBits(1);
			BIT_U32 ES_rate_flag =  bitStream.ReadBits(1);
			BIT_U32 DSM_trick_mode_flag =  bitStream.ReadBits(1);
			BIT_U32 additional_copy_info_flag =  bitStream.ReadBits(1);
			BIT_U32 PES_CRC_flag =  bitStream.ReadBits(1);
			BIT_U32 PES_extension_flag =  bitStream.ReadBits(1);
			BIT_U32 PES_header_data_length =  bitStream.ReadBits(8);
			if (PES_header_data_length + 4 + 9 > m_nPacketLen)
				return BIT_OK;
			BIT_U32 marker_bit = 0;
			if (PTS_DTS_flags == 0x02)
			{
				bitValue = bitStream.ReadBits(4);
				tentativePts = bitStream.ReadBits(3) << 30;
				marker_bit = bitStream.ReadBits(1);
				tentativePts |= bitStream.ReadBits(15) << 15;
				marker_bit = marker_bit & bitStream.ReadBits(1);
				tentativePts |= bitStream.ReadBits(15);
				marker_bit = marker_bit & bitStream.ReadBits(1);
				if (bitValue == 0x2 && marker_bit)
					nPktFlags |= PKTFLAG_PTS_PRESENT;
			}
			if (PTS_DTS_flags == 0x03)
			{
				// Parse PTS information
				bitValue = bitStream.ReadBits(4);
				tentativePts = bitStream.ReadBits(3) << 30;
				marker_bit = bitStream.ReadBits(1);
				tentativePts |= bitStream.ReadBits(15) << 15;
				marker_bit = marker_bit & bitStream.ReadBits(1);
				tentativePts |= bitStream.ReadBits(15);
				marker_bit = marker_bit & bitStream.ReadBits(1);
				if (bitValue == 0x03 && marker_bit)
					nPktFlags |= PKTFLAG_PTS_PRESENT;
				// Parse DTS information
				bitValue = bitStream.ReadBits(4);
				tentativePts = bitStream.ReadBits(3) << 30;
				marker_bit = bitStream.ReadBits(1);
				tentativePts |= bitStream.ReadBits(15) << 15;
				marker_bit = marker_bit & bitStream.ReadBits(1);
				tentativePts |= bitStream.ReadBits(15);
				marker_bit = marker_bit & bitStream.ReadBits(1);
				if (bitValue == 0x03 && marker_bit)
					nPktFlags |= PKTFLAG_DTS_PRESENT;
			}
			// Skip the header bytes
			pbPayload += 9 + PES_header_data_length;
			nBytes -= 9 + PES_header_data_length;
		}
		if (pListener->bStartIndicatorFound)
			pListener->pfnCallback(pListener->pvContext, nPID, pbPayload, nBytes, pbPacket, nPktFlags, tentativePts, tentativeDts);
	}
	return BIT_OK;
}

BIT_RETURNTYPE CTsDmx::ProcessSection(TsListener *pListener, BIT_BYTE *pbData, BIT_U32 nDataLen, BIT_BYTE *pbPktHead)
{
	BIT_BYTE *pbDataEnd = pbData + nDataLen;
	TsSection *pSection = pListener->pSection;
	BIT_U32 nBytes = 0;
	if (pSection->nSectionLength)
	{
		nBytes = pSection->nSectionLength - pSection->nCurrentLength;
		if (nBytes > nDataLen)
			nBytes = nDataLen;
		memcpy(pSection->pbData+pSection->nCurrentLength, pbData, nBytes);
		pbData += nBytes;
		nDataLen -= nBytes;
		pSection->nCurrentLength += nBytes;
		if (pSection->nCurrentLength == pSection->nSectionLength)
		{
			pListener->pfnCallback(pListener->pvContext, TSPKT_PID(pbPktHead), pSection->pbData,
				pSection->nSectionLength, pSection->pbTsPktHead, 0, 0, 0);
			pSection->nSectionLength = 0;
			pSection->nCurrentLength = 0;
		}					
	}
	/*	Section header
		table_id												8 uimsbf
		section_syntax_indicator								1 bslbf
		'0'														1 bslbf
		reserved												2 bslbf
		section_length											12 uimsbf
	 */
	if (nDataLen > 0 && nDataLen <= 3)
	{
		printf("Error!!!! Modify the demux to fill a complete section before parsing the Section fragment...**********\r\n");
		// TODO: Handle section fragment!!!!!!!!!! e.g. an new section starts its first byte only in a TS packet
	}
	while (pbData < pbDataEnd)
	{
		// There is no need to check the section_syntax_indicator, '0' means short (private) section
		BIT_U32 section_length = ((BIT_U32)(pbData[1]&0xf) << 8) | pbData[2];
		// Check if we meet the stuffing (0xff, 0xff ...)
		if (section_length > MAX_SECTION_LENGTH || pbData[0] == 0xff)
			return BIT_OK;
		pSection->nSectionLength = section_length + 3;
		// Do NOT skip the table_id, section_syntax_indicator, '0', reserved, and section_length bits, 3 bytes in 
		// total. The user needs them
		nBytes = pSection->nSectionLength;
		if (nBytes > nDataLen)
			nBytes = nDataLen;
		memcpy(pSection->pbTsPktHead, pbPktHead, m_nPktHeadLen);
		memcpy(pSection->pbData, pbData, nBytes);
		pbData += nBytes;
		nDataLen -= nBytes;
		pSection->nCurrentLength += nBytes;
		if (pSection->nCurrentLength == pSection->nSectionLength)
		{
			pListener->pfnCallback(pListener->pvContext, TSPKT_PID(pbPktHead), pSection->pbData,
				pSection->nSectionLength, pSection->pbTsPktHead, 0, 0, 0);
			pSection->nSectionLength = 0;
			pSection->nCurrentLength = 0;
		}					
	}
	return BIT_OK;
}