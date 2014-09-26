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

#include "BKByteBuffer.h"

#define MIN_CAPACITY 64

static BKInt BKByteBufferGrow (BKByteBuffer * buffer, size_t minCapacity)
{
	void * newData;
	size_t newCapacity;

	newCapacity = BKMax (buffer -> capacity + minCapacity, buffer -> capacity * 1.5);
	newCapacity = BKMax (MIN_CAPACITY, newCapacity);

	newData = realloc (buffer -> data, newCapacity);

	if (newData == NULL) {
		return -1;
	}

	buffer -> capacity = newCapacity;
	buffer -> data     = newData;

	return 0;
}

BKInt BKByteBufferInit (BKByteBuffer * buffer, size_t initCapacity)
{
	memset (buffer, 0, sizeof (*buffer));

	buffer -> capacity = initCapacity;
	buffer -> data     = malloc (buffer -> capacity);

	if (buffer -> data == NULL) {
		return -1;
	}

	return 0;
}

void BKByteBufferDispose (BKByteBuffer * buffer)
{
	if (buffer -> data) {
		free (buffer -> data);
	}

	memset (buffer, 0, sizeof (*buffer));
}

size_t BKByteBufferGetSize (BKByteBuffer const * buffer)
{
	return buffer -> size;
}

BKInt BKByteBufferAppendInt8 (BKByteBuffer * buffer, uint8_t c)
{
	size_t size = sizeof (c);

	if (buffer -> size + size > buffer -> capacity) {
		if (BKByteBufferGrow (buffer, size) < 0) {
			return -1;
		}
	}

	* (uint8_t *) (buffer -> data + buffer -> size) = c;
	buffer -> size += size;

	return 0;
}

BKInt BKByteBufferAppendInt16 (BKByteBuffer * buffer, uint16_t c)
{
	size_t size = sizeof (c);

	if (buffer -> size + size > buffer -> capacity) {
		if (BKByteBufferGrow (buffer, size) < 0) {
			return -1;
		}
	}

	* (uint16_t *) (buffer -> data + buffer -> size) = c;
	buffer -> size += size;

	return 0;
}

BKInt BKByteBufferAppendInt32 (BKByteBuffer * buffer, uint32_t c)
{
	size_t size = sizeof (c);

	if (buffer -> size + size > buffer -> capacity) {
		if (BKByteBufferGrow (buffer, size) < 0) {
			return -1;
		}
	}

	* (uint32_t *) (buffer -> data + buffer -> size) = c;
	buffer -> size += size;

	return 0;
}

BKInt BKByteBufferAppendPtr (BKByteBuffer * buffer, void const * ptr, size_t size)
{
	if (buffer -> size + size > buffer -> capacity) {
		if (BKByteBufferGrow (buffer, size) < 0) {
			return -1;
		}
	}

	memcpy (buffer -> data + buffer -> size, ptr, size);
	buffer -> size += size;

	return 0;
}

void BKByteBufferEmpty (BKByteBuffer * buffer, BKInt keepData)
{
	if (keepData == 0) {
		if (buffer -> data) {
			free (buffer -> data);
			buffer -> data = NULL;
		}

		buffer -> capacity = 0;
	}

	buffer -> size = 0;
}
