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

#include "descriptor.h"

typedef enum Mp4ClassTag
{
	// Descriptor tags
	Forbidden				= 0x00,
	ObjectDescrTag			= 0x01,
	InitialObjectDescrTag	= 0x02,
	ES_DescrTag				= 0x03,
	DecoderConfigDescrTag	= 0x04,
	DecSpecificInfoTag		= 0x05,
	SLConfigDescrTag		= 0x06,
	ExtSLConfigDescrTag		= 0x64,

	// Command tags
	ObjectDescrUpdateTag	= 0x01,
	ObjectDescrRemoveTag	= 0x02,
	ES_DescrUpdateTag		= 0x03,
	ES_DescrRemoveTag		= 0x04,
	IPMP_DescrUpdateTag		= 0x05,
	IPMP_DescrRemoveTag		= 0x06,
	ES_DescrRemoveRefTag	= 0x07,
	ObjectDescrExecuteTag	= 0x09,
} Mp4ClassTag;



/*
	Table 1 - List of Class Tags for Descriptors
	Tag value	Tag name
	0x00	Forbidden
	0x01	ObjectDescrTag
	0x02	InitialObjectDescrTag
	0x03	ES_DescrTag
	0x04	DecoderConfigDescrTag
	0x05	DecSpecificInfoTag
	0x06	SLConfigDescrTag
	0x07	ContentIdentDescrTag
	0x08	SupplContentIdentDescrTag
	0x09	IPI_DescrPointerTag
	0x0A	IPMP_DescrPointerTag
	0x0B	IPMP_DescrTag
	0x0C	QoS_DescrTag
	0x0D	RegistrationDescrTag
	0x0E	ES_ID_IncTag
	0x0F	ES_ID_RefTag
	0x10	MP4_IOD_Tag
	0x11	MP4_OD_Tag
	0x12	IPL_DescrPointerRefTag
	0x13	ExtensionProfileLevelDescrTag
	0x14	profileLevelIndicationIndexDescrTag
	0x15-0x3F	Reserved for ISO use
	0x40	ContentClassificationDescrTag
	0x41	KeyWordDescrTag
	0x42	RatingDescrTag
	0x43	LanguageDescrTag
	0x44	ShortTextualDescrTag
	0x45	ExpandedTextualDescrTag
	0x46	ContentCreatorNameDescrTag
	0x47	ContentCreationDateDescrTag
	0x48	OCICreatorNameDescrTag
	0x49	OCICreationDateDescrTag
	0x4A	SmpteCameraPositionDescrTag
	0x4B	SegmentDescrTag
	0x4C	MediaTimeDescrTag
	0x4D-0x5F	Reserved for ISO use (OCI extensions)
	0x60-0x61	Reserved for ISO use
	0x62	M4MuxTimingDescrTag
	0x63	M4MuxCodeTableDescrTag
	0x64	ExtSLConfigDescrTag
	0x65	M4MuxBufferSizeDescrTag
	0x66	M4MuxIdentDescrTag
	0x67	DependencyPointerTag
	0x68	DependencyMarkerTag
	0x69	M4MuxChannelDescrTag
	0x6A-0xBF	Reserved for ISO use
	0xC0-0xFE	User private
	0xFF	Forbidden
 */

IOD_descriptor::IOD_descriptor()
{
	pInitialObjectDescriptor = BIT_NULL;
}

IOD_descriptor::~IOD_descriptor()
{
	Reset();
}

BIT_RETURNTYPE IOD_descriptor::Reset()
{
	if (pInitialObjectDescriptor)
		delete pInitialObjectDescriptor;
	pInitialObjectDescriptor = BIT_NULL;
	return BIT_OK;
}

BIT_RETURNTYPE IOD_descriptor::Load(BitStream *pStream)
{
	BIT_U32 nSubDescrTag = 0;
	BIT_U32 nSubDescrSize = 0;
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 5)
		return BIT_ErrorBadParameter;
	Reset();
	descriptor_tag = (BIT_U8)BSReadBits(pStream, 8);
	if (descriptor_tag != 29)
		return BIT_ErrorUndefined;
	descriptor_length = (BIT_U8)BSReadBits(pStream, 8);
	if ((BIT_U32)descriptor_length > (BSGetLengthInBit(pStream)>>3))
		return BIT_ErrorUndefined;
	Scope_of_IOD_label = (BIT_U8)BSReadBits(pStream, 8);
	IOD_label = (BIT_U8)BSReadBits(pStream, 8);
	nSubDescrTag = BSReadBits(pStream, 8);
	nSubDescrSize = CalculateExpandableLength(pStream);
	if (nSubDescrTag != InitialObjectDescrTag)
		return BIT_ErrorUndefined;
	if (nSubDescrSize > (BSGetLengthInBit(pStream)>>3))
		return BIT_ErrorUndefined;
	pInitialObjectDescriptor = new InitialObjectDescriptor();
	if (pInitialObjectDescriptor == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	// Reset the length of the stream
	BSAlignToNextByte(pStream);
	pStream->pbBufferEnd = pStream->pbCurPointer + nSubDescrSize;
	if (pInitialObjectDescriptor->Load(pStream) != BIT_OK)
		return BIT_ErrorUndefined;
	return BIT_OK;
}

