/**
 * Copyright (c) 2012-2015 Simon Schoenenberger
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

#ifndef _BK_INTERPRETER_H_
#define _BK_INTERPRETER_H_

#include "BKObject.h"
#include "BKTrack.h"
#include "BKArray.h"

#define BK_INTR_CUSTOM_WAVEFORM_FLAG (1 << 15)
#define BK_INTR_STACK_SIZE 16
#define BK_INTR_JUMP_STACK_SIZE 16
#define BK_INTR_MAX_EVENTS 8

typedef struct BKInterpreter BKInterpreter;
typedef struct BKTickEvent   BKTickEvent;
typedef struct BKLongJump    BKLongJump;
typedef enum BKInstruction   BKInstruction;

enum BKInstruction
{
	BKIntrArpeggio           = 0,
	BKIntrArpeggioSpeed      = 1,
	BKIntrAttack             = 2,
	BKIntrAttackTicks        = 3,
	BKIntrCall               = 4,
	BKIntrDutyCycle          = 5,
	BKIntrEffect             = 6,
	BKIntrEnd                = 7,
	BKIntrGroupDef           = 8,
	BKIntrGroupJump          = 9,
	BKIntrInstrument         = 10,
	BKIntrInstrumentDef      = 11,
	BKIntrJump               = 12,
	BKIntrMasterVolume       = 13,
	BKIntrMute               = 14,
	BKIntrMuteTicks          = 15,
	BKIntrPanning            = 16,
	BKIntrPhaseWrap          = 17,
	BKIntrPitch              = 18,
	BKIntrRelease            = 19,
	BKIntrReleaseTicks       = 20,
	BKIntrRepeat             = 21,
	BKIntrReturn             = 22,
	BKIntrSample             = 23,
	BKIntrSampleDef          = 24,
	BKIntrSampleRange        = 25,
	BKIntrSampleRepeat       = 26,
	BKIntrSampleSustainRange = 27,
	BKIntrSetRepeatStart     = 28,
	BKIntrStep               = 29,
	BKIntrStepTicks          = 30,
	BKIntrTicks              = 31,
	BKIntrTrackDef           = 32,
	BKIntrVolume             = 33,
	BKIntrWaveform           = 34,
	BKIntrWaveformDef        = 35,
	BKIntrLineNo             = 36,
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
	BKObject        object;
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
	BKInt           time;
	BKInt           lineTime;
	BKInt           lineno, lastLineno;
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
 * Reset interpreter
 */
extern void BKInterpreterReset (BKInterpreter * interpreter);

#endif /* !_BK_INTERPRETER_H_  */
