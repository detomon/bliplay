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
#include "BKSTParser.h"

#define INIT_BUFFER_SIZE 0x4000

BKInt BKSTParserInit (BKSTParser * parser, char const * data, size_t dataSize)
{
	memset (parser, 0, sizeof (*parser));

	parser -> argBufCapacity = INIT_BUFFER_SIZE;

	parser -> argBuf = malloc (parser -> argBufCapacity);

	if (parser -> argBuf == NULL) {
		BKSTParserDispose (parser);
		return -1;
	}

	if (BKSTTokenizerInit (& parser -> tokenizer, data, dataSize)) {
		BKSTParserDispose (parser);
		return -1;
	}

	BKSTParserSetData (parser, data, dataSize);

	return 0;
}

void BKSTParserDispose (BKSTParser * parser)
{
	if (parser -> argBuf) {
		free (parser -> argBuf);
	}

	memset (parser, 0, sizeof (*parser));
}

static BKInt BKSTParserArgBufferGrow (BKSTParser * parser)
{
	off_t bufOff;
	size_t newCapacity = parser -> argBufCapacity * 2;
	BKSTArg * newBuf = realloc (parser -> argBuf, newCapacity);

	if (newBuf == NULL) {
		return -1;
	}

	bufOff = parser -> argBufPtr - parser -> argBuf;
	parser -> argBufPtr = newBuf + bufOff;
	parser -> argBuf = newBuf;
	parser -> argBufCapacity = newCapacity;

	return 0;
}

static BKInt BKSTParserArgPush (BKSTParser * parser, char const * arg, size_t size)
{
	if ((void *) parser -> argBufPtr >= (void *) parser -> argBuf + parser -> argBufCapacity) {
		if (BKSTParserArgBufferGrow (parser) < 0) {
			return -1;
		}
	}

	parser -> argBufPtr -> arg  = arg;
	parser -> argBufPtr -> size = size;
	parser -> argBufPtr ++;

	return 0;
}

static void BKSTParserArgsClear (BKSTParser * parser)
{
	BKSTTokenizerClearTokens (& parser -> tokenizer);

	memset (parser -> argBuf, 0, sizeof (BKSTArg) * (parser -> argBufPtr - parser -> argBuf));
	parser -> argBufPtr = parser -> argBuf;
}

BKSTToken BKSTParserNextCommand (BKSTParser * parser, BKSTCmd * outCmd)
{
	int flag = 1;
	BKSTToken token = BKSTTokenEnd;
	uint8_t const * arg;
	size_t size;

	memset (outCmd, 0, sizeof (*outCmd));

	BKSTParserArgsClear (parser);

	outCmd -> args = parser -> argBuf;

	do  {
		token = BKSTTokenizerNextToken (& parser -> tokenizer, & arg, & size);

		if ((int) token == -1) {
			return -1;
		}

		switch (token) {
			case BKSTTokenCmdSep: {
				if (parser -> argBufPtr > parser -> argBuf) {
					flag = 0;
				}
				break;
			}
			case BKSTTokenEnd: {
				flag = 0;
				break;
			}
			case BKSTTokenValue: {
				if (BKSTParserArgPush (parser, (void *) arg, size) < 0) {
					return -1;
				}
				break;
			}
			case BKSTTokenGrpBegin: {
				flag = 0;
				break;
			}
			case BKSTTokenGrpEnd: {
				flag = 0;
				break;
			}
			case BKSTTokenArgSep:
			case BKSTTokenNone:
			case BKSTTokenUnknown:
			case BKSTTokenComment: {
				break;
			}
		}
	}
	while (flag);

	outCmd -> numArgs = parser -> argBufPtr - parser -> argBuf;

	if (outCmd -> numArgs) {
		outCmd -> name     = outCmd -> args [0].arg;
		outCmd -> nameSize = outCmd -> args [0].size;
		outCmd -> args ++;
		outCmd -> numArgs --;
	}

	return token;
}

void BKSTParserSetData (BKSTParser * parser, char const * data, size_t dataSize)
{
	if (data == NULL) {
		dataSize = 0;
	}

	BKSTTokenizerSetData (& parser -> tokenizer, data, dataSize);

	parser -> argBufPtr = parser -> argBuf;
}