InitialObjectDescriptor::InitialObjectDescriptor()
{
	pbURLstring = BIT_NULL;
	pEsDescr = BIT_NULL;
	nEsDescrCount = 0;
}

InitialObjectDescriptor::~InitialObjectDescriptor()
{
	Reset();
}

BIT_RETURNTYPE InitialObjectDescriptor::Reset()
{
	if (pEsDescr)
		delete[] pEsDescr;
	pbURLstring = BIT_NULL;
	pEsDescr = BIT_NULL;
	nEsDescrCount = 0;
	return BIT_OK;
}

BIT_RETURNTYPE InitialObjectDescriptor::Load(BitStream *pStream)
{
	BIT_U32 nSubDescrTag = 0;
	BIT_U32 nSubDescrSize = 0;
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 3)
		return BIT_ErrorBadParameter;
	Reset();
	ObjectDescriptorID = (BIT_U16)BSReadBits(pStream, 10);
	URL_Flag = (BIT_U8)BSReadBits(pStream, 1);
	BIT_U32 includeInlineProfileLevelFlag = BSReadBits(pStream, 1);
	BIT_U32 reserved = BSReadBits(pStream, 4);
	if (reserved != 0xf)
		return BIT_ErrorUndefined;
	if (URL_Flag)
	{
		URLlength = (BIT_U8)BSReadBits(pStream, 8);
		if (URLlength > (BSGetLengthInBit(pStream)>>3))
			return BIT_ErrorUndefined;
		BSAlignToNextByte(pStream);
		pbURLstring = pStream->pbCurPointer;
		BSSkipBits(pStream, URLlength<<3);
	}
	else
	{
		ODProfileLevelIndication = (BIT_U8)BSReadBits(pStream, 8);
		sceneProfileLevelIndication = (BIT_U8)BSReadBits(pStream, 8);
		audioProfileLevelIndication = (BIT_U8)BSReadBits(pStream, 8);
		visualProfileLevelIndication = (BIT_U8)BSReadBits(pStream, 8);
		graphicsProfileLevelIndication = (BIT_U8)BSReadBits(pStream, 8);
		// 10 ES_Descriptor in maximum
		if (pEsDescr == BIT_NULL)
		{
			pEsDescr = new ES_Descriptor[10];
			if (pEsDescr == BIT_NULL)
				return BIT_ErrorOutOfMemory;
		}
		nEsDescrCount = 0;
		while ((BSGetLengthInBit(pStream)>>3) > 2)
		{
			BSAlignToNextByte(pStream);
			nSubDescrTag = BSReadBits(pStream, 8);
			nSubDescrSize = CalculateExpandableLength(pStream);			
			if (nSubDescrSize > (BSGetLengthInBit(pStream)>>3))
				return BIT_ErrorUndefined;
			BSAlignToNextByte(pStream);
			if (nSubDescrTag == ES_DescrTag && nEsDescrCount < 10)
			{
				// Parse ES_Descriptor				
				BitStream tempStream = *pStream;
				tempStream.pbBufferEnd = tempStream.pbCurPointer + nSubDescrSize;
				pEsDescr[nEsDescrCount++].Load(&tempStream);
			}
			// We just simply skip this descriptor right now
			// TODO: OCI_Descriptor ociDescr[0 .. 255];
			// TODO: IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];

			// Skip the descriptor bits
			BSSkipBits(pStream, nSubDescrSize<<3);
		}
	}
	//ExtensionDescriptor extDescr[0 .. 255];
	return BIT_OK;
}

ObjectDescriptor::ObjectDescriptor()
{
	pbURLstring = BIT_NULL;
	pEsDescr = BIT_NULL;
	nEsDescrCount = 0;
};

ObjectDescriptor::~ObjectDescriptor()
{
	Reset();
};

BIT_RETURNTYPE ObjectDescriptor::Reset()
{
	if (pEsDescr)
	{
		delete[] pEsDescr;
		pEsDescr = BIT_NULL;
	}
	nEsDescrCount = 0;
	pbURLstring = BIT_NULL;
	return BIT_OK;
};

