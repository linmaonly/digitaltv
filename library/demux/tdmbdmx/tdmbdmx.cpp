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

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


#include <windows.h>
#include <stdio.h>

#include "tsdmx.h"
#include "psiparser.h"


static BIT_HANDLE hTsDmx = NULL;
static BIT_U32 g_nPMTPID = (BIT_U32)-1;
static BIT_U32 g_nODStreamPID = (BIT_U32)-1;
static BIT_U32 g_nAudioStreamPID = (BIT_U32)-1;
static BIT_U32 g_nVideoStreamPID = (BIT_U32)-1;

CProgramMapSection g_ProgramMapTable;
ISO_IEC_14496_section g_ODStreamSection;

static SLConfigDescriptor *g_pODStreamSLCfg = BIT_NULL;
static SLConfigDescriptor *g_pAudioSLConfig = BIT_NULL;
static SLConfigDescriptor *g_pVisualSLConfig = BIT_NULL;
static AVCDecoderConfigurationRecord *g_pAvcCfgRecord = BIT_NULL;

static BIT_BYTE g_pPmtSectionBuffer[1024*10];
static BIT_BYTE g_pODStreamSectionBuffer[1024*10];

BIT_U32 TdmbVideoListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pData, BIT_U32 nDataSize, BIT_BYTE* pHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts);
BIT_U32 TdmbAudioListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pData, BIT_U32 nDataSize, BIT_BYTE* pHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts);

