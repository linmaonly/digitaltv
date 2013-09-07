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

// PID
enum ONESEG_PID
{
	ONESEG_PID_NIT = 0x0010,
	ONESEG_PID_SDT = 0x0011,
	ONESEG_PID_TOT = 0x0014,
	ONESEG_PID_BIT = 0x0024,
	ONESEG_PID_EIT = 0x0027,
	ONESEG_PID_PMT = 0x1FC8,
};

// Stream Type
enum ONESEG_STREAM_TYPE
{
	ONESEG_STREAM_TYPE_CLOSE_CAPTION = 0x6,
	ONESEG_STREAM_TYPE_DSMCC = 0x0D,
	ONESEG_STREAM_TYPE_AAC_ADTS = 0x0F,
	ONESEG_STREAM_TYPE_H264_VIDEO = 0x1b,
};

static BIT_HANDLE hTsDmx = NULL;
static BIT_U32 g_nPMTPID = (BIT_U32)ONESEG_PID_PMT;

BIT_U32 OneSegVideoListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pData, BIT_U32 nDataSize, BIT_BYTE* pHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts);
BIT_U32 OneSegAudioListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pData, BIT_U32 nDataSize, BIT_BYTE* pHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts);

BIT_U32 OneSegSectionListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pData, BIT_U32 nDataSize, BIT_BYTE* pHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts)
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
				listenPara.pfnCallback = OneSegSectionListener;
				listenPara.pvContext = pvContext;
				TsDmxListenPID(hTsDmx, &listenPara);
				g_nPMTPID = pat.program_map_PID[0];
			}
		}
		return BIT_OK;
	}
	if (TSPKT_PID(pHead) == g_nPMTPID && g_nPMTPID != (BIT_U32)-1)
	{
		CProgramMapSection pmt;
		if (pmt.Load((BIT_BYTE*)pData, nDataSize) == BIT_OK)
		{
			for (BIT_U32 n = 0; n < pmt.nActualESNum; n++)
			{
				if (pmt.pESInfo[n].stream_type == ONESEG_STREAM_TYPE_H264_VIDEO)
				{
					TsDmxListenParameters listenPara;
					memset(&listenPara, 0, sizeof(listenPara));
					listenPara.nStructSize = sizeof(listenPara);
					listenPara.eDataType = TSDMX_DATA_PES;
					listenPara.nPID = pmt.pESInfo[n].elementary_PID;
					listenPara.pfnCallback = OneSegVideoListener;
					listenPara.pvContext = pvContext;
					TsDmxListenPID(hTsDmx, &listenPara);
				}
				if (pmt.pESInfo[n].stream_type == ONESEG_STREAM_TYPE_AAC_ADTS)
				{
					TsDmxListenParameters listenPara;
					memset(&listenPara, 0, sizeof(listenPara));
					listenPara.nStructSize = sizeof(listenPara);
					listenPara.eDataType = TSDMX_DATA_PES;
					listenPara.nPID = pmt.pESInfo[n].elementary_PID;
					listenPara.pfnCallback = OneSegAudioListener;
					listenPara.pvContext = pvContext;
					TsDmxListenPID(hTsDmx, &listenPara);
				}
			}
		}
	}
	return BIT_OK;
}

static BIT_BOOL g_gIDRFound = BIT_FALSE;
static BIT_U32 nPtsBase = 0;
static BIT_U32 nLastPts = 0;
static BIT_U32 nLastDts = 0;

FILE *fpAVCDump = 0;
FILE *fpAACDump = 0;

BIT_U32 OneSegVideoListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pbData, BIT_U32 nDataSize, BIT_BYTE* pbHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts)
{
	//if (nPktFlags&PKTFLAG_PTS_PRESENT)
	//{
	//	BIT_U32 pts = nPts/90;
	//	if (nPtsBase == 0)
	//		nPtsBase = pts;
	//	pts -= nPtsBase;
	//	printf("PTS:%d \t Increment:%d\r\n", pts, pts-nLastPts);
	//	nLastPts = pts;
	//}
	fwrite(pbData, 1, nDataSize, fpAVCDump);
	return BIT_OK;
}

BIT_U32 OneSegAudioListener(BIT_PTR pvContext, BIT_U32 nPID, BIT_BYTE* pbData, BIT_U32 nDataSize, BIT_BYTE* pbHead, BIT_U32 nPktFlags, BIT_U64 nPts, BIT_U64 nDts)
{
	fwrite(pbData, 1, nDataSize, fpAACDump);
	return BIT_OK;
}

extern "C"  int OneSegDump(char *pszInput, char *pszAudioDump, char *pszVisualDump)
{
	BIT_BYTE pbBuffer[1024];
	FILE *fpInput = fopen(pszInput, "rb");
	fpAVCDump = fopen(pszVisualDump, "wb");
	fpAACDump = fopen(pszAudioDump, "wb");
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
	listenPara.nPID = g_nPMTPID;
	listenPara.pfnCallback = OneSegSectionListener;
	listenPara.pvContext = fpInput;
	if (TsDmxListenPID(hTsDmx, &listenPara) != BIT_OK)
		goto Fail;
	while (1)
	{
		BIT_U32 nReadBytes = sizeof(pbBuffer);
		BIT_U32 nRet = 0;
		if ((nRet = fread(pbBuffer, 1, nReadBytes, fpInput)) != nReadBytes)
			break;
		TsDmxProcess(hTsDmx, pbBuffer, nReadBytes);
	}
Fail:
	fclose(fpInput);
	TsDmxClose(hTsDmx);
	fclose(fpAVCDump);
	fclose(fpAACDump);

	return 0;
}
