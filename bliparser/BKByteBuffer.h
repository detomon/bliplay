/**
 * Copyright (c) 2014 Simon Schoenenberger
 * http://blipkit.monoxid.net/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _BK_BYTE_BUFFER_H_
#define _BK_BYTE_BUFFER_H_

#include "BKBase.h"

typedef struct BKByteBuffer BKByteBuffer;

struct BKByteBuffer
{
	BKUInt flags;
	BKSize size;
	BKSize capacity;
	void * data;
};

/**
 * Initialize static buffer object
 *
 * If `initCapacity` is greater than 0 space for the specific number of bytes
 * is reserved
 */
extern BKInt BKByteBufferInit (BKByteBuffer * buffer, BKSize initCapacity);

/**
 * Allocate buffer object
 *
 * If `initCapacity` is greater than 0 space for the specific number of bytes
 * is reserved
 */
extern BKInt BKByteBufferAlloc (BKByteBuffer ** outBuffer, BKSize initCapacity);

/**
 * Dispose buffer
 */
extern void BKByteBufferDispose (BKByteBuffer * buffer);

/**
 * Get number of bytes
 */
extern BKSize BKByteBufferGetSize (BKByteBuffer const * buffer);

/**
 * Append character
 */
extern BKInt BKByteBufferAppendInt8 (BKByteBuffer * buffer, uint8_t c);

/**
 * Append character
 */
extern BKInt BKByteBufferAppendInt16 (BKByteBuffer * buffer, uint16_t c);

/**
 * Append integer
 */
extern BKInt BKByteBufferAppendInt32 (BKByteBuffer * buffer, uint32_t c);

/**
 * Append integer
 */
extern BKInt BKByteBufferAppendPtr (BKByteBuffer * buffer, void const * ptr, BKSize size);

/**
 * Empty buffer
 *
 * If `keepData` is 1 the allocated buffer is reused
 */
extern void BKByteBufferEmpty (BKByteBuffer * buffer, BKInt keepData);

#endif /* ! _BK_BYTE_BUFFER_H_ */