BIT_U32 TdmbSectionListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pData, BIT_U32 nDataSize, BIT_BYTE* pHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts)
{
	if (TSPKT_PID(pHead) == 0x00 && g_nPMTPID == (BIT_U32)-1)
	{
		CProgramAssociationSection pat;
		if (pat.Load((BIT_BYTE*)pData, nDataSize) == BIT_OK)
		{
			if (pat.nProgramMapNum && pat.program_map_PID != NULL)
			{
				TsDmxListenParameters listenPara;
				memset(&listenPara, 0, sizeof(listenPara));
				listenPara.nStructSize = sizeof(listenPara);
				listenPara.eDataType = TSDMX_DATA_SECTION;
				listenPara.nPID = pat.program_map_PID[0];
				listenPara.pfnCallback = TdmbSectionListener;
				listenPara.pvContext = pvContext;
				TsDmxListenPID(hTsDmx, &listenPara);
				g_nPMTPID = pat.program_map_PID[0];
			}
		}
		return BIT_OK;
	}
	if (TSPKT_PID(pHead) == g_nPMTPID && g_nODStreamPID == (BIT_U32)-1)
	{
		// Check TDMB SceneDescriptionStream PID
		BIT_U32 nSceneDescrESID = 0;
		BIT_U32 nSceneDescrPID = 0;
		BIT_U32 nObjDescrESID = 0;
		BIT_U32 nObjDescrPID = 0;
		memcpy(g_pPmtSectionBuffer, pData, nDataSize);
		if (g_ProgramMapTable.Load((BIT_BYTE*)g_pPmtSectionBuffer, nDataSize) == BIT_OK)
		{
			if (g_ProgramMapTable.pIod_descriptor && g_ProgramMapTable.pIod_descriptor->pInitialObjectDescriptor)
			{
				InitialObjectDescriptor *pInitOD = g_ProgramMapTable.pIod_descriptor->pInitialObjectDescriptor;
				for (BIT_U32 n = 0; n < pInitOD->nEsDescrCount; ++n)
				{
					if (!pInitOD->pEsDescr[n].pDecConfigDescr)
						continue;
					// Skip the stream if is depends on another stream
					if (pInitOD->pEsDescr[n].streamDependenceFlag && pInitOD->pEsDescr[n].dependsOn_ES_ID)
						continue;
					// Check if it is a ObjectDescriptorStream 
					if (pInitOD->pEsDescr[n].pDecConfigDescr->streamType == Mp4StreamType_ObjectDescriptorStream)
					{
						g_pODStreamSLCfg = pInitOD->pEsDescr[n].pSlConfigDescr;
						nObjDescrESID = pInitOD->pEsDescr[n].ES_ID;
						for (BIT_U32 nEsIdx = 0; nEsIdx < g_ProgramMapTable.nActualESNum; ++nEsIdx)
						{
							if (g_ProgramMapTable.pESInfo[nEsIdx].stream_type == Mp2StreamType_SyncLayerInSection &&
								g_ProgramMapTable.pESInfo[nEsIdx].sL_descriptor.ES_ID == nObjDescrESID)
							{
								g_nODStreamPID = g_ProgramMapTable.pESInfo[nEsIdx].elementary_PID; 
							}
						}

					}
				}
			}
			if (g_nODStreamPID != (BIT_U32)-1)
			{
				TsDmxListenParameters listenPara;
				memset(&listenPara, 0, sizeof(listenPara));
				listenPara.nStructSize = sizeof(listenPara);
				listenPara.eDataType = TSDMX_DATA_SECTION;
				listenPara.nPID = g_nODStreamPID;
				listenPara.pfnCallback = TdmbSectionListener;
				listenPara.pvContext = pvContext;
				TsDmxListenPID(hTsDmx, &listenPara);
			}
		}
	}
	if (TSPKT_PID(pHead) == g_nODStreamPID && g_pODStreamSLCfg && g_nVideoStreamPID == (BIT_U32)-1 && g_nAudioStreamPID == (BIT_U32)-1)
	{
		BIT_U32 nVideoStreamESID = 0;
		// All descriptors reference the buffer pointers thus we must save the section buffer
		memcpy(g_pODStreamSectionBuffer, pData, nDataSize);
		if (g_ODStreamSection.Load(g_pODStreamSectionBuffer, nDataSize, g_pODStreamSLCfg) == BIT_OK)
		{
			// Check all ObjectDescriptor
			for (BIT_U32 nObjDescrIdx = 0; nObjDescrIdx < g_ODStreamSection.objDescrStream.nActualObjDescrNum; ++nObjDescrIdx)
			{
				ObjectDescriptor *pObjDescr = &(g_ODStreamSection.objDescrStream.pObjectDescriptor[nObjDescrIdx]);
				// Check all ES_Descriptor in this ObjectDescriptor
				for (BIT_U32 n = 0; pObjDescr, n < pObjDescr->nEsDescrCount; ++n)
				{
					if (!pObjDescr->pEsDescr[n].pDecConfigDescr)
						continue;
					// Check if it is a Audio Stream 
					if (pObjDescr->pEsDescr[n].pDecConfigDescr->streamType == Mp4StreamType_AudioStream)
					{
						BIT_U32 nAudioStreamESID = pObjDescr->pEsDescr[n].ES_ID;
						g_pAudioSLConfig = pObjDescr->pEsDescr[n].pSlConfigDescr;
						for (BIT_U32 nEsIdx = 0; nEsIdx < g_ProgramMapTable.nActualESNum; ++nEsIdx)
						{
							// The stream_type must be Mp2StreamType_SyncLayerInPES for a Audio Stream
							if (g_ProgramMapTable.pESInfo[nEsIdx].stream_type == Mp2StreamType_SyncLayerInPES &&
								g_ProgramMapTable.pESInfo[nEsIdx].sL_descriptor.ES_ID == nAudioStreamESID)
							{
								g_nAudioStreamPID = g_ProgramMapTable.pESInfo[nEsIdx].elementary_PID; 
							}	

						}
					}
					// Check if it is a Visual Stream 
					if (pObjDescr->pEsDescr[n].pDecConfigDescr->streamType == Mp4StreamType_VisualStream)
					{
						BIT_U32 nVideoStreamESID = pObjDescr->pEsDescr[n].ES_ID;
						g_pVisualSLConfig = pObjDescr->pEsDescr[n].pSlConfigDescr;
						g_pAvcCfgRecord = &(pObjDescr->pEsDescr[n].pDecConfigDescr->aVCDecoderConfigurationRecord);
						for (BIT_U32 nEsIdx = 0; nEsIdx < g_ProgramMapTable.nActualESNum; ++nEsIdx)
						{
							// The stream_type must be Mp2StreamType_SyncLayerInPES for a Visual Stream
							if (g_ProgramMapTable.pESInfo[nEsIdx].stream_type == Mp2StreamType_SyncLayerInPES &&
								g_ProgramMapTable.pESInfo[nEsIdx].sL_descriptor.ES_ID == nVideoStreamESID)
							{
								g_nVideoStreamPID = g_ProgramMapTable.pESInfo[nEsIdx].elementary_PID; 
							}	

						}
					}
				}
			}
			// If we get audio and video PIDs, listen them
			if (g_nAudioStreamPID != (BIT_U32)-1 && g_nVideoStreamPID != (BIT_U32)-1)
			{
				TsDmxListenParameters listenPara;
				memset(&listenPara, 0, sizeof(listenPara));
				// Listen the audio stream
				listenPara.nStructSize = sizeof(listenPara);
				listenPara.eDataType = TSDMX_DATA_PES;
				listenPara.nPID = g_nAudioStreamPID;
				listenPara.pfnCallback = TdmbAudioListener;
				listenPara.pvContext = pvContext;
				TsDmxListenPID(hTsDmx, &listenPara);
				// Listen the video stream
				memset(&listenPara, 0, sizeof(listenPara));
				listenPara.nStructSize = sizeof(listenPara);
				listenPara.eDataType = TSDMX_DATA_PES;
				listenPara.nPID = g_nVideoStreamPID;
				listenPara.pfnCallback = TdmbVideoListener;
				listenPara.pvContext = pvContext;
				TsDmxListenPID(hTsDmx, &listenPara);

			}
		}
	}
	return BIT_OK;
}

