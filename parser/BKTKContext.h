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
};

struct BKTKWaveform
{
	BKTKObject object;
	BKData     data;
};

struct BKTKSample
{
	BKTKObject object;
	BKString   path;
	BKInt      pitch;
	BKInt      repeat;
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
	BKObject    object;
	BKContext * renderContext;
	BKArray     instruments;  // BKTKInstrument
	BKArray     waveforms;    // BKTKWaveform
	BKArray     samples;      // BKTKSample
	BKArray     tracks;       // BKTKTrack
	BKString    loadPath;
	BKString    error;
	BKInt       tickRate;
	BKInt       stepTicks;
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