BIT_RETURNTYPE ObjectDescriptor::Load(BitStream *pStream)
{
	BIT_U32 nSubDescrTag = 0;
	BIT_U32 nSubDescrSize = 0;
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 3)
		return BIT_ErrorBadParameter;
	Reset();
	ObjectDescriptorID = (BIT_U16)BSReadBits(pStream, 10);
	URL_Flag = (BIT_U8)BSReadBits(pStream, 1);
	BIT_U32 reserved = BSReadBits(pStream, 5);
	if (reserved != 0x1f)
		return BIT_ErrorUndefined;
	if (URL_Flag)
	{
		URLlength = (BIT_U8)BSReadBits(pStream, 8);
		if (URLlength > (BSGetLengthInBit(pStream)>>3))
			return BIT_ErrorUndefined;
		BSAlignToNextByte(pStream);
		pbURLstring = pStream->pbCurPointer;
		BSSkipBits(pStream, URLlength<<3);
	}
	else
	{
		// 10 ES_Descriptor in maximum
		if (pEsDescr == BIT_NULL)
		{
			pEsDescr = new ES_Descriptor[10];
			if (pEsDescr == BIT_NULL)
				return BIT_ErrorOutOfMemory;
		}
		nEsDescrCount = 0;
		while ((BSGetLengthInBit(pStream)>>3) > 2)
		{
			BSAlignToNextByte(pStream);
			nSubDescrTag = BSReadBits(pStream, 8);
			nSubDescrSize = CalculateExpandableLength(pStream);
			if (nSubDescrSize > (BSGetLengthInBit(pStream)>>3))
				return BIT_ErrorUndefined;
			BSAlignToNextByte(pStream);
			if (nSubDescrTag == ES_DescrTag && nEsDescrCount < 10)
			{
				// Parse ES_Descriptor				
				BitStream tempStream = *pStream;
				tempStream.pbBufferEnd = tempStream.pbCurPointer + nSubDescrSize;
				pEsDescr[nEsDescrCount++].Load(&tempStream);
			}
			// We just simply skip this descriptor right now
			// TODO: OCI_Descriptor ociDescr[0 .. 255];
			// TODO: IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];

			// Skip the descriptor bits
			BSSkipBits(pStream, nSubDescrSize<<3);
		}
	}
	return BIT_OK;
};

//
// ES_Descriptor
//
ES_Descriptor::ES_Descriptor()
{
	pbURLstring = BIT_NULL;
	pDecConfigDescr = BIT_NULL;
	pSlConfigDescr = BIT_NULL;
}

ES_Descriptor::~ES_Descriptor()
{
	Reset();
}

BIT_RETURNTYPE ES_Descriptor::Reset()
{
	if (pDecConfigDescr)
		delete pDecConfigDescr;
	if (pSlConfigDescr)
		delete pSlConfigDescr;
	pbURLstring = BIT_NULL;
	pDecConfigDescr = BIT_NULL;
	pSlConfigDescr = BIT_NULL;
	return BIT_OK;
}

BIT_RETURNTYPE ES_Descriptor::Load(BitStream *pStream)
{
	BIT_U32 nSubDescrTag = 0;
	BIT_U32 nSubDescrSize = 0;
	BitStream tempStream;
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 5)
		return BIT_ErrorBadParameter;
	Reset();
	ES_ID = (BIT_U16)BSReadBits(pStream, 16);
	streamDependenceFlag = (BIT_U8)BSReadBits(pStream, 1);
	URL_Flag = (BIT_U8)BSReadBits(pStream, 1);
	OCRstreamFlag = (BIT_U8)BSReadBits(pStream, 1);
	streamPriority = (BIT_U8)BSReadBits(pStream, 5);
	if (streamDependenceFlag)
		dependsOn_ES_ID = (BIT_U16)BSReadBits(pStream, 16);
	if (URL_Flag)
	{
		URLlength = (BIT_U8)BSReadBits(pStream, 8);
		if (URLlength > (BSGetLengthInBit(pStream)>>3))
			return BIT_ErrorUndefined;
		BSAlignToNextByte(pStream);
		pbURLstring = pStream->pbCurPointer;
		BSSkipBits(pStream, URLlength<<3);
	}
	if (OCRstreamFlag)
		OCR_ES_Id = (BIT_U16)BSReadBits(pStream, 16);
	nSubDescrTag = BSReadBits(pStream, 8);
	if (nSubDescrTag != DecoderConfigDescrTag)
		return BIT_ErrorUndefined;
	nSubDescrSize = CalculateExpandableLength(pStream);
	if (nSubDescrSize > (BSGetLengthInBit(pStream)>>3))
		return BIT_ErrorUndefined;
	pDecConfigDescr = new DecoderConfigDescriptor();
	if (pDecConfigDescr == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	BSAlignToNextByte(pStream);
	// Load DecoderConfigDescriptor  
	tempStream = *pStream;
	tempStream.pbBufferEnd = tempStream.pbCurPointer + nSubDescrSize;
	if (pDecConfigDescr->Load(&tempStream) != BIT_OK)
		return BIT_ErrorUndefined;
	// Skip the DecoderConfigDescriptor bits
	BSSkipBits(pStream, nSubDescrSize<<3);

	// Skip other descriptor bits
	while ((BSGetLengthInBit(pStream)>>3)  > 2)
	{
		BSAlignToNextByte(pStream);
		nSubDescrTag = BSReadBits(pStream, 8);
		nSubDescrSize = CalculateExpandableLength(pStream);
		if (nSubDescrSize > (BSGetLengthInBit(pStream)>>3))
			return BIT_ErrorUndefined;
		BSAlignToNextByte(pStream);
		if (nSubDescrTag == SLConfigDescrTag || nSubDescrTag == ExtSLConfigDescrTag)
		{
			// TODO: Support multiple SLConfigDescriptor
			if (!pSlConfigDescr)
			{
				pSlConfigDescr = new SLConfigDescriptor();
				if (pSlConfigDescr == BIT_NULL)
					return BIT_ErrorOutOfMemory;
			}
			tempStream = *pStream;
			tempStream.pbBufferEnd = tempStream.pbCurPointer + nSubDescrSize;
			BIT_RETURNTYPE ret = pSlConfigDescr->Load(&tempStream);
			pSlConfigDescr->bExtSLConfigDescr = (nSubDescrTag == ExtSLConfigDescrTag);
			if (ret != BIT_OK)
				return ret;
		}
		/*
			TODO: 
			IPI_DescrPointer ipiPtr[0 .. 1];
			IP_IdentificationDataSet ipIDS[0 .. 255];
			IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
			LanguageDescriptor langDescr[0 .. 255];
			QoS_Descriptor qosDescr[0 .. 1];
			RegistrationDescriptor regDescr[0 .. 1];
			ExtensionDescriptor extDescr[0 .. 255];
		*/
		BSSkipBits(pStream, nSubDescrSize<<3);
	}
	return BIT_OK;
}