static unsigned char g_pNALPrefix[] =  {0x0, 0x0, 0x0, 0x1};
static unsigned char g_pSpsPpsPrefix[] =  {0x0, 0x0, 0x0, 0x1};
static unsigned char g_pFrameBuffer[1024*1024] = {0};
static unsigned long g_dwCurFrmLen = 0;
static unsigned long g_dwFrmBufOffset = 0;
static FILE* g_fpAvcDump = 0;

static BIT_U32 g_gIDRFound = BIT_FALSE;
static BIT_U32 nPtsBase = 0;
static BIT_U32 nLastPts = 0;
static BIT_U32 nLastDts = 0;
BIT_U32 TdmbVideoListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pbData, BIT_U32 nDataSize, BIT_BYTE* pbHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts)
{
	if (nPktFlags&PKTFLAG_PAYLOAD_UNIT_START_INDICATOR)
	{
		SL_Packet syncLayerPkt;
		BIT_RETURNTYPE ret = BIT_ErrorUndefined;
		ret = syncLayerPkt.Load(pbData, nDataSize, g_pVisualSLConfig);
		if (ret != BIT_OK || !syncLayerPkt.pbSlPacketPayload)
			return ret;
		if (!g_gIDRFound)
		{
			g_gIDRFound = (syncLayerPkt.pbSlPacketPayload[4] & 0xf) == 5;
			// Just skip this frame until we find the first IDR
			if (!g_gIDRFound)
				return BIT_OK;
			memcpy(g_pFrameBuffer, g_pSpsPpsPrefix, sizeof(g_pSpsPpsPrefix));
			g_dwFrmBufOffset = sizeof(g_pSpsPpsPrefix);
			memcpy(g_pFrameBuffer+g_dwFrmBufOffset, g_pAvcCfgRecord->ppSequenceParameterSetNALUnit[0], g_pAvcCfgRecord->pSequenceParameterSetLength[0]);
			g_dwFrmBufOffset += g_pAvcCfgRecord->pSequenceParameterSetLength[0];
			memcpy(g_pFrameBuffer+g_dwFrmBufOffset, g_pSpsPpsPrefix, sizeof(g_pSpsPpsPrefix));
			g_dwFrmBufOffset += sizeof(g_pSpsPpsPrefix);
			memcpy(g_pFrameBuffer+g_dwFrmBufOffset, g_pAvcCfgRecord->ppPictureParameterSetNALUnit[0], g_pAvcCfgRecord->pPictureParameterSetLength[0]);
			g_dwFrmBufOffset += g_pAvcCfgRecord->pPictureParameterSetLength[0];
		}
		if (syncLayerPkt.nSlPktPayloadLen > 4)
		{
			g_dwCurFrmLen = (syncLayerPkt.pbSlPacketPayload[0]<<24)|(syncLayerPkt.pbSlPacketPayload[1]<<16)|(syncLayerPkt.pbSlPacketPayload[2]<<8)|(syncLayerPkt.pbSlPacketPayload[3]);
			memcpy(g_pFrameBuffer+g_dwFrmBufOffset, g_pNALPrefix, sizeof(g_pNALPrefix));
			g_dwFrmBufOffset += sizeof(g_pNALPrefix);
			memcpy(g_pFrameBuffer+g_dwFrmBufOffset, syncLayerPkt.pbSlPacketPayload+4, syncLayerPkt.nSlPktPayloadLen-4);
			g_dwFrmBufOffset += syncLayerPkt.nSlPktPayloadLen-4;
		}

	}
	else if (g_gIDRFound)
	{
		memcpy(g_pFrameBuffer+g_dwFrmBufOffset, pbData, nDataSize);
		g_dwFrmBufOffset += nDataSize;
		if (g_dwFrmBufOffset >= g_dwCurFrmLen)
		{
			fwrite(g_pFrameBuffer, 1, g_dwFrmBufOffset, g_fpAvcDump);
			g_dwFrmBufOffset = 0;
			g_dwCurFrmLen = 0;
			fflush(g_fpAvcDump);
		}
	}
	return BIT_OK;
}

