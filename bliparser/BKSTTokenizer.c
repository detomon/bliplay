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

#include <sys/types.h>
#include "BKSTTokenizer.h"

#define INIT_BUFFER_SIZE 0x4000

/**
 * Base64 conversion table
 *
 * Converts a base64 character to its representing integer value.
 * '-' is the same as '+' and '_' is the same as '/'.
 * Invalid characters have the value -1.
 * Terminal characters (':', ';') which are used in the syntax have the value -2.
 * '=' also counts as terminal character to stop parsing.
 */
static char const base64Chars [256] =
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

BKInt BKSTTokenizerInit (BKSTTokenizer * tokenizer, char const * data, size_t dataSize)
{
	memset (tokenizer, 0, sizeof (*tokenizer));

	tokenizer -> readBufCapacity = INIT_BUFFER_SIZE;
	tokenizer -> readBuf = malloc (tokenizer -> readBufCapacity);

	if (tokenizer -> readBuf == NULL) {
		BKSTTokenizerDispose (tokenizer);
		return -1;
	}

	BKSTTokenizerSetData (tokenizer, data, dataSize);

	return 0;
}

BKInt BKSTTokenizerInitWithFile (BKSTTokenizer * tokenizer, FILE * file)
{
	if (BKSTTokenizerInit (tokenizer, NULL, 0) < 0) {
		return -1;
	}

	if (file == NULL) {
		return -1;
	}

	tokenizer -> file = file;

	return 0;
}

void BKSTTokenizerDispose (BKSTTokenizer * tokenizer)
{
	if (tokenizer -> readBuf) {
		free (tokenizer -> readBuf);
	}

	tokenizer -> prevChars [0] = -1;
	tokenizer -> prevChars [1] = -1;
	memset (tokenizer, 0, sizeof (*tokenizer));
}

static BKInt BKSTTokenizerReadBufferGrow (BKSTTokenizer * tokenizer)
{
	off_t bufOff;
	size_t newCapacity = tokenizer -> readBufCapacity * 2;
	uint8_t * newBuf = realloc (tokenizer -> readBuf, newCapacity);

	if (newBuf == NULL) {
		return -1;
	}

	bufOff = tokenizer -> readBufPtr - tokenizer -> readBuf;
	tokenizer -> readBufPtr = newBuf + bufOff;
	tokenizer -> readBuf = newBuf;
	tokenizer -> readBufCapacity = newCapacity;

	return 0;
}

static BKInt BKSTTokenizerReadBufferPutChar (BKSTTokenizer * tokenizer, int c)
{
	if (tokenizer -> readBufPtr >= tokenizer -> readBuf + tokenizer -> readBufCapacity) {
		if (BKSTTokenizerReadBufferGrow (tokenizer) < 0) {
			return -1;
		}
	}

	*tokenizer -> readBufPtr ++ = c;

	return 0;
}

static int BKSTTokenizerNextChar (BKSTTokenizer * tokenizer)
{
	int hasPrevChar = 0;
	int nextChar = -1;

	if (tokenizer -> prevChars [0] >= 0 && tokenizer -> prevChars [1] == -1) {
		nextChar = tokenizer -> prevChars [0];
		tokenizer -> prevChars [0] = -1;
		hasPrevChar = 1;
	}
	else if (tokenizer -> file) {
		nextChar = fgetc (tokenizer -> file);
	}
	else if (tokenizer -> dataPtr < tokenizer -> data + tokenizer -> dataSize) {
		nextChar = *tokenizer -> dataPtr ++;
	}

	tokenizer -> prevChars [0] = tokenizer -> prevChars [1];
	tokenizer -> prevChars [1] = nextChar;

	if (hasPrevChar == 0) {
		if (nextChar == '\n') {
			tokenizer -> lineno ++;
			tokenizer -> colno = 0;
		}
		else {
			tokenizer -> colno ++;
		}
	}

	return nextChar;
}

static int BKSTTokenizerPrevChar (BKSTTokenizer * tokenizer)
{
	int prevChars = -1;

	if (tokenizer -> prevChars [0] >= 0) {
		prevChars = tokenizer -> prevChars [0];
		tokenizer -> prevChars [0] = tokenizer -> prevChars [1];
		tokenizer -> prevChars [1] = -1;
	}

	return prevChars;
}

static int BKSTTokenizerReadComment (BKSTTokenizer * tokenizer)
{
	int c;
	int flag = 1;

	do {
		c = BKSTTokenizerNextChar (tokenizer);

		switch (c) {
			case '\n':
			case '\r': {
				flag = 0;
				break;
			}
			default: {
				if (BKSTTokenizerReadBufferPutChar (tokenizer, c) < 0) {
					return -1;
				}
				break;
			}
		}
	}
	while (flag);

	return 0;
}

