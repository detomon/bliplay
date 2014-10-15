/**
 * Copyright (c) 2012-2014 Simon Schoenenberger
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

#ifndef _BK_INTERPRETER_H_
#define _BK_INTERPRETER_H_

#include "BKBase.h"
#include "BKTrack.h"
#include "BKArray.h"

#define BK_INTR_CUSTOM_WAVEFOMR_FLAG (1 << 15)
#define BK_INTR_STACK_SIZE 16
#define BK_INTR_JUMP_STACK_SIZE 16
#define BK_INTR_MAX_EVENTS 8

typedef struct BKInterpreter BKInterpreter;
typedef struct BKTickEvent   BKTickEvent;
typedef struct BKLongJump    BKLongJump;
typedef enum BKInstruction   BKInstruction;

enum BKInstruction
{
	BKIntrArpeggio,
	BKIntrArpeggioSpeed,
	BKIntrAttack,
	BKIntrAttackTicks,
	BKIntrCall,
	BKIntrDutyCycle,
	BKIntrEffect,
	BKIntrEnd,
	BKIntrGroupDef,
	BKIntrGroupJump,
	BKIntrInstrument,
	BKIntrInstrumentDef,
	BKIntrJump,
	BKIntrMasterVolume,
	BKIntrMute,
	BKIntrMuteTicks,
	BKIntrPanning,
	BKIntrPhaseWrap,
	BKIntrPitch,
	BKIntrRelease,
	BKIntrReleaseTicks,
	BKIntrRepeat,
	BKIntrReturn,
	BKIntrSample,
	BKIntrSampleDef,
	BKIntrSampleRange,
	BKIntrSampleRepeat,
	BKIntrSetRepeatStart,
	BKIntrStep,
	BKIntrStepTicks,
	BKIntrTicks,
	BKIntrTrackDef,
	BKIntrVolume,
	BKIntrWaveform,
	BKIntrWaveformDef,
};

enum
{
	BKInterpreterFlagHasAttackEvent = 1 << 0,
	BKInterpreterFlagHasArpeggio    = 1 << 1,
	BKInterpreterFlagHasStopped     = 1 << 2,
	BKInterpreterFlagHasRepeated    = 1 << 3,
};

struct BKTickEvent
{
	BKInt event;
	BKInt ticks;
};

struct BKLongJump
{
	BKInt   ticks;
	BKInt   opcodeAddr;
	BKInt * stackPtr;
};

struct BKInterpreter
{
	BKUInt          flags;
	void          * opcode;
	void          * opcodePtr;
	BKInt           repeatStartAddr;
	BKInt           stack [BK_INTR_STACK_SIZE];
	BKInt         * stackPtr;
	BKInt         * stackEnd;
	//BKLongJump      jumpStack [BK_INTR_JUMP_STACK_SIZE];
	//BKLongJump    * jumpStackPtr;
	//BKLongJump    * jumpStackEnd;
	BKArray       * instruments;
	BKArray       * waveforms;
	BKArray       * samples;
	BKUInt          stepTickCount;
	BKUInt          numSteps;
	BKUInt          nextNoteIndex;
	BKInt           nextNotes [2];
	BKInt           nextArpeggio [1 + BK_MAX_ARPEGGIO];
	BKInt           numEvents;
	BKTickEvent     events [BK_INTR_MAX_EVENTS];
};

/**
 * Initialize interpreter
 */
extern BKInt BKInterpreterInit (BKInterpreter * interpreter);

/**
 * Apply commands to track
 *
 * Return 1 if more events are available otherwise 0
 * `outTicks` is set to number of ticks to next event
 */
extern BKInt BKInterpreterTrackAdvance (BKInterpreter * interpreter, BKTrack * track, BKInt * outTicks);

/**
 * Dispose interpreter
 */
extern void BKInterpreterDispose (BKInterpreter * interpreter);

/**
 * Reset interpreter
 */
extern void BKInterpreterReset (BKInterpreter * interpreter);

#endif /* !_BK_INTERPRETER_H_  */
