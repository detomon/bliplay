/**
 * Copyright (c) 2012-2014 Simon Schoenenberger
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

#include <ctype.h>
#include "BKBlipReader.h"

#define INIT_BUFFER_SIZE 0x10000
#define INIT_DATA_SIZE     0x200
#define INIT_ARGS_SIZE     0x400

/**
 * Base64 conversion table
 *
 * Converts a base64 character to its representing integer value.
 * '-' is the same as '+' and '_' is the same as '/'.
 * Invalid characters have the value -1.
 * Terminal characters (':', ';') which are used in the syntax have the value -2.
 * '=' also counts as terminal character to stop parsing.
 */
static char const BKReaderBase64Table [256] =
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -1, -2, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

BKInt BKBlipReaderInit (BKBlipReader * reader, char const * data, size_t dataSize)
{
	memset (reader, 0, sizeof (BKBlipReader));

	reader -> buffer = malloc (INIT_BUFFER_SIZE);

	if (reader -> buffer == NULL)
		return -1;

	reader -> argBuffer = malloc (INIT_ARGS_SIZE * sizeof (BKBlipArgument));

	if (reader -> argBuffer == NULL) {
		BKBlipReaderDispose (reader);
		return -1;
	}

	reader -> bufferPtr = reader -> buffer;
	reader -> argPtr    = reader -> argBuffer;

	BKBlipReaderReset (reader, data, dataSize);

	memset (reader -> argBuffer, 0, INIT_ARGS_SIZE * sizeof (BKBlipArgument));

	return 0;
}

void BKBlipReaderDispose (BKBlipReader * reader)
{
	if (reader -> buffer)
		free (reader -> buffer);

	if (reader -> argBuffer)
		free (reader -> argBuffer);

	memset (reader, 0, sizeof (BKBlipReader));
}

static int BKBlipReaderGetChar (BKBlipReader * reader)
{
	int c = -1;

	if (reader -> dataPtr < & reader -> data [reader -> dataSize])
		c = * reader -> dataPtr ++;

	return c;
}

static int BKBlipReaderGetCharTrimWhitespace (BKBlipReader * reader)
{
	int c;

	do {
		c = BKBlipReaderGetChar (reader);
	}
	while (c != -1 && isspace (c));

	return c;
}

static void BKBlipReaderRemapArgs (BKBlipReader * reader, unsigned char * oldBuffer, unsigned char * newBuffer)
{
	size_t offset;
	BKBlipArgument * arg;

	for (arg = reader -> argBuffer; arg < reader -> argPtr; arg ++) {
		offset = (void *) arg -> arg - (void *) oldBuffer;
		arg -> arg = (void *) newBuffer + offset;
	}
}

static BKInt BKBlipReaderResizeBuffer (BKBlipReader * reader, size_t newCapacity)
{
	unsigned char * newBuffer;

	if (newCapacity > reader -> bufferCapacity) {
		newBuffer = realloc (reader -> buffer, newCapacity);

		if (newBuffer) {
			BKBlipReaderRemapArgs (reader, reader -> buffer, newBuffer);

			reader -> bufferPtr      = & newBuffer [reader -> bufferPtr - reader -> buffer];
			reader -> buffer         = newBuffer;
			reader -> bufferCapacity = newCapacity;
		}
		else {
			return -1;
		}
	}

	return 0;
}

static BKInt BKBlipReaderBufferEnsureSpace (BKBlipReader * reader, size_t space)
{
	if (reader -> bufferPtr >= & reader -> buffer [reader -> bufferCapacity] - space) {
		if (BKBlipReaderResizeBuffer (reader, reader -> bufferCapacity * 2) < 0)
			return -1;
	}

	return 0;
}

static BKInt BKBlipReaderBufferPutChar (BKBlipReader * reader, int c)
{
	if (reader -> bufferPtr >= & reader -> buffer [reader -> bufferCapacity] - 1) {
		if (BKBlipReaderResizeBuffer (reader, reader -> bufferCapacity * 2) < 0)
			return -1;
	}

	* reader -> bufferPtr ++ = c;

	return 0;
}

static BKInt BKBlipReaderBufferPutChars (BKBlipReader * reader, char const * chars, size_t size)
{
	size_t newSize;
	size_t remaningSize = reader -> bufferCapacity - (reader -> bufferPtr - reader -> buffer);

	if (size > remaningSize) {
		newSize = BKMax (reader -> bufferCapacity * 2, reader -> bufferCapacity + size);

		if (BKBlipReaderResizeBuffer (reader, newSize) < 0)
			return -1;
	}

	memcpy (reader -> bufferPtr, chars, size);
	reader -> bufferPtr += size;

	return 0;
}

