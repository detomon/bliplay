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

#ifndef _BK_COMPILER2_H_
#define _BK_COMPILER2_H_

#include "BKSTParser.h"
#include "BKInterpreter.h"
#include "BKArray.h"
#include "BKByteBuffer.h"

typedef struct BKCompiler2     BKCompiler2;
typedef struct BKCompilerTrack BKCompilerTrack;

struct BKCompilerTrack
{
	BKUInt       flags;
	BKArray      cmdGroups;
	BKByteBuffer globalCmds;
};

struct BKCompiler2
{
	BKUInt          flags;
	BKArray         groupStack;
	BKArray         tracks;
	BKCompilerTrack globalTrack;
	BKInt           ignoreGroupLevel;
	BKArray         instruments;
	BKArray         waveforms;
	BKArray         samples;
	BKUInt          stepTicks;
};

/**
 * Initialize compiler
 */
extern BKInt BKCompiler2Init (BKCompiler2 * compiler);

/**
 * Dispose compiler
 */
extern void BKCompiler2Dispose (BKCompiler2 * compiler);

/**
 * Push command
 */
extern BKInt BKCompiler2PushCommand (BKCompiler2 * compiler, BKSTCmd const * cmd);

/**
 * Compile all commands from parser and terminate compiler
 *
 * No options are defined at the moment
 */
extern BKInt BKCompiler2Compile (BKCompiler2 * compiler, BKSTParser * parser, BKEnum options);

/**
 * Compile commands from parser
 *
 * No options are defined at the moment
 */
extern BKInt BKCompiler2Terminate (BKCompiler2 * compiler, BKEnum options);

/**
 * Reset compiler for compiling new data
 */
extern void BKCompiler2Reset (BKCompiler2 * compiler, BKInt keepData);

#endif /* ! _BK_COMPILER2_H_ */
