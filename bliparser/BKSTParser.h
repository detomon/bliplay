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

#ifndef _BKST_PARSER_H_
#define _BKST_PARSER_H_

#include "BKObject.h"
#include "BKSTTokenizer.h"

typedef struct BKSTParser BKSTParser;
typedef struct BKSTArg    BKSTArg;
typedef struct BKSTCmd    BKSTCmd;

enum
{
	BKSTParserFlagGroup = 1 << 0,
};

struct BKSTParser
{
	BKObject      object;
	BKSTTokenizer tokenizer;
	BKSTArg     * argBuf;
	BKSTArg     * argBufPtr;
	BKSize        argBufCapacity;
};

struct BKSTArg
{
	char const * arg;
	BKSize       size;
};

struct BKSTCmd
{
	BKUInt          flags;
	BKSTTokenType   token;
	char const    * name;
	BKSize          nameSize;
	BKSTArg const * args;
	BKSize          numArgs;
	int             lineno;
	int             colno;
};

/**
 * Initialize parser object with data
 */
extern BKInt BKSTParserInit (BKSTParser * parser, char const * data, BKSize dataSize);

/**
 * Initialize parser object with file
 */
extern BKInt BKSTParserInitWithFile (BKSTParser * parser, FILE * file);

/**
 * Read next command and arguments
 */
extern BKSTTokenType BKSTParserNextCommand (BKSTParser * parser, BKSTCmd * outCmd);

/**
 * Reset parser with new data
 *
 * Allocated memory is reused
 */
extern void BKSTParserSetData (BKSTParser * parser, char const * data, BKSize dataSize);

/**
 * Reset parser with new file
 *
 * The file cursor is not reset
 * Allocated memory is reused
 */
extern void BKSTParserSetFile (BKSTParser * parser, FILE * file);

#endif /* ! _BKST_PARSER_H_ */