DecoderConfigDescriptor::DecoderConfigDescriptor()
{
	//pDecSpecificInfo = BIT_NULL;
	//pProfileLevelIndicationIndexDescr = BIT_NULL;
}

DecoderConfigDescriptor::~DecoderConfigDescriptor()
{
	Reset();
}

BIT_RETURNTYPE DecoderConfigDescriptor::Reset()
{
	//if (pDecSpecificInfo)
	//	delete pDecSpecificInfo;
	//if (pProfileLevelIndicationIndexDescr)
	//	delete pProfileLevelIndicationIndexDescr;
	//pDecSpecificInfo = BIT_NULL;
	//pProfileLevelIndicationIndexDescr = BIT_NULL;
	return BIT_OK;
}

/*
	Table 4 - objectTypeIndication Values
	Value	ObjectTypeIndication Description
	0x00	Forbidden
	0x01	Systems ISO/IEC 14496-1   a
	0x02	Systems ISO/IEC 14496-1   b
	0x03	Interaction Stream
	0x04	Systems ISO/IEC 14496-1 Extended BIFS Configuration   c
	0x05	Systems ISO/IEC 14496-1 AFX  d
	0x06	reserved for ISO use
	0x07	reserved for ISO use
	0x08-0x1F	reserved for ISO use
	0x20	Visual ISO/IEC 14496-2   e
	0x21-0x3F	reserved for ISO use
	0x40	Audio ISO/IEC 14496-3   f
	0x41-0x5F	reserved for ISO use
	0x60	Visual ISO/IEC 13818-2 Simple Profile
	0x61	Visual ISO/IEC 13818-2 Main Profile
	0x62	Visual ISO/IEC 13818-2 SNR Profile
	0x63	Visual ISO/IEC 13818-2 Spatial Profile
	0x64	Visual ISO/IEC 13818-2 High Profile
	0x65	Visual ISO/IEC 13818-2 422 Profile
	0x66	Audio ISO/IEC 13818-7 Main Profile
	0x67	Audio ISO/IEC 13818-7 LowComplexity Profile
	0x68	Audio ISO/IEC 13818-7 Scaleable Sampling Rate Profile
	0x69	Audio ISO/IEC 13818-3
	0x6A	Visual ISO/IEC 11172-2
	0x6B	Audio ISO/IEC 11172-3
	0x6C	Visual ISO/IEC 10918-1
	0x6D - 0xBF	reserved for ISO use
	0xC0 - 0xFE	user private
	0xFF	no object type specified g
 */