static BKInt BKBlipReaderReadBase64 (BKBlipReader * reader, BKBlipArgument * arg)
{
	int  c;
	char bc;
	unsigned value = 0;
	int      count = 0;

	do {
		c = BKBlipReaderGetChar (reader);

		if (c == -1) {
			return -1;
		}

		bc = BKReaderBase64Table [c];

		// ignore invalid char
		if (bc == -1) {
			continue;
		}
		// terminal character
		else if (bc == -2) {
			break;
		}

		value = (value << 6) + bc;

		if (++ count >= 4) {
			if (BKBlipReaderBufferEnsureSpace (reader, 3) < 0)
				return -1;

			* reader -> bufferPtr ++ = (value >> 16) & 0xFF;
			* reader -> bufferPtr ++ = (value >>  8) & 0xFF;
			* reader -> bufferPtr ++ = (value >>  0) & 0xFF;

			arg -> size += 3;
			value = 0;
			count = 0;
		}
	}
	while (c != -1);

	// padding not needed
	while (c == '=') {
		c = BKBlipReaderGetChar (reader);

		if (c == -1)
			return -1;
	}

	if (count) {
		value <<= 6 * (4 - count);

		switch (count) {
			case 1: // actually an error
				// no break
			case 2:
				if (BKBlipReaderBufferEnsureSpace (reader, 1) < 0)
					return -1;

				* reader -> bufferPtr ++ = (value >> 16) & 0xFF;
				arg -> size ++;

				break;
			case 3:
				if (BKBlipReaderBufferEnsureSpace (reader, 2) < 0)
					return -1;

				* reader -> bufferPtr ++ = (value >> 16) & 0xFF;
				* reader -> bufferPtr ++ = (value >>  8) & 0xFF;
				arg -> size += 2;

				break;
		}
	}

	switch (c) {
		case ':':
			return 1;
			break;
		case ';':
			return 0;
			break;
		default:
			return -1;
			break;
	}

	return -1;
}

static BKInt BKBlipReaderReadSingleArg (BKBlipReader * reader, BKBlipArgument * arg, int c)
{
	do {
		switch (c) {
			case '\\':
				c = BKBlipReaderGetChar (reader);
				break;
			case ':':
				return 1;
				break;
			case ';':
				return 0;
				break;
			case -1:
				return 0;
				break;
			default:
				break;
		}

		if (BKBlipReaderBufferPutChar (reader, c) < 0)
			return -1;

		arg -> size ++;

		c = BKBlipReaderGetChar (reader);
	}
	while (c != -1);

	return 0;
}

static BKInt BKBlipReaderSkipLine (BKBlipReader * reader)
{
	int c;

	do {
		c = BKBlipReaderGetChar (reader);

		if (c == '\n')
			break;
	}
	while (c != -1);

	return 0;
}

static BKInt BKBlipReaderReadArg (BKBlipReader * reader, BKBlipArgument * arg)
{
	int c;

	arg -> arg  = (char *) reader -> bufferPtr;
	arg -> size = 0;

	do {
		c = BKBlipReaderGetCharTrimWhitespace (reader);

		// comment line
		if (c == '#') {
			if (BKBlipReaderSkipLine (reader) == -1)
				return -1;

			continue;
		}
		// base64 encoded data
		else if (c == '!') {
			return BKBlipReaderReadBase64 (reader, arg);
		}
		// normal arg
		else {
			return BKBlipReaderReadSingleArg (reader, arg, c);
		}

		break;
	}
	while (c != -1);

	return 0;
}

static BKBlipArgument * BKBlipReaderPrepareArgument (BKBlipReader * reader)
{
	BKBlipArgument * arg;
	BKBlipArgument * newBuffer;
	size_t           newCapacity;

	if (reader -> argPtr >= & reader -> argBuffer [reader -> argBufferCapacity]) {
		newCapacity = reader -> argBufferCapacity * 2;
		newBuffer   = realloc (reader -> argBuffer, newCapacity * sizeof (BKBlipArgument));

		if (newBuffer) {
			reader -> argPtr            = & newBuffer [reader -> argPtr - reader -> argBuffer];
			reader -> argBuffer         = newBuffer;
			reader -> argBufferCapacity = newCapacity;
		}
		else {
			return NULL;
		}
	}

	arg = reader -> argPtr ++;
	memset (arg, 0, sizeof (BKBlipArgument));

	return arg;
}

BKInt BKBlipReaderNextCommand (BKBlipReader * reader, BKBlipCommand * command)
{
	int ret;
	BKBlipArgument * arg;

	memset (command, 0, sizeof (BKBlipCommand));

	do {
		 // clear previous args
		memset (reader -> argBuffer, 0, (reader -> argPtr - reader -> argBuffer) * sizeof (BKBlipArgument));

		reader -> bufferPtr = reader -> buffer;
		reader -> argPtr    = reader -> argBuffer;

		do {
			arg = BKBlipReaderPrepareArgument (reader);

			if (arg == NULL)
				return -1;

			ret = BKBlipReaderReadArg (reader, arg);

			if (ret >= 0) {
				if (BKBlipReaderBufferPutChar (reader, '\0') < 0)
					return -1;

				if (ret == 0) {
					if ((reader -> argPtr - reader -> argBuffer) == 1 && arg -> size == 0)
						return 0;
					break;
				}
				else if (ret == 1) {
					// more args
				}
			}
			else {
				return -1;
			}
		}
		while (1);
	}
	while (arg -> size == 0);

	command -> name     = reader -> argBuffer [0].arg;
	command -> nameSize = reader -> argBuffer [0].size;
	command -> args     = & reader -> argBuffer [1];
	command -> argCount = (reader -> argPtr - reader -> argBuffer) - 1;

	return 1;
}

void BKBlipReaderReset (BKBlipReader * reader, char const * data, size_t dataSize)
{
	if (data == NULL) {
		data     = "";
		dataSize = 0;
	}

	reader -> data     = (unsigned char *) data;
	reader -> dataPtr  = (unsigned char *) data;
	reader -> dataSize = dataSize;

	 // clear previous args
	if (reader -> argBuffer)
		memset (reader -> argBuffer, 0, (reader -> argPtr - reader -> argBuffer) * sizeof (BKBlipArgument));

	reader -> bufferPtr         = reader -> buffer;
	reader -> bufferCapacity    = INIT_BUFFER_SIZE;
	reader -> argPtr            = reader -> argBuffer;
	reader -> argBufferCapacity = INIT_ARGS_SIZE;
}
