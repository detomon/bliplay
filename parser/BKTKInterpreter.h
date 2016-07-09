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

#ifndef _BK_TK_INTERPRETER_H_
#define _BK_TK_INTERPRETER_H_

#include "BKTKBase.h"
#include "BKInstrument.h"
#include "BKTrack.h"
#include "BKTKTokenizer.h"

#define BK_INTR_CUSTOM_WAVEFORM_FLAG (1 << 24)
#define BK_INTR_STACK_SIZE 16
#define BK_INTR_MAX_EVENTS 8
#define BK_INTR_STEP_TICKS 24

typedef struct BKTKInterpreter BKTKInterpreter;
typedef struct BKTKTickEvent BKTKTickEvent;
typedef struct BKTKStackItem BKTKStackItem;

enum BKInstruction
{
	BKIntrNoop               = 0,
	BKIntrArpeggio           = 1,
	BKIntrArpeggioSpeed      = 2,
	BKIntrAttack             = 3,
	BKIntrAttackTicks        = 4,
	BKIntrCall               = 5,
	BKIntrDutyCycle          = 6,
	BKIntrEffect             = 7,
	BKIntrEnd                = 8,
	BKIntrGroupDef           = 9,
	BKIntrGroupJump          = 10,
	BKIntrInstrument         = 11,
	BKIntrInstrumentDef      = 12,
	BKIntrJump               = 13,
	BKIntrMasterVolume       = 14,
	BKIntrMute               = 15,
	BKIntrMuteTicks          = 16,
	BKIntrPanning            = 17,
	BKIntrPhaseWrap          = 18,
	BKIntrPitch              = 19,
	BKIntrRelease            = 20,
	BKIntrReleaseTicks       = 21,
	BKIntrRepeat             = 22,
	BKIntrRepeatStart        = 23,
	BKIntrReturn             = 24,
	BKIntrSample             = 25,
	BKIntrSampleDef          = 26,
	BKIntrSampleRange        = 27,
	BKIntrSampleRepeat       = 28,
	BKIntrSampleSustainRange = 29,
	BKIntrStep               = 30,
	BKIntrStepTicks          = 31,
	BKIntrStepTicksTrack     = 32,
	BKIntrTickRate           = 33,
	BKIntrTicks              = 34,
	BKIntrTrackDef           = 35,
	BKIntrVolume             = 36,
	BKIntrWaveform           = 37,
	BKIntrWaveformDef        = 38,
	BKIntrLineNo             = 39,
	BKIntrPulseKernel        = 40,
};

enum BKTKInterpreterFlag
{
	BKTKInterpreterFlagHasAttackEvent = 1 << 0,
	BKTKInterpreterFlagHasArpeggio    = 1 << 1,
	BKTKInterpreterFlagHasStopped     = 1 << 2,
	BKTKInterpreterFlagHasRepeated    = 1 << 3,
};

enum BKTKGroupIndexType
{
	BKGroupIndexTypeLocal  = 0,
	BKGroupIndexTypeGlobal = 1,
	BKGroupIndexTypeTrack  = 2,
};

typedef union
{
	struct {
		unsigned   cmd:6;
		signed int arg1:26;
	} arg1;
	struct {
		unsigned   cmd:6;
		signed int arg1:13;
		signed int arg2:13;
	} arg2;
	struct {
		unsigned   cmd:6;
		unsigned   type:2;
		signed int idx1:12;
		signed int idx2:12;
	} grp;
	uint32_t value;
} BKInstrMask;

struct BKTKTickEvent
{
	BKInt event;
	BKInt ticks;
};

struct BKTKStackItem
{
	uintptr_t ptr;
	uint8_t   trackIdx;
};

struct BKTKInterpreter {
	BKObject        object;
	void          * opcode;
	void          * opcodePtr;
	uintptr_t       repeatStartAddr;
	BKTKStackItem   stack [BK_INTR_STACK_SIZE];
	BKTKStackItem * stackPtr;
	BKTKStackItem * stackEnd;
	BKUInt          stepTickCount;
	BKUInt          numSteps;
	BKUInt          nextNoteIndex;
	BKInt           nextNotes [2];
	BKInt           nextArpeggio [1 + BK_MAX_ARPEGGIO];
	BKInt           numEvents;
	BKTKTickEvent   events [BK_INTR_MAX_EVENTS];
	BKInt           time;
	BKInt           lineTime;
	BKInt           lineno;
};

/**
 * Initialize interpreter
 */
extern BKInt BKTKInterpreterInit (BKTKInterpreter * interpreter);

/**
 * Apply commands to track
 *
 * Return 1 if more events are available otherwise 0
 * `outTicks` is set to number of ticks to next event
 */
extern BKInt BKTKInterpreterAdvance (BKTKInterpreter * interpreter, BKTKTrack * ctx, BKInt * outTicks);

/**
 * Reset interpreter
 */
extern void BKTKInterpreterReset (BKTKInterpreter * interpreter);

#endif /* ! _BK_TK_INTERPRETER_H_ */