static int BKSTTokenizerReadBase64 (BKSTTokenizer * tokenizer)
{
	int  c;
	char bc;
	unsigned value = 0;
	int      count = 0;

	do {
		c = BKSTTokenizerNextChar (tokenizer);

		// no more characters
		if (c == -1) {
			return -1;
		}

		bc = base64Chars [c];

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
			BKSTTokenizerReadBufferPutChar (tokenizer, (value >> 16) & 0xFF);
			BKSTTokenizerReadBufferPutChar (tokenizer, (value >>  8) & 0xFF);
			BKSTTokenizerReadBufferPutChar (tokenizer, (value >>  0) & 0xFF);
			value = 0;
			count = 0;
		}
	}
	while (c != -1);

	// padding not needed
	while (c == '=') {
		c = BKSTTokenizerNextChar (tokenizer);

		if (c == -1) {
			break;
		}
	}

	if (count) {
		value <<= 6 * (4 - count);

		switch (count) {
			case 1: { // actually an error
				// no break
			}
			case 2: {
				BKSTTokenizerReadBufferPutChar (tokenizer,  (value >> 16) & 0xFF);
				break;
			}
			case 3: {
				BKSTTokenizerReadBufferPutChar (tokenizer, (value >> 16) & 0xFF);
				BKSTTokenizerReadBufferPutChar (tokenizer, (value >>  8) & 0xFF);
				break;
			}
		}
	}

	BKSTTokenizerPrevChar (tokenizer);

	return 0;
}

BKSTTokenType BKSTTokenizerNextToken (BKSTTokenizer * tokenizer, BKSTToken * outToken)
{
	int c;
	int lineno, colno;
	uint8_t const * readPtr = tokenizer -> readBufPtr;
	BKSTTokenType token = BKSTTokenNone;

	outToken -> value = (void *) readPtr;

	do {
		lineno = tokenizer -> lineno;
		colno  = tokenizer -> colno;

		c = BKSTTokenizerNextChar (tokenizer);

		if (c == -1) {
			token = BKSTTokenEnd;
			break;
		}

		switch (c) {
			case '[': { // group begin
				token = BKSTTokenGrpBegin;
				break;
			}
			case ']': { // group end
				token = BKSTTokenGrpEnd;
				break;
			}
			case '%': { // comment
				token = BKSTTokenComment;

				if (BKSTTokenizerReadComment (tokenizer) < 0) {
					return -1;
				}
				break;
			}
			case ' ':
			case '\t':
			case '\v':
			case '\f':
			case '\xA0': { // space
				// ignore
				break;
			}
			case ':': { // argument separator
				token = BKSTTokenArgSep;
				break;
			}
			case ';':
			case '\n':
			case '\r': { // command separator
				token = BKSTTokenCmdSep;
				break;
			}
			case '!': { // base64 data
				token = BKSTTokenValue;

				if (BKSTTokenizerReadBase64 (tokenizer) < 0) {
					return -1;
				}

				break;
			}
			default: { // generic value
				BKInt flag = 1;
				token = BKSTTokenValue;

				do {
					switch (c) {
						case ':':
						case ';':
						case '\n':
						case '\r': { // end
							BKSTTokenizerPrevChar (tokenizer);
							flag = 0;
							break;
						}
						case '\0': {
							flag = 0;
							break;
						}
						default: {
							if (c == '\\') {
								c = BKSTTokenizerNextChar (tokenizer);

								if (c == '\0') {
									continue;
								}
							}

							if (BKSTTokenizerReadBufferPutChar (tokenizer, c) < 0) {
								return -1;
							}

							c = BKSTTokenizerNextChar (tokenizer);

							break;
						}
					}
				}
				while (flag);

				break;
			}
		}
	}
	while (token == BKSTTokenNone);

	outToken -> size = tokenizer -> readBufPtr - readPtr;
	outToken -> lineno = lineno + 1;
	outToken -> colno = colno + 1;
	BKSTTokenizerReadBufferPutChar (tokenizer, '\0');

	return token;
}

void BKSTTokenizerClearTokens (BKSTTokenizer * tokenizer)
{
	tokenizer -> readBufPtr = tokenizer -> readBuf;
}

void BKSTTokenizerSetData (BKSTTokenizer * tokenizer, char const * data, size_t dataSize)
{
	if (data == NULL) {
		dataSize = 0;
	}

	tokenizer -> prevChars [0] = -1;
	tokenizer -> prevChars [1] = -1;
	tokenizer -> data          = (uint8_t *) data;
	tokenizer -> dataPtr       = (uint8_t *) data;
	tokenizer -> dataSize      = dataSize;
	tokenizer -> readBufPtr    = tokenizer -> readBuf;
	tokenizer -> lineno        = 0;
	tokenizer -> colno         = 0;
}

void BKSTTokenizerSetFile (BKSTTokenizer * tokenizer, FILE * file)
{
	BKSTTokenizerSetData (tokenizer, NULL, 0);
	tokenizer -> file = file;
}
