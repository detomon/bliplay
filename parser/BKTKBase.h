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
