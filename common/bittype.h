/*
* Copyright (c) 2004 BIT Everest, Lin Ma
*
* All Rights Reserved
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

#ifndef __BIT_TYPE_H__
#define __BIT_TYPE_H__

typedef signed char BIT_S8;
typedef unsigned char BIT_U8;
typedef unsigned short BIT_U16;
typedef signed short BIT_S16;
typedef unsigned long BIT_U32;
typedef signed long BIT_S32;
typedef unsigned __int64 BIT_U64;
typedef signed __int64 BIT_S64;
typedef void* BIT_PTR;
typedef unsigned char BIT_BYTE;

typedef void* BIT_HANDLE;

/* Return type
* Returning BIT_OK indicates success
*/
typedef enum BIT_RETURNTYPE
{
	BIT_OK = 0,
	BIT_ErrorUndefined = 0x80000000,
	BIT_ErrorBadParameter = 0x80001000,
	BIT_ErrorOutOfMemory = 0x80001001,
} BIT_RETURNTYPE;

typedef enum BIT_BOOL
{
	BIT_FALSE = 0,
	BIT_TRUE = 1,
	BIT_BOOL_MAX = 0x7fffffff,
} BIT_BOOL;

#define BIT_NULL (0)


/* Common function and definitions */
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


#endif /* __BIT_TYPE_H__ */