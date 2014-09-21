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

#include "BKBase.h"

typedef struct BKSTTokenizer BKSTTokenizer;
typedef enum   BKSTToken     BKSTToken;

struct BKSTTokenizer
{
	uint8_t * data;
	uint8_t * dataPtr;
	size_t    dataSize;
	uint8_t * readBuf;
	uint8_t * readBufPtr;
	size_t    readBufCapacity;
};

enum BKSTToken
{
	BKSTTokenEnd,
	BKSTTokenNone,
	BKSTTokenUnknown,
	BKSTTokenValue,
	BKSTTokenGrpBegin,
	BKSTTokenGrpEnd,
	BKSTTokenComment,
	BKSTTokenArgSep,
	BKSTTokenCmdSep,
};

/**
 * Initialize tokenizer object
 */
extern BKInt BKSTTokenizerInit (BKSTTokenizer * tokenizer, char const * data, size_t dataSize);

/**
 * Dispose tokenizer object
 */
extern void BKSTTokenizerDispose (BKSTTokenizer * tokenizer);

/**
 * Read next command and arguments
 */
extern BKSTToken BKSTTokenizerNextToken (BKSTTokenizer * tokenizer, uint8_t const ** outPtr, size_t * outSize);

/**
 * Reset tokenizer with new data
 *
 * Allocated memory is reused
 */
extern void BKSTTokenizerSetData (BKSTTokenizer * tokenizer, char const * data, size_t dataSize);

#endif /* ! _BKST_TOKENIZER_H_ */
