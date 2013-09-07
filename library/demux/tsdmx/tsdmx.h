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

#ifndef __BIT_TSDEMUX_H__
#define __BIT_TSDEMUX_H__

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "bittype.h"

/* pbPacket points to the beginning of the TS packet */
#define TSPKT_SYNC_BYTE(pbPacket) (pbPacket[0])
#define TSPKT_TEI(pbPacket) ((pbPacket[1]&0x80)>>7)
#define TSPKT_PAYLOAD_UNIT_START(pbPacket) ((pbPacket[1]&0x40)>>6)
#define TSPKT_PID(pbPacket) (((BIT_U32)(pbPacket[1]&0x1f)<<8) | pbPacket[2])
#define TSPKT_ADAPTATION_EXIST(pbPacket) ((pbPacket[3]&0x20)>>5)
#define TSPKT_PAYLOAD_EXIST(pbPacket) ((pbPacket[3]&0x10)>>4)
#define TSPKT_COUNTER(pbPacket) (pbPacket[3]&0xf)

typedef struct TsDmxOpenParameters 
{
	BIT_U32 nStructSize;
	BIT_U32 nPacketLen;
	BIT_U32 nHeadLen;
	BIT_U32 nFlag;
} TsDmxOpenParameters;

typedef enum TsDmxDataType
{
	TSDMX_DATA_NONE = 0,
	TSDMX_DATA_PAYLOAD,
	TSDMX_DATA_SECTION,
	TSDMX_DATA_PES,
} TsDmxDataType;

typedef enum TsDmxPktFlag
{
	PKTFLAG_DISCONTINUITY = (1<<0),
	PKTFLAG_PAYLOAD_EXIST = (1<<4),
	PKTFLAG_ADAPTATION_EXIST = (1<<5),
	// Reserved bits: bit 6, bit 7 and bit 8,
	PKTFLAG_TRANSPORT_PRIORITY = (1<<9),
	PKTFLAG_PAYLOAD_UNIT_START_INDICATOR = (1<<10),
	PKTFLAG_TRANSPORT_ERROR_INDICATOR = (1<<11),
	PKTFLAG_PTS_PRESENT = (1<<30),
	PKTFLAG_DTS_PRESENT = (1<<31),
} TsDmxPktFlag;

typedef struct TsDmxListenerData 
{
	BIT_U32		nPID;
	BIT_U32		nPktFlags;
	BIT_BYTE*	pbData;
	BIT_U32		nDataSize;
	BIT_BYTE*	pbPktHead;
	BIT_U64		pts;
	BIT_U64		dts;
	
} TsDmxListenerData;

/** Callback function for PID listener
    Note: nPktFlags, pbHead, nPts and nDts are for payload listener only. The demux will check the integrity for
	      sections thus it is not necessary for a section listener to check these parameters.
*/
typedef BIT_U32 (*PFNLISTENER)(
		BIT_PTR pvContext,
		BIT_U32 nPID,
		BIT_BYTE* pbData,
		BIT_U32 nDataSize,
		BIT_BYTE* pbHead,
		BIT_U32 nPktFlags,
		BIT_U64 nPts,
		BIT_U64 nDts);

typedef struct TsDmxListenParameters 
{
	BIT_U32 nStructSize;
	BIT_U32 nPID;
	TsDmxDataType eDataType;
	PFNLISTENER pfnCallback;
	BIT_PTR pvContext; // The demux will pass this when calling pfnCallback
} TsDmxListenParameters;

typedef struct ListenParameters
{
	BIT_U32 dwStructSize;
	BIT_PTR pvContext;
	PFNLISTENER pfnListener;
} ListenParameters;

BIT_RETURNTYPE TsDmxOpen(
		BIT_HANDLE* ppHandle, 
		TsDmxOpenParameters *pOpenParameters);

BIT_RETURNTYPE TsDmxReset(
		BIT_HANDLE pHandle);

/* Register a listener to the demux. All data for that PID will be delivered 
   to the caller via the callback function (PFNLISTENER)
 */ 
BIT_RETURNTYPE TsDmxListenPID(
		BIT_HANDLE pHandle, 
		TsDmxListenParameters *pListenPara);

BIT_RETURNTYPE TsDmxRemovePID(
		BIT_HANDLE pHandle, 
		BIT_U32 nPID);

BIT_RETURNTYPE TsDmxProcess(
		BIT_HANDLE pHandle,
		BIT_PTR pData,
		BIT_U32 nDataLen);

BIT_RETURNTYPE TsDmxClose(
		BIT_HANDLE pHandle);

#ifdef __cplusplus
};
#endif

#endif /* __BIT_TSDEMUX_H__ */