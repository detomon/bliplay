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

#include "BKArray.h"

#define MIN_CAPACITY 8

static BKInt BKArrayGrow (BKArray * array, size_t minCapacity)
{
	void * newItems;
	size_t newCapacity;

	newCapacity = BKMax (array -> capacity + minCapacity * array -> itemSize, array -> capacity * 1.5);
	newCapacity = BKMax (MIN_CAPACITY, newCapacity);

	newItems = realloc (array -> items, newCapacity * array -> itemSize);

	if (newItems == NULL) {
		return -1;
	}

	array -> capacity = newCapacity;
	array -> items    = newItems;

	return 0;
}

BKInt BKArrayInit (BKArray * array, size_t itemSize, size_t initCapacity)
{
	memset (array, 0, sizeof (*array));

	array -> itemSize = BKMax (1, itemSize);
	array -> capacity = initCapacity;
	array -> items    = malloc (array -> capacity * array -> itemSize);

	if (array -> items == NULL) {
		return -1;
	}

	return 0;
}

void BKArrayDispose (BKArray * array)
{
	if (array -> items) {
		free (array -> items);
	}

	memset (array, 0, sizeof (*array));
}

size_t BKArrayGetLength (BKArray const * array)
{
	return array -> length;
}

void * BKArrayGetItemAtIndex (BKArray const * array, size_t index)
{
	if (index >= array -> length) {
		return NULL;
	}

	return array -> items + index * array -> itemSize;
}

BKInt BKArrayGetItemAtIndexCopy (BKArray const * array, size_t index, void * outItem)
{
	void const * item = BKArrayGetItemAtIndex (array, index);

	if (item == NULL) {
		return -1;
	}

	memcpy (outItem, item, array -> itemSize);

	return 0;
}

void * BKArrayGetLastItem (BKArray const * array)
{
	if (array -> length == 0) {
		return NULL;
	}

	return BKArrayGetItemAtIndex (array, array -> length - 1);
}

BKInt BKArrayGetLastItemCopy (BKArray const * array, void * outItem)
{
	void const * item = BKArrayGetLastItem (array);

	if (item == NULL) {
		return -1;
	}

	memcpy (outItem, item, array -> itemSize);

	return 0;
}

BKInt BKArrayPush (BKArray * array, void const * item)
{
	if (array -> length >= array -> capacity) {
		if (BKArrayGrow (array, 1) < 0) {
			return -1;
		}
	}

	memcpy (array -> items + array -> length * array -> itemSize, item, array -> itemSize);
	array -> length ++;

	return 0;
}

void * BKArrayPushPtr (BKArray * array)
{
	void * ptr;

	if (array -> length >= array -> capacity) {
		if (BKArrayGrow (array, 1) < 0) {
			return NULL;
		}
	}

	ptr = array -> items + array -> length * array -> itemSize;
	memset (ptr, 0, array -> itemSize);
	array -> length ++;

	return ptr;
}

BKInt BKArrayPop (BKArray * array, void * outItem)
{
	void const * ptr = BKArrayGetItemAtIndex (array, array -> length - 1);

	if (ptr) {
		if (outItem) {
			memcpy (outItem, ptr, array -> itemSize);
		}

		array -> length --;

		return 0;
	}
	else if (outItem) {
		memset (outItem, 0, array -> itemSize);
	}

	return -1;
}

void BKArrayEmpty (BKArray * array, BKInt keepData)
{
	if (keepData == 0) {
		if (array -> items) {
			free (array -> items);
			array -> items = NULL;
		}

		array -> capacity = 0;
	}

	array -> length = 0;
}