static FILE* g_fpAACDump = 0;
static BIT_BOOL g_gAudioUnitStartIndicator = BIT_FALSE;

BIT_U32 TdmbAudioListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pbData, BIT_U32 nDataSize, BIT_BYTE* pbHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts)
{
	if(nPktFlags & PKTFLAG_PAYLOAD_UNIT_START_INDICATOR)
	{
		SL_Packet syncLayerPkt;
		BIT_RETURNTYPE ret = BIT_ErrorUndefined;
		ret = syncLayerPkt.Load(pbData, nDataSize, g_pVisualSLConfig);
		if (ret != BIT_OK || !syncLayerPkt.pbSlPacketPayload)
			return ret;
		g_gAudioUnitStartIndicator = BIT_TRUE;
		fwrite(syncLayerPkt.pbSlPacketPayload, 1, syncLayerPkt.nSlPktPayloadLen, g_fpAACDump);
	}		
	else if (g_gAudioUnitStartIndicator)
	{
		fwrite(pbData, 1, nDataSize, g_fpAACDump);
	}
	return BIT_OK;
}

extern "C"  int TdmbDump(char *pszInput, char *pszAudioDump, char *pszVisualDump)
{
	BIT_BYTE pbBuffer[1024];
	FILE *fpInput = fopen(pszInput, "rb");
	g_fpAvcDump = fopen(pszVisualDump, "wb");
	g_fpAACDump = fopen(pszAudioDump, "wb");
	if (!fpInput)
		return 0;
	TsDmxOpenParameters dmxOpenPara;
	memset(&dmxOpenPara, 0, sizeof(dmxOpenPara));
	dmxOpenPara.nStructSize = sizeof(TsDmxOpenParameters);
	dmxOpenPara.nFlag = 0;
	dmxOpenPara.nHeadLen = 4;
	dmxOpenPara.nPacketLen = 188;
	if (TsDmxOpen(&hTsDmx, &dmxOpenPara) != BIT_OK)
	{
		fclose(fpInput);
		return 0;
	}
	TsDmxListenParameters listenPara;
	memset(&listenPara, 0, sizeof(listenPara));
	listenPara.nStructSize = sizeof(listenPara);
	listenPara.eDataType = TSDMX_DATA_SECTION;
	listenPara.nPID = 0x00; //PAT
	listenPara.pfnCallback = TdmbSectionListener;
	listenPara.pvContext = fpInput;
	if (TsDmxListenPID(hTsDmx, &listenPara) != BIT_OK)
		goto Fail;
	while (1)
	{
		BIT_U32 nReadBytes = sizeof(pbBuffer);
		BIT_U32 nRet = 0;
		if ((nRet = (BIT_U32)fread(pbBuffer, 1, nReadBytes, fpInput)) != nReadBytes)
			break;
		TsDmxProcess(hTsDmx, pbBuffer, nReadBytes);
	}
Fail:
	fclose(fpInput);
	TsDmxClose(hTsDmx);
	fclose(g_fpAACDump);
	fclose(g_fpAvcDump);
	return 0;
}

