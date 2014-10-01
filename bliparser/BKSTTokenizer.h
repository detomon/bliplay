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

#ifndef _BKST_TOKENIZER_H_
#define _BKST_TOKENIZER_H_

#include <stdio.h>
#include "BKBase.h"

typedef enum   BKSTTokenType BKSTTokenType;
typedef struct BKSTTokenizer BKSTTokenizer;
typedef struct BKSTToken     BKSTToken;

enum BKSTTokenType
{
	BKSTTokenEnd,
	BKSTTokenNone,
	BKSTTokenValue,
	BKSTTokenGrpBegin,
	BKSTTokenGrpEnd,
	BKSTTokenComment,
	BKSTTokenArgSep,
	BKSTTokenCmdSep,
};

struct BKSTTokenizer
{
	uint8_t * data;
	uint8_t * dataPtr;
	BKSize    dataSize;
	FILE    * file;
	int       prevChars [2];
	uint8_t * readBuf;
	uint8_t * readBufPtr;
	BKSize    readBufCapacity;
	int       lineno;
	int       colno;
};

struct BKSTToken
{
	BKSTTokenType type;
	char const  * value;
	BKSize        size;
	int           lineno;
	int           colno;
};

/**
 * Initialize tokenizer object with data
 */
extern BKInt BKSTTokenizerInit (BKSTTokenizer * tokenizer, char const * data, BKSize dataSize);

/**
 * Initialize tokenizer object with file
 */
extern BKInt BKSTTokenizerInitWithFile (BKSTTokenizer * tokenizer, FILE * file);

/**
 * Dispose tokenizer object
 */
extern void BKSTTokenizerDispose (BKSTTokenizer * tokenizer);

/**
 * Read next token
 *
 * Previous tokens are kept until `BKSTTokenizerClearTokens` is called
 */
extern BKSTTokenType BKSTTokenizerNextToken (BKSTTokenizer * tokenizer, BKSTToken * outToken);

/**
 * Clear tokens buffer
 */
extern void BKSTTokenizerClearTokens (BKSTTokenizer * tokenizer);

/**
 * Reset tokenizer with new data
 *
 * Allocated memory is reused
 */
extern void BKSTTokenizerSetData (BKSTTokenizer * tokenizer, char const * data, BKSize dataSize);

/**
 * Reset tokenizer with new file
 *
 * The file cursor is not reset
 * Allocated memory is reused
 */
extern void BKSTTokenizerSetFile (BKSTTokenizer * tokenizer, FILE * file);

#endif /* ! _BKST_TOKENIZER_H_ */
