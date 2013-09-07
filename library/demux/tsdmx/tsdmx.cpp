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

BIT_RETURNTYPE TsDmxOpen(
		BIT_HANDLE* ppHandle, 
		TsDmxOpenParameters *pOpenParameters)
{
	if (!ppHandle || !pOpenParameters)
		return BIT_ErrorBadParameter;
	CTsDmx *pDmx = new CTsDmx();
	if (pDmx == 0)
		return BIT_ErrorOutOfMemory;
	BIT_RETURNTYPE ret = pDmx->Open(pOpenParameters);
	if (ret != BIT_OK)
		return ret;
	*ppHandle = pDmx;
	return BIT_OK;
}

BIT_RETURNTYPE TsDmxReset(
		BIT_HANDLE pHandle)
{
	CTsDmx *pDmx = reinterpret_cast<CTsDmx*>(pHandle);
	if (!pDmx)
		return BIT_ErrorBadParameter;
	return pDmx->Reset();
}

BIT_RETURNTYPE TsDmxListenPID(
		BIT_HANDLE pHandle,
		TsDmxListenParameters *pListenPara)
{
	CTsDmx *pDmx = reinterpret_cast<CTsDmx*>(pHandle);
	if (!pDmx)
		return BIT_ErrorBadParameter;
	return pDmx->ListenPID(pListenPara);
}

BIT_RETURNTYPE TsDmxRemovePID(
		BIT_HANDLE pHandle, 
		BIT_U32 nPID)
{
	CTsDmx *pDmx = reinterpret_cast<CTsDmx*>(pHandle);
	if (!pDmx)
		return BIT_ErrorBadParameter;
	return pDmx->RemovePID(nPID);
}

BIT_RETURNTYPE TsDmxProcess(
		 BIT_HANDLE pHandle,
		 BIT_PTR pData,
		 BIT_U32 nDataLen)
{
	CTsDmx *pDmx = reinterpret_cast<CTsDmx*>(pHandle);
	if (!pDmx)
		return BIT_ErrorBadParameter;
	return pDmx->Process(pData, nDataLen);
}

BIT_RETURNTYPE TsDmxClose(
		BIT_HANDLE pHandle)
{
	BIT_RETURNTYPE ret = BIT_ErrorUndefined;
	CTsDmx *pDmx = reinterpret_cast<CTsDmx*>(pHandle);
	if (!pDmx)
		return BIT_ErrorBadParameter;
	ret = pDmx->Close();
	delete pDmx;
	return ret;
}
