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

#include "BKArray.h"
#include "BKByteBuffer.h"
#include "BKInterpreter.h"
#include "BKSTParser.h"
#include "BKString.h"

typedef struct BKCompiler     BKCompiler;
typedef struct BKCompilerTrack BKCompilerTrack;

struct BKCompilerTrack
{
	BKUInt       flags;
	BKArray      cmdGroups;
	BKByteBuffer globalCmds;
	BKEnum       waveform;
};

struct BKCompiler
{
	BKObject        object;
	BKArray         groupStack;
	BKArray         tracks;
	BKCompilerTrack globalTrack;
	BKInt           ignoreGroupLevel;
	BKArray         instruments;
	BKArray         waveforms;
	BKArray         samples;
	BKUInt          stepTicks;
	BKInstrument  * currentInstrument;
	BKData        * currentWaveform;
	BKData        * currentSample;
	BKString        loadPath;
};

/**
 * Initialize compiler
 */
extern BKInt BKCompilerInit (BKCompiler * compiler);

/**
 * Push command
 */
extern BKInt BKCompilerPushCommand (BKCompiler * compiler, BKSTCmd const * cmd);

/**
 * Compile all commands from parser and terminate compiler
 *
 * No options are defined at the moment
 */
extern BKInt BKCompilerCompile (BKCompiler * compiler, BKSTParser * parser, BKEnum options);

/**
 * Compile commands from parser
 *
 * No options are defined at the moment
 */
extern BKInt BKCompilerTerminate (BKCompiler * compiler, BKEnum options);

/**
 * Reset compiler for compiling new data
 *
 * The `loadPath` is not changed
 */
extern void BKCompilerReset (BKCompiler * compiler, BKInt keepData);

#endif /* ! _BK_COMPILER2_H_ */
