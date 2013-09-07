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

#ifndef __BIT_DESCRIPTOR_H__
#define __BIT_DESCRIPTOR_H__

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


#include "bittype.h"
#include "bitstream.h"

//
// Definitions
//

// Refer to Table 2-39 ¨C Program and program element descriptors in 13818-1
typedef enum Mp2SystemDescrTag
{
	Mp2DescrTag_Reserved = 0,
	Mp2DescrTag_IOD_descriptor = 29,
	Mp2DescrTag_SL_descriptor = 30,
} Mp2SystemDescrTag;


//
// MPEG-2 descriptor (defined in 13818-1)
//
typedef struct SL_descriptor
{
	BIT_U8 descriptor_tag;
	BIT_U8 descriptor_length;
	BIT_U16 ES_ID;
} SL_descriptor;

//
// Note: IOD_descriptor is not a MPEG-4 system descriptor. It is defined in MPEG-2 system spec. (13818-1)
//       while the InitialObjectDescriptor is defined in MPEG-4 system (14496-1)
//
class InitialObjectDescriptor;
class IOD_descriptor
{
public:
	IOD_descriptor();
	~IOD_descriptor();
	BIT_RETURNTYPE Load(BitStream *pStream);

protected:
	BIT_RETURNTYPE Reset();

public:
	BIT_U8 descriptor_tag;
	BIT_U8 descriptor_length;
	BIT_U8 Scope_of_IOD_label;
	BIT_U8 IOD_label;
	InitialObjectDescriptor *pInitialObjectDescriptor;
};

/*
	MPEG-4 Stream Type definitions

	Table 5 - streamType Values. Please refer to MPEG-4 system (14496-1)
	streamType value	Stream type description
	0x00	Forbidden
	0x01	ObjectDescriptorStream (see 1.5)
	0x02	ClockReferenceStream (see 2.2.5)
	0x03	SceneDescriptionStream (see 0.1.1)
	0x04	VisualStream
	0x05	AudioStream
	0x06	MPEG7Stream
	0x07	IPMPStream (see 1.3.2)
	0x08	ObjectContentInfoStream (see 1.4.2)
	0x09	MPEGJStream
	0x0A	Interaction Stream
	0x0B - 0x1F	reserved for ISO use
	0x20 - 0x3F	user private
 */
typedef enum Mp4SystemStreamType
{
	Mp4StreamType_Forbidden = 0x00,
	Mp4StreamType_ObjectDescriptorStream = 0x01,
	Mp4StreamType_ClockReferenceStream = 0x02,
	Mp4StreamType_SceneDescriptionStream = 0x03,
	Mp4StreamType_VisualStream = 0x04,
	Mp4StreamType_AudioStream = 0x05,
	Mp4StreamType_MPEG7Stream = 0x06,
	Mp4StreamType_IPMPStream = 0x07,
	Mp4StreamType_ObjectContentInfoStream = 0x08,
	Mp4StreamType_MPEGJStream = 0x09,
	Mp4StreamType_InteractionStream = 0x0A,
} Mp4SystemStreamType;

// Refer to Table 2-29 ¨C Stream type assignments, ISO/IEC 13818-1 : 2000 (E)
typedef enum Mp2SystemStreamType
{
	Mp2StreamType_Reserved = 0x00,
	Mp2StreamType_SyncLayerInPES = 0x12,		// ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets 
	Mp2StreamType_SyncLayerInSection = 0x13,	// ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC14496_sections.
} Mp2SystemStreamType;

//
// Utility functions
//

/*
	Please refer to 14.3.3 Expandable classes, ISO/IEC 14496-1
	The length encoding is itself defined in SDL as follows:
	int sizeOfInstance = 0;
	BIT_U8 nextByte;
	bit(7) sizeOfInstance;
	while(nextByte) {
	BIT_U8 nextByte;
	bit(7) sizeByte;
	sizeOfInstance = sizeOfInstance<<7 | sizeByte;
}
 */
inline BIT_U32 CalculateExpandableLength(BitStream *pStream)
{
	BIT_U32 bNextByte = BSReadBits(pStream, 1);
	BIT_U32 nSizeOfInstance = BSReadBits(pStream, 7);
	while (bNextByte)
	{
		bNextByte = BSReadBits(pStream, 1);
		nSizeOfInstance = nSizeOfInstance<<7 | BSReadBits(pStream, 7);
	}
	return nSizeOfInstance;
}

