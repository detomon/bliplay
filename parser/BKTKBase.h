/**
 * Copyright (c) 2012-2016 Simon Schoenenberger
 * http://blipkit.audio
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

#ifndef _BK_TK_BASE_H_
#define _BK_TK_BASE_H_

#include <stdarg.h>
#include "BKArray.h"
#include "BKBase.h"
#include "BKBlockPool.h"
#include "BKByteBuffer.h"
#include "BKHashTable.h"
#include "BKObject.h"
#include "BKString.h"
#include "BKTrack.h"

typedef struct BKString BKString;
typedef struct BKTKOffset BKTKOffset;
typedef struct BKTKTrack BKTKTrack;
typedef struct BKTKCompiler BKTKCompiler;

/**
 * Defines an offset in the given string
 */
struct BKTKOffset
{
	BKInt lineno;
	BKInt colno;
};

/**
 * Get the next power of 2
 */
BK_INLINE BKUSize BKNextPow2 (BKUSize v)
{
	v --;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v ++;

	v += (v == 0);

	return v;
}

#endif /* ! _BK_TK_BASE_H_ */