BIT_RETURNTYPE DecoderConfigDescriptor::Load(BitStream *pStream)
{
	BIT_U32 nSubDescrTag = 0;
	BIT_U32 nSubDescrSize = 0;
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 5)
		return BIT_ErrorBadParameter;
	Reset();
	objectTypeIndication = (BIT_U8)BSReadBits(pStream, 8);
	streamType = (BIT_U8)BSReadBits(pStream, 6);
	upStream = (BIT_U8)BSReadBits(pStream, 1);
	BIT_U32 reserved = BSReadBits(pStream, 1);
	if (reserved != 0x01)
		return BIT_ErrorUndefined;
	bufferSizeDB = BSReadBits(pStream, 24);
	maxBitrate = BSReadBits(pStream, 32);
	avgBitrate = BSReadBits(pStream, 32);
	while ((BSGetLengthInBit(pStream)>>3)  > 2)
	{
		// The temporary stream pointer includes the "tag" and "expandable sizeOfInstance" bits 
		BitStream tempStream = *pStream;
		BSAlignToNextByte(pStream);
		nSubDescrTag = BSReadBits(pStream, 8);
		nSubDescrSize = CalculateExpandableLength(pStream);
		if (nSubDescrSize > (BSGetLengthInBit(pStream)>>3))
			return BIT_ErrorUndefined;
		/*	TDMB TTAS.KO-07.0026
		ObjectTypeIndication 
		0x02 Systems ISO/IEC 14496-1
		0x21 Visual ISO/IEC 14496-10
		0x40 Audio ISO/IEC 14496-3
		0x6C Visual ISO/IEC 10918-1£¸
		0xC0 ¨C 0xFE user private

		streamType value
		0x01 ObjectDescriptorStream
		0x02 ClockReferenceStream
		0x03 SceneDescriptionStream
		0x04 VisualStream
		0x05 AudioStream
		0x20 - 0x3F user private
		*/
		BSAlignToNextByte(pStream);
		if (nSubDescrTag == DecSpecificInfoTag)
		{
			if ((objectTypeIndication == 0x21 || objectTypeIndication == 0x6c) && streamType == 0x04)
			{
				// Video decoder specific information
				// The AVCDecoderConfigurationRecord does want the tag and expandable length bits...
				tempStream = *pStream;
				tempStream.pbBufferEnd = tempStream.pbCurPointer + nSubDescrSize;
				aVCDecoderConfigurationRecord.Load(&tempStream);
			}
			if (objectTypeIndication == 0x40 && streamType == 0x05)
			{
				// Audio decoder specific information
				tempStream = *pStream;
				tempStream.pbBufferEnd = tempStream.pbCurPointer + nSubDescrSize;
				audioSpecificConfig.Load(&tempStream);
			}
		}
		BSSkipBits(pStream, nSubDescrSize<<3);
		// TODO: profileLevelIndicationIndexDescriptor profileLevelIndicationIndexDescr [0..255];
	}
	return BIT_OK;
}

BIT_RETURNTYPE SLConfigDescriptor::Load(BitStream *pStream)
{
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 1)
		return BIT_ErrorBadParameter;
	predefined = (BIT_U8)BSReadBits(pStream, 8);
	if (predefined==0) 
	{
		useAccessUnitStartFlag = (BIT_U8)BSReadBits(pStream, 1);
		useAccessUnitEndFlag = (BIT_U8)BSReadBits(pStream, 1);
		useRandomAccessPointFlag = (BIT_U8)BSReadBits(pStream, 1);
		hasRandomAccessUnitsOnlyFlag = (BIT_U8)BSReadBits(pStream, 1);
		usePaddingFlag = (BIT_U8)BSReadBits(pStream, 1);
		useTimeStampsFlag = (BIT_U8)BSReadBits(pStream, 1);
		useIdleFlag = (BIT_U8)BSReadBits(pStream, 1);
		durationFlag = (BIT_U8)BSReadBits(pStream, 1);
		timeStampResolution = BSReadBits(pStream, 32);
		OCRResolution = BSReadBits(pStream, 32);
		timeStampLength = (BIT_U8)BSReadBits(pStream, 8);	// must be <= 64
		if (timeStampLength > 64)
			return BIT_ErrorUndefined;
		OCRLength = (BIT_U8)BSReadBits(pStream, 8);		// must be <= 64
		if (OCRLength > 64)
			return BIT_ErrorUndefined;
		AU_Length = (BIT_U8)BSReadBits(pStream, 8);		// must be <= 32
		if (AU_Length > 32)
			return BIT_ErrorUndefined;
		instantBitrateLength = (BIT_U8)BSReadBits(pStream, 8);
		degradationPriorityLength = (BIT_U8)BSReadBits(pStream,4);
		AU_seqNumLength = (BIT_U8)BSReadBits(pStream, 5); // must be <= 16
		if (AU_seqNumLength > 16)
			return BIT_ErrorUndefined;
		packetSeqNumLength = (BIT_U8)BSReadBits(pStream, 5); // must be <= 16
		if (packetSeqNumLength > 16)
			return BIT_ErrorUndefined;
		BIT_U8 reserved = (BIT_U8)BSReadBits(pStream, 2);
		if (reserved != 0x03)
			return BIT_ErrorUndefined;
	}
	if (durationFlag) 
	{
		timeScale = (BIT_U8)BSReadBits(pStream, 32);
		accessUnitDuration = (BIT_U8)BSReadBits(pStream, 16);
		compositionUnitDuration = (BIT_U8)BSReadBits(pStream, 16);
	}
	if (!useTimeStampsFlag) 
	{
		startDecodingTimeStamp = (BIT_U8)BSReadBits(pStream, timeStampLength);
		startCompositionTimeStamp = (BIT_U8)BSReadBits(pStream, timeStampLength);
	}
	// TODO: Support ExtendedSLConfigDescriptor 
	//SLExtensionDescriptor slextDescr[1..255];
	return BIT_OK;
}