class BaseDescriptor
{
public:
	//BIT_U8 tag;
	//BIT_U32 nSizeOfInstance;
};

class ES_Descriptor;

class InitialObjectDescriptor : public BaseDescriptor
{
public:
	InitialObjectDescriptor();
	~InitialObjectDescriptor();
	BIT_RETURNTYPE Load(BitStream *pStream);
	BIT_RETURNTYPE Reset();

public:
	BIT_U16 ObjectDescriptorID;
	BIT_U8 URL_Flag;

	// if URL_Flag present
	BIT_U8 URLlength;
	BIT_BYTE *pbURLstring;

	// else
	BIT_U8 ODProfileLevelIndication;
	BIT_U8 sceneProfileLevelIndication;
	BIT_U8 audioProfileLevelIndication;
	BIT_U8 visualProfileLevelIndication;
	BIT_U8 graphicsProfileLevelIndication;

	ES_Descriptor *pEsDescr;
	BIT_U8 nEsDescrCount;
	//OCI_Descriptor ociDescr[0 .. 255];
	//IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
	//ExtensionDescriptor extDescr[0 .. 255];
};

class ObjectDescriptor 
{
public:
	ObjectDescriptor();
	~ObjectDescriptor();
	BIT_RETURNTYPE Load(BitStream *pStream);
	BIT_RETURNTYPE Reset();

public:
	BIT_U16 ObjectDescriptorID;
	BIT_U8 URL_Flag;
	BIT_U8 URLlength;
	BIT_BYTE *pbURLstring;
	ES_Descriptor *pEsDescr;
	BIT_U8 nEsDescrCount;
	//OCI_Descriptor ociDescr[0 .. 255];
	//IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
	//ExtensionDescriptor extDescr[0 .. 255];
};

class DecoderConfigDescriptor;
class SLConfigDescriptor;

class ES_Descriptor : public BaseDescriptor
{
public:
	ES_Descriptor();
	~ES_Descriptor();
	BIT_RETURNTYPE Load(BitStream *pStream);

protected:
	BIT_RETURNTYPE Reset();

public:
	BIT_U16 ES_ID;
	BIT_U8 streamDependenceFlag;
	BIT_U8 URL_Flag;
	BIT_U8 OCRstreamFlag;
	BIT_U8 streamPriority;
	BIT_U16 dependsOn_ES_ID;	//if (streamDependenceFlag)	
	BIT_U8 URLlength;			//if (URL_Flag) 
	BIT_BYTE *pbURLstring;		//if (URL_Flag) 
	BIT_U16 OCR_ES_Id;			//if (OCRstreamFlag)
	DecoderConfigDescriptor *pDecConfigDescr;
	SLConfigDescriptor *pSlConfigDescr;
	//IPI_DescrPointer ipiPtr[0 .. 1];
	//IP_IdentificationDataSet ipIDS[0 .. 255];
	//IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
	//LanguageDescriptor langDescr[0 .. 255];
	//QoS_Descriptor qosDescr[0 .. 1];
	//RegistrationDescriptor regDescr[0 .. 1];
	//ExtensionDescriptor extDescr[0 .. 255];
};

class DecoderSpecificInfo;
class ProfileLevelIndicationIndexDescr;
class AVCDecoderConfigurationRecord;

//
// Decoder Specific Information structures (AVC and AAC)
//
class AVCDecoderConfigurationRecord 
{
public:
	AVCDecoderConfigurationRecord();
	~AVCDecoderConfigurationRecord();
	BIT_RETURNTYPE Load(BitStream *pStream);

protected:
	BIT_RETURNTYPE Reset();

public:
	BIT_U8 configurationVersion;
	BIT_U8 AVCProfileIndication;
	BIT_U8 profile_compatibility;
	BIT_U8 AVCLevelIndication; 
	BIT_U8 lengthSizeMinusOne; 
	BIT_U8 numOfSequenceParameterSets;
	BIT_U16 *pSequenceParameterSetLength;
	BIT_BYTE **ppSequenceParameterSetNALUnit;
	BIT_U8 numOfPictureParameterSets;
	BIT_U16 *pPictureParameterSetLength;
	BIT_BYTE **ppPictureParameterSetNALUnit;	
};

