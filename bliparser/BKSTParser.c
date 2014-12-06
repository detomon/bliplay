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

#define INIT_BUFFER_SIZE 0x1000

extern BKClass BKSTParserClass;

static BKInt BKSTParserInitGeneric (BKSTParser * parser)
{
	if (BKObjectInit (parser, & BKSTParserClass, sizeof (*parser)) < 0) {
		return -1;
	}

	parser -> argBufCapacity = INIT_BUFFER_SIZE;
	parser -> argBuf = malloc (parser -> argBufCapacity);

	if (parser -> argBuf == NULL) {
		BKDispose (parser);
		return -1;
	}

	memset (parser -> argBuf, 0, parser -> argBufCapacity);
	parser -> argBufPtr = parser -> argBuf;

	return 0;
}

BKInt BKSTParserInit (BKSTParser * parser, char const * data, BKSize dataSize)
{
	if (BKSTParserInitGeneric (parser) < 0) {
		BKDispose (parser);
		return -1;
	}

	if (BKSTTokenizerInit (& parser -> tokenizer, data, dataSize)) {
		BKDispose (parser);
		return -1;
	}

	BKSTParserSetData (parser, data, dataSize);

	return 0;
}

BKInt BKSTParserInitWithFile (BKSTParser * parser, FILE * file)
{
	if (BKSTParserInitGeneric (parser) < 0) {
		BKDispose (parser);
		return -1;
	}

	if (BKSTTokenizerInitWithFile (& parser -> tokenizer, file)) {
		BKDispose (parser);
		return -1;
	}

	BKSTParserSetData (parser, NULL, 0);

	return 0;
}

static void BKSTParserDispose (BKSTParser * parser)
{
	if (parser -> argBuf) {
		free (parser -> argBuf);
	}
}

static BKInt BKSTParserArgBufferGrow (BKSTParser * parser)
{
	off_t bufOff;
	BKSize newCapacity = parser -> argBufCapacity * 2;
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

static BKInt BKSTParserArgPush (BKSTParser * parser, char const * arg, BKSize size)
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

BKSTTokenType BKSTParserNextCommand (BKSTParser * parser, BKSTCmd * outCmd)
{
	BKInt flag = 1, flags = 0;
	BKInt lineno = 0, colno = 0;
	BKSTToken token;
	BKSTTokenType type = BKSTTokenEnd;

	memset (outCmd, 0, sizeof (*outCmd));
	BKSTParserArgsClear (parser);

	outCmd -> args = parser -> argBuf;

	do  {
		type = BKSTTokenizerNextToken (& parser -> tokenizer, & token);

		if ((int) type == -1) {
			return -1;
		}

		switch (type) {
			case BKSTTokenEnd: {
				if (parser -> argBufPtr > parser -> argBuf) {
					type = BKSTTokenValue;
				}
				flag = 0;
				break;
			}
			case BKSTTokenGrpBegin: {
				flags |= BKSTParserFlagGroup;
				break;
			}
			case BKSTTokenGrpEnd: {
				lineno = token.lineno;
				colno  = token.colno;
				flag   = 0;
				break;
			}
			case BKSTTokenCmdSep: {
				if (parser -> argBufPtr > parser -> argBuf) {
					type = BKSTTokenValue;
					flag = 0;
				}
				break;
			}
			case BKSTTokenValue: {
				if (parser -> argBufPtr == parser -> argBuf) {
					lineno = token.lineno;
					colno  = token.colno;
				}

				if (BKSTParserArgPush (parser, (void *) token.value, token.size) < 0) {
					return -1;
				}
				break;
			}
			case BKSTTokenArgSep:
			case BKSTTokenNone:
			case BKSTTokenComment: {
				break;
			}
		}
	}
	while (flag);

	if (flags & BKSTParserFlagGroup) {
		type = BKSTTokenGrpBegin;
	}

	outCmd -> flags   = flags;
	outCmd -> token   = type;
	outCmd -> numArgs = parser -> argBufPtr - parser -> argBuf;
	outCmd -> lineno  = lineno;
	outCmd -> colno   = colno;

	if (outCmd -> numArgs) {
		outCmd -> name     = outCmd -> args [0].arg;
		outCmd -> nameSize = outCmd -> args [0].size;
		outCmd -> args ++;
		outCmd -> numArgs --;
	}

	return type;
}

void BKSTParserSetData (BKSTParser * parser, char const * data, BKSize dataSize)
{
	if (data == NULL) {
		dataSize = 0;
	}

	BKSTTokenizerSetData (& parser -> tokenizer, data, dataSize);
	BKSTParserArgsClear (parser);
}

void BKSTParserSetFile (BKSTParser * parser, FILE * file)
{
	BKSTParserSetData (parser, NULL, 0);
	BKSTTokenizerSetFile (& parser -> tokenizer, file);
}

BKClass BKSTParserClass =
{
	.instanceSize = sizeof (BKSTParser),
	.dispose      = (BKDisposeFunc) BKSTParserDispose,
};