SL_PacketHeader::SL_PacketHeader()
{
	accessUnitStartFlag= 0;
	accessUnitEndFlag= 0;
	OCRflag= 0;
	idleFlag= 0;
	paddingFlag= 0;
	paddingBits= 0;
	DegPrioflag= 0;
	randomAccessPointFlag= 0;
	packetSequenceNumber= 0;
	degradationPriority= 0;
	objectClockReference= 0;
	AU_sequenceNumber= 0;
	decodingTimeStampFlag= 0;
	compositionTimeStampFlag= 0;
	instantBitrateFlag= 0;
	reserve1= 0;
	decodingTimeStamp= 0;
	compositionTimeStamp= 0;
	accessUnitLength= 0;
	instantBitrate= 0;
}

BIT_RETURNTYPE SL_PacketHeader::Reset()
{
	accessUnitStartFlag= 0;
	accessUnitEndFlag= 0;
	OCRflag= 0;
	idleFlag= 0;
	paddingFlag= 0;
	paddingBits= 0;
	DegPrioflag= 0;
	randomAccessPointFlag= 0;
	packetSequenceNumber= 0;
	degradationPriority= 0;
	objectClockReference= 0;
	AU_sequenceNumber= 0;
	decodingTimeStampFlag= 0;
	compositionTimeStampFlag= 0;
	instantBitrateFlag= 0;
	reserve1= 0;
	decodingTimeStamp= 0;
	compositionTimeStamp= 0;
	accessUnitLength= 0;
	instantBitrate= 0;
	return BIT_OK;
}

BIT_RETURNTYPE SL_PacketHeader::Load(BitStream *pStream, SLConfigDescriptor *pSLConfig)
{
	if (!pSLConfig || !pStream || (BSGetLengthInBit(pStream)>>3) < 1)
		return BIT_ErrorBadParameter;
	Reset();
	if (pSLConfig->useAccessUnitStartFlag)
		accessUnitStartFlag = (BIT_U8)BSReadBits(pStream, 1);
	if (pSLConfig->useAccessUnitEndFlag)
		accessUnitEndFlag = (BIT_U8)BSReadBits(pStream, 1);
	if (pSLConfig->OCRLength>0)
		OCRflag = (BIT_U8)BSReadBits(pStream, 1);
	if (pSLConfig->useIdleFlag)
		idleFlag = (BIT_U8)BSReadBits(pStream, 1);
	if (pSLConfig->usePaddingFlag)
		paddingFlag = (BIT_U8)BSReadBits(pStream, 1);
	if (paddingFlag)
		paddingBits = (BIT_U8)BSReadBits(pStream, 3);

	if (!idleFlag && (!paddingFlag || paddingBits != 0))
	{
		if (pSLConfig->packetSeqNumLength > 0)
			packetSequenceNumber = BSReadBits(pStream, pSLConfig->packetSeqNumLength);
		if (pSLConfig->degradationPriorityLength > 0)
			DegPrioflag = (BIT_U8)BSReadBits(pStream, 1);
		if (DegPrioflag)
			degradationPriority = BSReadBits(pStream, pSLConfig->degradationPriorityLength);
		if (OCRflag)
			objectClockReference = BSReadBits(pStream, pSLConfig->OCRLength);
		if (accessUnitStartFlag) 
		{
			if (pSLConfig->useRandomAccessPointFlag)
				randomAccessPointFlag = (BIT_U8)BSReadBits(pStream, 1);
			if (pSLConfig->AU_seqNumLength >0)
				AU_sequenceNumber = BSReadBits(pStream, pSLConfig->AU_seqNumLength);
			if (pSLConfig->useTimeStampsFlag) 
			{
				decodingTimeStampFlag = (BIT_U8)BSReadBits(pStream, 1);
				compositionTimeStampFlag = (BIT_U8)BSReadBits(pStream, 1);
			}
			if (pSLConfig->instantBitrateLength > 0)
				instantBitrateFlag = (BIT_U8)BSReadBits(pStream, 1);
			if (decodingTimeStampFlag)
				decodingTimeStamp = BSReadBits(pStream, pSLConfig->timeStampLength);
			if (compositionTimeStampFlag)
				compositionTimeStamp = BSReadBits(pStream, pSLConfig->timeStampLength);
			if (pSLConfig->AU_Length > 0)
				accessUnitLength = BSReadBits(pStream, pSLConfig->AU_Length);
			if (instantBitrateFlag)
				instantBitrate = BSReadBits(pStream, pSLConfig->instantBitrateLength);
		}
		if (pSLConfig->bExtSLConfigDescr)
		{
			if (BSGetLengthInBit(pStream)>>3 < 2)
				return BIT_ErrorUndefined;
			BSAlignToNextByte(pStream);
			BIT_U32 nSubDescrTag = BSReadBits(pStream, 8);
			BIT_U32 nSubDescrSize = CalculateExpandableLength(pStream);
			// TODO: Support DependencyPointerTag and DependencyMarkerTag
			BSAlignToNextByte(pStream);
			BSSkipBits(pStream, nSubDescrSize<<3);
		}
	}
	return BIT_OK;
}

