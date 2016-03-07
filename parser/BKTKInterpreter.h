#ifndef _BK_TK_INTERPRETER_H_
#define _BK_TK_INTERPRETER_H_

#include "BKTKBase.h"
#include "BKInstrument.h"
#include "BKTrack.h"
#include "BKTKTokenizer.h"

#define BK_INTR_CUSTOM_WAVEFORM_FLAG (1 << 15)
#define BK_INTR_STACK_SIZE 16
#define BK_INTR_MAX_EVENTS 8

typedef struct BKTKInterpreter BKTKInterpreter;
typedef struct BKTickEvent BKTickEvent;

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
} BKInstrMask;

struct BKTickEvent
{
	BKInt event;
	BKInt ticks;
};

struct BKTKInterpreter {
	BKObject    object;
	void      * opcode;
	void      * opcodePtr;
	uintptr_t   repeatStartAddr;
	uintptr_t   stack [BK_INTR_STACK_SIZE];
	uintptr_t * stackPtr;
	uintptr_t * stackEnd;
	BKUInt      stepTickCount;
	BKUInt      numSteps;
	BKUInt      nextNoteIndex;
	BKInt       nextNotes [2];
	BKInt       nextArpeggio [1 + BK_MAX_ARPEGGIO];
	BKInt       numEvents;
	BKTickEvent events [BK_INTR_MAX_EVENTS];
	BKInt       time;
	BKInt       lineTime;
	BKInt       lineno, lastLineno;
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
