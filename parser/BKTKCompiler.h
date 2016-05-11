/*
 * Copyright (c) 2012-2016 Simon Schoenenberger
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

#ifndef _BK_TK_COMPILER_H_
#define _BK_TK_COMPILER_H_

#include "BKTKBase.h"
#include "BKInstrument.h"
#include "BKTKInterpreter.h"
#include "BKTKParser.h"

/**
 * Globally used flags
 */
enum BKTKFlag
{
	BKTKFlagUsed      = 1 << 0,
	BKTKFlagAutoIndex = 1 << 1,
};

struct BKTKCompiler
{
	BKObject     object;
	BKHashTable  instruments;
	BKHashTable  waveforms;
	BKHashTable  samples;
	BKArray      tracks;
	BKString     auxString;
	BKString     error;
	BKInt        lineno;
	BKTKFileInfo info;
};

/**
 * Initialize compiler object
 */
extern BKInt BKTKCompilerInit (BKTKCompiler * compiler);

/**
 * Parse tree from BKParser
 */
extern BKInt BKTKCompilerCompile (BKTKCompiler * compiler, BKTKParserNode const * tree);

/**
 * Reset compiler to compile another node
 */
extern BKInt BKTKCompilerReset (BKTKCompiler * compiler);

#endif /* ! _BK_TK_COMPILER_H_ */