SL_Packet::SL_Packet()
{
	pbSlPacketPayload = 0;
}

BIT_RETURNTYPE SL_Packet::Reset()
{
	pbSlPacketPayload = 0;
	return BIT_OK;
}

BIT_RETURNTYPE SL_Packet::Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferSize, SLConfigDescriptor *pSLConfig)
{
	BitStream bitStream;
	BIT_RETURNTYPE ret = BIT_ErrorUndefined;
	if (!pSLConfig || !pbBuffer || nBufferSize < 1)
		return BIT_ErrorBadParameter;
	Reset();
	BSInit(&bitStream, pbBuffer, nBufferSize);
	ret = slPacketHeader.Load(&bitStream, pSLConfig);
	if (ret != BIT_OK)
		return ret;
	if ((BSGetLengthInBit(&bitStream)>>3) < 1)
		return BIT_ErrorUndefined;
	BSAlignToNextByte(&bitStream);
	pbSlPacketPayload = bitStream.pbCurPointer;
	nSlPktPayloadLen = BSGetLengthInBit(&bitStream)>>3;
	return BIT_OK;
}

ObjectDescriptorStream::ObjectDescriptorStream()
{
	pObjectDescriptor = BIT_NULL;
	nActualObjDescrNum = 0;
}

ObjectDescriptorStream::~ObjectDescriptorStream()
{
	Reset();
}

BIT_RETURNTYPE ObjectDescriptorStream::Reset()
{
	if (pObjectDescriptor)
	{
		delete[] pObjectDescriptor;
		pObjectDescriptor = BIT_NULL;
	}
	nActualObjDescrNum = 0;
	return BIT_OK;
}

BIT_RETURNTYPE ObjectDescriptorStream::Load(BIT_BYTE *pbBuffer, BIT_U32 nBufferSize, SLConfigDescriptor *pSLConfig)
{
	SL_Packet slPacket;
	BitStream bitStream;
	BIT_U32 nSubDescrTag = 0;
	BIT_U32 nSubDescrSize = 0;
	BIT_RETURNTYPE ret = BIT_ErrorUndefined;
	ret = slPacket.Load(pbBuffer, nBufferSize, pSLConfig);
	if (ret != BIT_OK)
		return ret;
	BSInit(&bitStream, slPacket.pbSlPacketPayload, slPacket.nSlPktPayloadLen);
	while ((BSGetLengthInBit(&bitStream)) > 0)
	{
		BSAlignToNextByte(&bitStream);
		BIT_U32 nCmdDescrTag = BSReadBits(&bitStream, 8);
		BIT_U32 nCmdDescrSize = CalculateExpandableLength(&bitStream);
		if (nCmdDescrSize > (BSGetLengthInBit(&bitStream)>>3))
			return BIT_ErrorUndefined;
		BSAlignToNextByte(&bitStream);
		// Can't pass the bit stream to second level parser. We need the pointers
		BitStream cmdDescrStream;
		BSInit(&cmdDescrStream, bitStream.pbCurPointer, nCmdDescrSize);
		if (nCmdDescrTag == ObjectDescrUpdateTag)
		{
			nActualObjDescrNum = 0;
			if (!pObjectDescriptor)
			{
				// Maximum 16 ObjectDescriptor supported in a ObjectDescriptorUpdate command
				// TODO: Support up to 255 ObjectDescriptor
				pObjectDescriptor = new ObjectDescriptor[16];
				if (!pObjectDescriptor)
					return BIT_ErrorOutOfMemory;
			}
			while ((BSGetLengthInBit(&cmdDescrStream)>>3) > 2)
			{
				BSAlignToNextByte(&cmdDescrStream);
				nSubDescrTag = BSReadBits(&cmdDescrStream, 8);
				nSubDescrSize = CalculateExpandableLength(&cmdDescrStream);
				if (nSubDescrSize > (BSGetLengthInBit(&cmdDescrStream)>>3))
					return BIT_ErrorUndefined;
				BSAlignToNextByte(&cmdDescrStream);
				BitStream tempStream;
				BSInit(&tempStream, cmdDescrStream.pbCurPointer, nSubDescrSize);
				if (nSubDescrTag == ObjectDescrTag && nActualObjDescrNum < 16)
				{
					BIT_RETURNTYPE ret = pObjectDescriptor[nActualObjDescrNum++].Load(&tempStream);
					if (ret != BIT_OK)
						return ret;
				}
				BSSkipBits(&cmdDescrStream, nSubDescrSize<<3);
			}
		}
		// Skip the command descriptor bits
		BSSkipBits(&bitStream, nCmdDescrSize<<3);
	}
	return BIT_OK;
}

