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

#ifndef _BK_TK_CONTEXT_H_
#define _BK_TK_CONTEXT_H_

#include "BKTKBase.h"
#include "BKTKInterpreter.h"
#include "BKTKCompiler.h"

typedef struct BKTKGroup BKTKGroup;
typedef struct BKTKInstrument BKTKInstrument;
typedef struct BKTKWaveform BKTKWaveform;
typedef struct BKTKSample BKTKSample;
typedef struct BKTKTrack BKTKTrack;
typedef struct BKTKContext BKTKContext;
typedef struct BKTKObject BKTKObject;

struct BKTKObject
{
	BKObject   object;
	BKInt      index;
	BKTKOffset offset;
};

struct BKTKGroup
{
	BKTKObject   object;
	BKByteBuffer byteCode;
	BKSize       codeSize;
};

struct BKTKInstrument
{
	BKTKObject   object;
	BKInstrument instr;
	BKString     name;
};

struct BKTKWaveform
{
	BKTKObject object;
	BKData     data;
	BKString   name;
};

struct BKTKSample
{
	BKTKObject object;
	BKString   path;
	BKString   name;
	BKInt      pitch;
	BKInt      repeat;
	BKInt      range [2];
	BKInt      sustainRange [2];
	BKData     data;
};

struct BKTKTrack
{
	BKTKObject      object;
	BKArray         groups; // BKTKGroup
	BKByteBuffer    byteCode;
	BKDivider       divider;
	BKTKContext   * ctx;
	BKTrack         renderTrack;
	BKInt           waveform;
	BKTKInterpreter interpreter;
	BKByteBuffer    timingData;
	BKInt           lineno;
};

struct BKTKContext
{
	BKObject     object;
	BKContext  * renderContext;
	BKArray      instruments;  // BKTKInstrument
	BKArray      waveforms;    // BKTKWaveform
	BKArray      samples;      // BKTKSample; may contain shared BKData!
	BKArray      tracks;       // BKTKTrack
	BKString     loadPath;
	BKString     error;
	BKTKFileInfo info;
};

enum BKTKContextOption
{
	BKTKContextOptionTimingShift     = 16,
	BKTKContextOptionTimingDataSecs  = 1 << 16,
	BKTKContextOptionTimingDataTicks = 2 << 16, // next field is at 2
	BKTKContextOptionTimingDataMask  = 3 << 16,
};

/**
 * Initialize context
 */
extern BKInt BKTKContextInit (BKTKContext * ctx, BKUInt flags);

/**
 * Create context from compiler
 */
extern BKInt BKTKContextCreate (BKTKContext * ctx, BKTKCompiler * compiler);

/**
 * Attach to render context
 */
extern BKInt BKTKContextAttach (BKTKContext * ctx, BKContext * renderContext);

/**
 * Detach from render context
 */
extern void BKTKContextDetach (BKTKContext * ctx);

/**
 * Reset context
 */
extern void BKTKContextReset (BKTKContext * ctx);

/**
 * Allocate context objects
 */
extern BKInt BKTKTrackAlloc (BKTKTrack ** track);
extern BKInt BKTKGroupAlloc (BKTKGroup ** group);
extern BKInt BKTKInstrumentAlloc (BKTKInstrument ** instrument);
extern BKInt BKTKWaveformAlloc (BKTKWaveform ** waveform);
extern BKInt BKTKSampleAlloc (BKTKSample ** sample);

#endif /* ! _BK_TK_CONTEXT_H_ */