class AudioSpecificConfig
{
public:
	BIT_RETURNTYPE Load(BitStream *pStream);

public:
	BIT_U8 audioObjectType;
	BIT_U8 samplingFrequencyIndex;
	BIT_U32 samplingFrequency;
	BIT_U8 channelConfiguration;
	BIT_BYTE  pAudioConfigBytes[4];
};

class DecoderConfigDescriptor : public BaseDescriptor
{
public:
	DecoderConfigDescriptor();
	~DecoderConfigDescriptor();
	BIT_RETURNTYPE Load(BitStream *pStream);

protected:
	BIT_RETURNTYPE Reset();

public:
	BIT_U8 objectTypeIndication;
	BIT_U8 streamType;
	BIT_U8 upStream;
	BIT_U32 bufferSizeDB;
	BIT_U32 maxBitrate;
	BIT_U32 avgBitrate;
	//DecoderSpecificInfo *pDecSpecificInfo;
	//ProfileLevelIndicationIndexDescr *pProfileLevelIndicationIndexDescr;
	AVCDecoderConfigurationRecord aVCDecoderConfigurationRecord;
	AudioSpecificConfig audioSpecificConfig;
};

class SLConfigDescriptor  : public BaseDescriptor
{
public:
	BIT_RETURNTYPE Load(BitStream *pStream);

public:
	BIT_U8 predefined;
	BIT_U8 useAccessUnitStartFlag;
	BIT_U8 useAccessUnitEndFlag;
	BIT_U8 useRandomAccessPointFlag;
	BIT_U8 hasRandomAccessUnitsOnlyFlag;
	BIT_U8 usePaddingFlag;
	BIT_U8 useTimeStampsFlag;
	BIT_U8 useIdleFlag;
	BIT_U8 durationFlag;
	BIT_U32 timeStampResolution;
	BIT_U32 OCRResolution;
	BIT_U8 timeStampLength;	// must be <= 64
	BIT_U8 OCRLength;		// must be <= 64
	BIT_U8 AU_Length;		// must be <= 32
	BIT_U8 instantBitrateLength;
	BIT_U8 degradationPriorityLength;
	BIT_U8 AU_seqNumLength; // must be <= 16
	BIT_U8 packetSeqNumLength; // must be <= 16
	BIT_U32 timeScale;
	BIT_U16 accessUnitDuration;
	BIT_U16 compositionUnitDuration;
	BIT_U64 startDecodingTimeStamp;
	BIT_U64 startCompositionTimeStamp;
	BIT_U32 bExtSLConfigDescr;
};

// The SL_PacketHeader is not an expandable calss
class SL_PacketHeader
{
public:
	SL_PacketHeader();
	BIT_RETURNTYPE Load(BitStream *pStream, SLConfigDescriptor *pSLConfig);
	BIT_RETURNTYPE Reset();

public:
	BIT_U8 accessUnitStartFlag;
	BIT_U8 accessUnitEndFlag;
	BIT_U8 OCRflag;
	BIT_U8 idleFlag;
	BIT_U8 paddingFlag;
	BIT_U8 paddingBits;
	BIT_U8 DegPrioflag;
	BIT_U8 randomAccessPointFlag;
	BIT_U32 packetSequenceNumber;
	BIT_U32 degradationPriority;
	BIT_U32 objectClockReference;
	BIT_U32 AU_sequenceNumber;
	BIT_U8 decodingTimeStampFlag;
	BIT_U8 compositionTimeStampFlag;
	BIT_U8 instantBitrateFlag;
	BIT_U8 reserve1;
	BIT_U32 decodingTimeStamp;
	BIT_U32 compositionTimeStamp;
	BIT_U32 accessUnitLength;
	BIT_U32 instantBitrate;
};

class SL_Packet
{
public:
	SL_Packet();
	BIT_RETURNTYPE Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferSize, SLConfigDescriptor *pSLConfig);
	BIT_RETURNTYPE Reset();

public:
	SL_PacketHeader slPacketHeader;
	BIT_BYTE *pbSlPacketPayload;
	BIT_U32 nSlPktPayloadLen;
};

class ObjectDescriptorStream
{
public:
	ObjectDescriptorStream();
	~ObjectDescriptorStream();
	BIT_RETURNTYPE Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferSize, SLConfigDescriptor *pSLConfig);
	BIT_RETURNTYPE Reset();

public:
	ObjectDescriptor *pObjectDescriptor;
	BIT_U32 nActualObjDescrNum;
};


#endif /* __BIT_DESCRIPTOR_H__ */