AVCDecoderConfigurationRecord::AVCDecoderConfigurationRecord()
{
	numOfSequenceParameterSets = 0;
	pSequenceParameterSetLength = BIT_NULL;
	ppSequenceParameterSetNALUnit = BIT_NULL;
	numOfPictureParameterSets = 0;
	pPictureParameterSetLength = BIT_NULL;
	ppPictureParameterSetNALUnit = BIT_NULL;
}

AVCDecoderConfigurationRecord::~AVCDecoderConfigurationRecord()
{
	Reset();
}

BIT_RETURNTYPE AVCDecoderConfigurationRecord::Reset()
{
	if (ppSequenceParameterSetNALUnit)
		delete[] ppSequenceParameterSetNALUnit;
	if (pSequenceParameterSetLength)
		delete[] pSequenceParameterSetLength;
	if (ppPictureParameterSetNALUnit)
		delete[] ppPictureParameterSetNALUnit;
	if (pPictureParameterSetLength)
		delete[] pPictureParameterSetLength;	
	numOfSequenceParameterSets = 0;
	pSequenceParameterSetLength = BIT_NULL;
	ppSequenceParameterSetNALUnit = BIT_NULL;
	numOfPictureParameterSets = 0;
	pPictureParameterSetLength = BIT_NULL;
	ppPictureParameterSetNALUnit = BIT_NULL;
	return BIT_OK;
}

BIT_RETURNTYPE AVCDecoderConfigurationRecord::Load(BitStream *pStream)
{
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 6)
		return BIT_ErrorBadParameter;
	Reset();
	configurationVersion = (BIT_U8)BSReadBits(pStream, 8);
	AVCProfileIndication = (BIT_U8)BSReadBits(pStream, 8);
	profile_compatibility = (BIT_U8)BSReadBits(pStream, 8);
	AVCLevelIndication = (BIT_U8)BSReadBits(pStream, 8);
	BIT_U32 reserved = BSReadBits(pStream, 6);
	if (reserved != 0x3f)
		return BIT_ErrorUndefined;
	lengthSizeMinusOne = (BIT_U8)BSReadBits(pStream, 2);
	reserved = BSReadBits(pStream, 3);
	if (reserved != 0x07)
		return BIT_ErrorUndefined;
	numOfSequenceParameterSets = (BIT_U8)BSReadBits(pStream, 5);
	pSequenceParameterSetLength = new BIT_U16[numOfSequenceParameterSets];
	if (pSequenceParameterSetLength == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	ppSequenceParameterSetNALUnit = new BIT_BYTE*[numOfSequenceParameterSets];
	if (pSequenceParameterSetLength == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	for (BIT_U32 n = 0; n < numOfSequenceParameterSets; ++n)
	{
		BIT_U16 nLength = (BIT_U16)BSReadBits(pStream, 16);
		*(pSequenceParameterSetLength + n) = nLength;
		BSAlignToNextByte(pStream);
		*(ppSequenceParameterSetNALUnit + n) = pStream->pbCurPointer;
		BSSkipBits(pStream, nLength<<3);
	}
	numOfPictureParameterSets = (BIT_U8)BSReadBits(pStream, 8);
	pPictureParameterSetLength = new BIT_U16[numOfPictureParameterSets];
	if (pPictureParameterSetLength == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	ppPictureParameterSetNALUnit = new BIT_BYTE*[numOfPictureParameterSets];
	if (ppPictureParameterSetNALUnit == BIT_NULL)
		return BIT_ErrorOutOfMemory;
	for (BIT_U32 n = 0; n < numOfPictureParameterSets; ++n)
	{
		BIT_U16 nLength = (BIT_U16)BSReadBits(pStream, 16);
		*(pPictureParameterSetLength + n) = nLength;
		BSAlignToNextByte(pStream);
		*(ppPictureParameterSetNALUnit + n) = pStream->pbCurPointer;
		BSSkipBits(pStream, nLength<<3);
	}
	return BIT_OK;
}

static BIT_U32 samplingFrequencyTable[16] =
{
	96000, 88200, 64000, 
	48000, 44100, 32000,
	24000, 22050, 16000, 
	12000, 11025, 8000,	7350,
};

BIT_RETURNTYPE AudioSpecificConfig::Load(BitStream *pStream)
{
	if (pStream == BIT_NULL || (BSGetLengthInBit(pStream)>>3) < 4)
		return BIT_ErrorBadParameter;
	BSAlignToNextByte(pStream);
	for (BIT_U32 n = 0; n < 4; ++n)
		pAudioConfigBytes[n] = pStream->pbCurPointer[n];
	audioObjectType = (BIT_U8)BSReadBits(pStream, 5);
	samplingFrequencyIndex = (BIT_U8)BSReadBits(pStream, 4);
	if (samplingFrequencyIndex != 0xf)
	{	
		if (samplingFrequencyIndex >= sizeof(samplingFrequencyTable))
			return BIT_ErrorUndefined;
		samplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
	}
	else
	{
		samplingFrequency = BSReadBits(pStream, 24);
	}
	channelConfiguration = (BIT_U8)BSReadBits(pStream, 4);
}
