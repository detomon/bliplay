/**
 * Copyright (c) 2012-2015 Simon Schoenenberger
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

#ifndef _BK_ARRAY_H_
#define _BK_ARRAY_H_

#include "BKObject.h"

typedef struct BKArray BKArray;

struct BKArray
{
	BKObject object;
	BKSize   length;
	BKSize   capacity;
	BKSize   itemSize;
	void   * items;
};

/**
 * Initialize static array object
 *
 * The size of a single item is given with `itemSize`
 * If `initCapacity` is greater than 0, space for the specific number of items
 * is reserved
 */
extern BKInt BKArrayInit (BKArray * array, BKSize itemSize, BKSize initCapacity);

/**
 * Allocate array object
 *
 * The size of a single item is given with `itemSize`
 * If `initCapacity` is greater than 0, space for the specific number of items
 * is reserved
 */
extern BKInt BKArrayAlloc (BKArray ** outArray, BKSize itemSize, BKSize initCapacity);

/**
 * Get number of items
 */
extern BKSize BKArrayGetLength (BKArray const * array);

/**
 * Get item at index
 *
 * Returns NULL if index is out of bounds
 */
extern void * BKArrayGetItemAtIndex (BKArray const * array, BKSize index);

/**
 * Get item at index and copy to `outItem`
 *
 * Returns -1 if index is out of bounds
 */
extern BKInt BKArrayGetItemAtIndexCopy (BKArray const * array, BKSize index, void * outItem);

/**
 * Get last item
 *
 * Returns NULL if array is empty
 */
extern void * BKArrayGetLastItem (BKArray const * array);

/**
 * Get last item and copy to `outItem`
 *
 * Returns -1 if array is empty
 */
extern BKInt BKArrayGetLastItemCopy (BKArray const * array, void * outItem);

/**
 * Set item at index
 *
 * Returns -1 if an error occurred
 */
extern BKInt BKArraySetItemAtIndex (BKArray const * array, void * item, BKSize index);

/**
 * Append an item at end of array
 *
 * Returns a value other than 0 if an error occured
 */
extern BKInt BKArrayPush (BKArray * array, void const * item);

/**
 * Append an empty item at end of array and return a pointer to it
 *
 * Returns NULL if an error occurred
 */
extern void * BKArrayPushPtr (BKArray * array);

/**
 * Remove last item
 *
 * Returns -1 if array is empty
 * If `outItem` is not NULL the last item is copied into it or emptied if array
 * is empty
 */
extern BKInt BKArrayPop (BKArray * array, void * outItem);

/**
 * Empty array
 *
 * If `keepData` is 1 the allocated buffer is reused
 */
extern void BKArrayEmpty (BKArray * array, BKInt keepData);

#endif /* ! _BK_ARRAY_H_ */
