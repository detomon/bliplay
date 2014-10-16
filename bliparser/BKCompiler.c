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

#include <stdio.h>
#include <unistd.h>
#include "BKTone.h"
#include "BKCompiler.h"

#define MAX_GROUPS 255
#define VOLUME_UNIT (BK_MAX_VOLUME / 255)
#define PITCH_UNIT (BK_FINT20_UNIT / 100)

#define BK_MAX_SEQ_LENGTH 256
#define BK_MAX_WAVEFORM_LENGTH 64
#define BK_MAX_PATH 2048

enum BKCompilerFlag
{
	BKCompilerFlagAllocated = 1 << 0,
	BKCompilerFlagOpenGroup = 1 << 1,
	BKCompilerFlagArpeggio  = 1 << 2,
};

enum BKCompilerEnvelopeType
{
	BKCompilerEnvelopeTypeVolumeSeq,
	BKCompilerEnvelopeTypePitchSeq,
	BKCompilerEnvelopeTypePanningSeq,
	BKCompilerEnvelopeTypeDutyCycleSeq,
	BKCompilerEnvelopeTypeADSR,
	BKCompilerEnvelopeTypeVolumeEnv,
	BKCompilerEnvelopeTypePitchEnv,
	BKCompilerEnvelopeTypePanningEnv,
	BKCompilerEnvelopeTypeDutyCycleEnv,
};

enum BKCompilerMiscCmds
{
	BKCompilerMiscLoad,
	BKCompilerMiscRaw,
	BKCompilerMiscPitch,
};

/**
 * Used for lookup tables to assign a string to a value
 */
typedef struct
{
	char * name;
	BKInt  value;
	BKUInt flags;
} const strval;

/**
 * Group stack item
 */
typedef struct {
	BKUInt            level;
	BKInstruction     groupType;
	BKCompilerTrack * track;
	BKByteBuffer    * cmdBuffer;
} BKCompilerGroup;

/**
 * Note lookup table
 */
static strval noteNames [] =
{
	{"a",  9},
	{"a#", 10},
	{"b",  11},
	{"c",  0},
	{"c#", 1},
	{"d",  2},
	{"d#", 3},
	{"e",  4},
	{"f",  5},
	{"f#", 6},
	{"g",  7},
	{"g#", 8},
	{"h",  11},
};

/**
 * Command lookup table
 */
static strval cmdNames [] =
{
	{"a",         BKIntrAttack},
	{"as",        BKIntrArpeggioSpeed},
	{"at",        BKIntrAttackTicks},
	{"d",         BKIntrSample},
	{"dc",        BKIntrDutyCycle},
	{"dn",        BKIntrSampleRange},
	{"dr",        BKIntrSampleRepeat},
	{"e",         BKIntrEffect},
	{"g",         BKIntrGroupJump},
	{"grp",       BKIntrGroupDef, BKCompilerFlagOpenGroup},
	{"i",         BKIntrInstrument},
	{"instr",     BKIntrInstrumentDef, BKCompilerFlagOpenGroup},
	{"m",         BKIntrMute},
	{"mt",        BKIntrMuteTicks},
	{"p",         BKIntrPanning},
	{"pt",        BKIntrPitch},
	{"pw",        BKIntrPhaseWrap},
	{"r",         BKIntrRelease},
	{"rt",        BKIntrReleaseTicks},
	{"s",         BKIntrStep},
	{"samp",      BKIntrSampleDef},
	{"st",        BKIntrStepTicks},
	{"stepticks", BKIntrStepTicks},
	{"t",         BKIntrTicks},
	{"track",     BKIntrTrackDef, BKCompilerFlagOpenGroup},
	{"v",         BKIntrVolume},
	{"vm",        BKIntrMasterVolume},
	{"w",         BKIntrWaveform},
	{"wave",      BKIntrWaveformDef, BKCompilerFlagOpenGroup},
	{"x",         BKIntrRepeat},
	{"xb",        BKIntrSetRepeatStart},
	{"z",         BKIntrEnd},
};

/**
 * Effect lookup table
 */
static strval effectNames [] =
{
	{"pr", BK_EFFECT_PORTAMENTO},
	{"ps", BK_EFFECT_PANNING_SLIDE},
	{"tr", BK_EFFECT_TREMOLO},
	{"vb", BK_EFFECT_VIBRATO},
	{"vs", BK_EFFECT_VOLUME_SLIDE},
};

/**
 * Waveform lookup table
 */
static strval waveformNames [] =
{
	{"noise",    BK_NOISE},
	{"sawtooth", BK_SAWTOOTH},
	{"sine",     BK_SINE},
	{"square",   BK_SQUARE},
	{"triangle", BK_TRIANGLE},
};

/**
 * Envelope lookup table
 */
static strval envelopeNames [] =
{
	{"a",    BKCompilerEnvelopeTypePitchSeq},
	{"adsr", BKCompilerEnvelopeTypeADSR},
	{"anv",  BKCompilerEnvelopeTypePitchEnv},
	{"dc",   BKCompilerEnvelopeTypeDutyCycleSeq},
	{"dcnv", BKCompilerEnvelopeTypeDutyCycleEnv},
	{"p",    BKCompilerEnvelopeTypePanningSeq},
	{"pnv",  BKCompilerEnvelopeTypePanningEnv},
	{"v",    BKCompilerEnvelopeTypeVolumeSeq},
	{"vnv",  BKCompilerEnvelopeTypeVolumeEnv},
};

/**
 * Miscellaneous lookup table
 */
static strval miscNames [] =
{
	{"load", BKCompilerMiscLoad},
	{"pt",   BKCompilerMiscPitch},
	{"raw",  BKCompilerMiscRaw},
};

#define NUM_WAVEFORM_NAMES (sizeof (waveformNames) / sizeof (strval))
#define NUM_CMD_NAMES (sizeof (cmdNames) / sizeof (strval))
#define NUM_EFFECT_NAMES (sizeof (effectNames) / sizeof (strval))
#define NUM_NOTE_NAMES (sizeof (noteNames) / sizeof (strval))
#define NUM_ENVELOPE_NAMES (sizeof (envelopeNames) / sizeof (strval))
#define NUM_MISC_NAMES (sizeof (miscNames) / sizeof (strval))

/**
 * Convert string to signed integer like `atoi`
 * Returns alternative value if string is NULL
 */
static int atoix (char const * str, int alt)
{
	return str ? atoi (str) : alt;
}

/**
 * Compare two strings like `strcmp`
 * Returns -1 is if one of the strings is NULL
 */
static int strcmpx (char const * a, char const * b)
{
	return (a && b) ? strcmp (a, b) : -1;
}

/**
 * Compare name of command with string
 * Used as callback for `bsearch`
 */
static int strvalcmp (char const * name, strval const * item)
{
	return strcmp (name, item -> name);
}

static BKInt BKCompilerTrackInit (BKCompilerTrack * track)
{
	memset (track, 0, sizeof (* track));

	if (BKArrayInit (& track -> cmdGroups, sizeof (BKByteBuffer *), 0) < 0) {
		return -1;
	}

	if (BKByteBufferInit (& track -> globalCmds, 0) < 0) {
		return -1;
	}

	return 0;
}

static BKInt BKCompilerTrackAlloc (BKCompilerTrack ** outTrack)
{
	BKInt res;
	BKCompilerTrack * track = malloc (sizeof (* track));

	* outTrack = NULL;

	if (track == NULL) {
		return -1;
	}

	res = BKCompilerTrackInit (track);

	if (res != 0) {
		free (track);
		return res;
	}

	track -> flags |= BKCompilerFlagAllocated;
	* outTrack = track;

	return 0;
}

static void BKCompilerTrackDispose (BKCompilerTrack * track)
{
	BKUInt flags;
	BKByteBuffer * buffer;

	if (track == NULL) {
		return;
	}

	for (BKInt i = 0; i < track -> cmdGroups.length; i ++) {
		BKArrayGetItemAtIndexCopy (& track -> cmdGroups, i, & buffer);
		BKByteBufferDispose (buffer);
	}

	BKArrayDispose (& track -> cmdGroups);
	BKByteBufferDispose (& track -> globalCmds);

	flags = track -> flags;
	memset (track, 0, sizeof (* track));

	if (flags) {
		free (track);
	}
}

static void BKCompilerTrackClear (BKCompilerTrack * track, BKInt keepData)
{
	BKArrayEmpty (& track -> cmdGroups, keepData);
	BKByteBufferEmpty (& track -> globalCmds, keepData);
}

BKInt BKCompilerInit (BKCompiler * compiler)
{
	memset (compiler, 0, sizeof (* compiler));

	if (BKArrayInit (& compiler -> groupStack, sizeof (BKCompilerGroup), 8) < 0) {
		BKCompilerDispose (compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> tracks, sizeof (BKCompilerTrack *), 8) < 0) {
		BKCompilerDispose (compiler);
		return -1;
	}

	if (BKCompilerTrackInit (& compiler -> globalTrack) < 0) {
		BKCompilerDispose (compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> instruments, sizeof (BKInstrument *), 4) < 0) {
		BKCompilerDispose (compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> waveforms, sizeof (BKData *), 4) < 0) {
		BKCompilerDispose (compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> samples, sizeof (BKData *), 4) < 0) {
		BKCompilerDispose (compiler);
		return -1;
	}

	BKCompilerReset (compiler, 0);

	return 0;
}

void BKCompilerDispose (BKCompiler * compiler)
{
	BKCompilerReset (compiler, 0);

	if (compiler -> loadPath) {
		free (compiler -> loadPath);
	}

	memset (compiler, 0, sizeof (* compiler));
}

static BKInt BKCompilerStrvalTableLookup (strval table [], BKSize size, char const * name, BKInt * outValue, BKUInt * outFlags)
{
	strval * item = bsearch (name, table, size, sizeof (strval), (void *) strvalcmp);

	if (item == NULL) {
		return 0;
	}

	* outValue = item -> value;

	if (outFlags) {
		* outFlags = item -> flags;
	}

	return 1;
}

/**
 * Get note value for note string
 *
 * note octave [+-pitch]
 * Examples: c#3, e2+56, a#2-26
 */
static BKInt BKCompilerParseNote (char const * name)
{
	char  note [3];
	BKInt value  = 0;
	BKInt octave = 0;
	BKInt pitch  = 0;

	strcpy (note, "");  // empty name
	sscanf (name, "%2[a-z#]%u%d", note, & octave, & pitch);  // scan string; d#3[+-p] => "d#", 3, p

	if (BKCompilerStrvalTableLookup (noteNames, NUM_NOTE_NAMES, note, & value, NULL)) {
		value += octave * 12;
		value = BKClamp (value, BK_MIN_NOTE, BK_MAX_NOTE);
		value = (value << BK_FINT20_SHIFT) + pitch * PITCH_UNIT;
	}

	return value;
}

static BKByteBuffer * BKCompilerGetCmdGroupForIndex (BKCompiler * compiler, BKInt index)
{
	BKByteBuffer    * buffer = NULL;
	BKCompilerGroup * group  = BKArrayGetLastItem (& compiler -> groupStack);
	BKCompilerTrack * track  = group -> track;

	// search for free slot
	if (index == -1) {
		for (BKInt i = 0; i < track -> cmdGroups.length; i ++) {
			BKArrayGetItemAtIndexCopy (& track -> cmdGroups, i, & buffer);

			// is empty slot
			if (buffer == NULL) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			index  = track -> cmdGroups.length;
			buffer = NULL;
		}
	}
	else {
		// search buffer at slot
		BKArrayGetItemAtIndexCopy (& track -> cmdGroups, index, & buffer);

		// overwrite existing buffer
		if (buffer != NULL) {
			BKByteBufferEmpty (buffer, 1);
		}
	}

	// create new buffer
	if (buffer == NULL) {
		// fill slots with empty buffers
		while (track -> cmdGroups.length < index) {
			if (BKArrayPushPtr (& track -> cmdGroups) == NULL) {
				return NULL;
			}
		}

		if (BKByteBufferAlloc (& buffer, 0) < 0) {
			return NULL;
		}

		// append new buffer
		if (BKArrayPush (& track -> cmdGroups, & buffer) < 0) {
			return NULL;
		}
	}

	return buffer;
}

static BKInt BKInstrumentAlloc (BKInstrument ** outInstrument)
{
	BKInstrument * instrument;

	instrument = malloc (sizeof (* instrument));

	if (instrument == NULL) {
		return -1;
	}

	if (BKInstrumentInit (instrument) < 0) {
		free (instrument);
		return -1;
	}

	* outInstrument = instrument;

	return 0;
}

static void BKInstrumentFree (BKInstrument * instrument)
{
	if (instrument) {
		free (instrument);
	}
}

static BKInt BKDataAlloc (BKData ** outData)
{
	BKData * data;

	data = malloc (sizeof (* data));

	if (data == NULL) {
		return -1;
	}

	if (BKDataInit (data) < 0) {
		free (data);
		return -1;
	}

	* outData = data;

	return 0;
}

static void BKDataFree (BKData * data)
{
	if (data) {
		free (data);
	}
}

static BKInstrument * BKCompilerGetInstrumentForIndex (BKCompiler * compiler, BKInt index)
{
	BKInstrument * instrument = NULL;

	// search for free slot
	if (index == -1) {
		for (BKInt i = 0; i < compiler -> instruments.length; i ++) {
			BKArrayGetItemAtIndexCopy (& compiler -> instruments, i, & instrument);

			// is empty slot
			if (instrument == NULL) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			index = compiler -> instruments.length;
			instrument = NULL;
		}
	}
	else {
		// search item at slot
		BKArrayGetItemAtIndexCopy (& compiler -> instruments, index, & instrument);
	}

	// create new item
	if (instrument == NULL) {
		// fill slots with empty buffers
		while (compiler -> instruments.length < index) {
			if (BKArrayPushPtr (& compiler -> instruments) == NULL) {
				return NULL;
			}
		}

		if (BKInstrumentAlloc (& instrument) < 0) {
			return NULL;
		}

		// append new item
		if (BKArrayPush (& compiler -> instruments, & instrument) < 0) {
			BKInstrumentFree (instrument);
			return NULL;
		}
	}

	return instrument;
}

static BKData * BKCompilerGetWaveformForIndex (BKCompiler * compiler, BKInt index)
{
	BKData * data = NULL;

	// search for free slot
	if (index == -1) {
		for (BKInt i = 0; i < compiler -> waveforms.length; i ++) {
			BKArrayGetItemAtIndexCopy (& compiler -> waveforms, i, & data);

			// is empty slot
			if (data == NULL) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			index = compiler -> waveforms.length;
			data = NULL;
		}
	}
	else {
		// search item at slot
		BKArrayGetItemAtIndexCopy (& compiler -> waveforms, index, & data);
	}

	// create new item
	if (data == NULL) {
		// fill slots with empty buffers
		while (compiler -> waveforms.length < index) {
			if (BKArrayPushPtr (& compiler -> waveforms) == NULL) {
				return NULL;
			}
		}

		if (BKDataAlloc (& data) < 0) {
			return NULL;
		}

		// append new instrument
		if (BKArrayPush (& compiler -> waveforms, & data) < 0) {
			BKDataFree (data);
			return NULL;
		}
	}

	return data;
}

static BKData * BKCompilerGetSampleForIndex (BKCompiler * compiler, BKInt index)
{
	BKData * data = NULL;

	// search for free slot
	if (index == -1) {
		for (BKInt i = 0; i < compiler -> samples.length; i ++) {
			BKArrayGetItemAtIndexCopy (& compiler -> samples, i, & data);

			// is empty slot
			if (data == NULL) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			index = compiler -> samples.length;
			data = NULL;
		}
	}
	else {
		// search item at slot
		BKArrayGetItemAtIndexCopy (& compiler -> samples, index, & data);
	}

	// create new item
	if (data == NULL) {
		// fill slots with empty buffers
		while (compiler -> samples.length < index) {
			if (BKArrayPushPtr (& compiler -> samples) == NULL) {
				return NULL;
			}
		}

		if (BKDataAlloc (& data) < 0) {
			return NULL;
		}

		// append new instrument
		if (BKArrayPush (& compiler -> samples, & data) < 0) {
			BKDataFree (data);
			return NULL;
		}
	}

	return data;
}

static BKInt BKCompilerInstrumentSequenceParse (BKSTCmd const * cmd, BKInt * sequence, BKInt * outRepeatBegin, BKInt * outRepeatLength, BKInt multiplier)
{
	BKInt length = (BKInt) cmd -> numArgs - 2;
	BKInt repeatBegin = 0, repeatLength = 0;

	length       = BKMin (length, BK_MAX_SEQ_LENGTH);
	repeatBegin  = atoix (cmd -> args [0].arg, 0);
	repeatLength = atoix (cmd -> args [1].arg, 1);

	if (repeatBegin > length) {
		repeatBegin = length;
	}

	if (repeatBegin + repeatLength > length) {
		repeatLength = length - repeatBegin;
	}

	* outRepeatBegin  = repeatBegin;
	* outRepeatLength = repeatLength;

	for (BKInt i = 0; i < length; i ++) {
		sequence [i] = atoix (cmd -> args [i + 2].arg, 0) * multiplier;
	}

	return length;
}

static BKInt BKCompilerInstrumentEnvelopeParse (BKSTCmd const * cmd, BKSequencePhase * phases, BKInt * outRepeatBegin, BKInt * outRepeatLength, BKInt multiplier)
{
	BKInt length = (BKInt) cmd -> numArgs - 2;
	BKInt repeatBegin = 0, repeatLength = 0;

	length       = BKMin (length, BK_MAX_SEQ_LENGTH);
	repeatBegin  = atoix (cmd -> args [0].arg, 0);
	repeatLength = atoix (cmd -> args [1].arg, 1);

	if (repeatBegin > length) {
		repeatBegin = length;
	}

	if (repeatBegin + repeatLength > length) {
		repeatLength = length - repeatBegin;
	}

	* outRepeatBegin  = repeatBegin;
	* outRepeatLength = repeatLength;

	for (BKInt i = 0, j = 0; i < length; i += 2, j ++) {
		phases [j].steps = atoix (cmd -> args [i + 2].arg, 0);
		phases [j].value = atoix (cmd -> args [i + 3].arg, 0) * multiplier;
	}

	return length / 2;
}

static BKInt BKCompilerPushCommandInstrument (BKCompiler * compiler, BKSTCmd const * cmd)
{
	BKInt value;
	BKInt sequence [BK_MAX_SEQ_LENGTH];
	BKSequencePhase phases [BK_MAX_SEQ_LENGTH];
	BKInt sequenceLength, repeatBegin, repeatLength;
	BKInt asdr [4];
	BKInt res = 0;
	BKInstrument * instrument = compiler -> currentInstrument;

	// search for envelope type
	if (BKCompilerStrvalTableLookup (envelopeNames, NUM_ENVELOPE_NAMES, cmd -> name, & value, NULL) == 0) {
		fprintf (stderr, "Unknown command '%s' on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
		return 0;
	}

	switch (value) {
		case BKCompilerEnvelopeTypeVolumeSeq:
		case BKCompilerEnvelopeTypePitchSeq:
		case BKCompilerEnvelopeTypePanningSeq:
		case BKCompilerEnvelopeTypeDutyCycleSeq: {
			if (cmd -> numArgs < 3) {
				fprintf (stderr, "Sequence '%s' needs at least 3 values on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
				return 0;
			}
			break;
		}
		case BKCompilerEnvelopeTypeADSR: {
			if (cmd -> numArgs < 4) {
				fprintf (stderr, "Sequence '%s' needs 4 values on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
				return 0;
			}
			break;
		}
		case BKCompilerEnvelopeTypeVolumeEnv:
		case BKCompilerEnvelopeTypePitchEnv:
		case BKCompilerEnvelopeTypePanningEnv:
		case BKCompilerEnvelopeTypeDutyCycleEnv: {
			if (cmd -> numArgs < 4) {
				fprintf (stderr, "Sequence '%s' needs at least 4 values on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
				return 0;
			}
			break;
		}
	}

	switch (value) {
		case BKCompilerEnvelopeTypeVolumeSeq: {
			sequenceLength = BKCompilerInstrumentSequenceParse (cmd, sequence, & repeatBegin, & repeatLength, (BK_MAX_VOLUME / 255));
			res = BKInstrumentSetSequence (instrument, BK_SEQUENCE_VOLUME, sequence, sequenceLength, repeatBegin, repeatLength);
			break;
		}
		case BKCompilerEnvelopeTypePitchSeq: {
			sequenceLength = BKCompilerInstrumentSequenceParse (cmd, sequence, & repeatBegin, & repeatLength, (BK_FINT20_UNIT / 100));
			res = BKInstrumentSetSequence (instrument, BK_SEQUENCE_PITCH, sequence, sequenceLength, repeatBegin, repeatLength);
			break;
		};
		case BKCompilerEnvelopeTypePanningSeq: {
			sequenceLength = BKCompilerInstrumentSequenceParse (cmd, sequence, & repeatBegin, & repeatLength, (BK_MAX_VOLUME / 255));
			res = BKInstrumentSetSequence (instrument, BK_SEQUENCE_PANNING, sequence, sequenceLength, repeatBegin, repeatLength);
			break;
		}
		case BKCompilerEnvelopeTypeDutyCycleSeq: {
			sequenceLength = BKCompilerInstrumentSequenceParse (cmd, sequence, & repeatBegin, & repeatLength, 1);
			res = BKInstrumentSetSequence (instrument, BK_SEQUENCE_DUTY_CYCLE, sequence, sequenceLength, repeatBegin, repeatLength);
			break;
		}
		case BKCompilerEnvelopeTypeADSR: {
			asdr [0] = atoix (cmd -> args [0].arg, 0);
			asdr [1] = atoix (cmd -> args [1].arg, 0);
			asdr [2] = atoix (cmd -> args [2].arg, 0) * (BK_MAX_VOLUME / 255);
			asdr [3] = atoix (cmd -> args [3].arg, 0);
			res = BKInstrumentSetEnvelopeADSR (instrument, asdr [0], asdr [1], asdr [2], asdr [3]);
			break;
		}
		case BKCompilerEnvelopeTypeVolumeEnv: {
			sequenceLength = BKCompilerInstrumentEnvelopeParse (cmd, phases, & repeatBegin, & repeatLength, (BK_MAX_VOLUME / 255));
			res = BKInstrumentSetEnvelope (instrument, BK_SEQUENCE_VOLUME, phases, sequenceLength, repeatBegin, repeatLength);
			break;
		}
		case BKCompilerEnvelopeTypePitchEnv: {
			sequenceLength = BKCompilerInstrumentEnvelopeParse (cmd, phases, & repeatBegin, & repeatLength, (BK_FINT20_UNIT / 100));
			res = BKInstrumentSetEnvelope (instrument, BK_SEQUENCE_PITCH, phases, sequenceLength, repeatBegin, repeatLength);
			break;
		}
		case BKCompilerEnvelopeTypePanningEnv: {
			sequenceLength = BKCompilerInstrumentEnvelopeParse (cmd, phases, & repeatBegin, & repeatLength, (BK_MAX_VOLUME / 255));
			res = BKInstrumentSetEnvelope (instrument, BK_SEQUENCE_PANNING, phases, sequenceLength, repeatBegin, repeatLength);
		}
		case BKCompilerEnvelopeTypeDutyCycleEnv: {
			sequenceLength = BKCompilerInstrumentEnvelopeParse (cmd, phases, & repeatBegin, & repeatLength, 1);
			res = BKInstrumentSetEnvelope (instrument, BK_SEQUENCE_DUTY_CYCLE, phases, sequenceLength, repeatBegin, repeatLength);
			break;
		}
	}

	if (res < 0) {
		fprintf (stderr, "Invalid sequence '%s' (%d) on line %u:%u\n", cmd -> name, res, cmd -> lineno, cmd -> colno);
	}

	return res;
}

static char const * BKCompilerTrimParentPartOfPath (char const * filename)
{
	size_t length = strlen (filename);

	do {
		if (length >= 2 && (memcmp (filename, "./", 2) == 0 || memcmp (filename, ".\\", 2) == 0)) {
			filename += 2;
		}
		if (length >= 3 && (memcmp (filename, "../", 3) == 0 || memcmp (filename, "..\\", 3) == 0)) {
			filename += 3;
		}
		else {
			break;
		}
	}
	while (* filename);

	return filename;
}

static BKInt BKCompilerGetFilePath (BKCompiler * compiler, char const * filename, char path [])
{
	filename = BKCompilerTrimParentPartOfPath (filename);

	if (compiler -> loadPath == NULL) {
		if (getcwd (path, BK_MAX_PATH) == NULL) {
			return -1;
		}

		compiler -> loadPath = strdup (path);
	}

	strncpy (path, compiler -> loadPath, BK_MAX_PATH);
	strncat (path, "/", BK_MAX_PATH);
	strncat (path, filename, BK_MAX_PATH);

	return 0;
}

static BKInt BKCompilerPushCommandSample (BKCompiler * compiler, BKSTCmd const * cmd)
{
	BKInt value;
	char const * filename;
	char path [BK_MAX_PATH];
	FILE * file;
	BKData * sample = compiler -> currentSample;

	// search for command
	if (BKCompilerStrvalTableLookup (miscNames, NUM_MISC_NAMES, cmd -> name, & value, NULL) == 0) {
		fprintf (stderr, "Unknown command '%s' on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
		return 0;
	}

	switch (value) {
		case BKCompilerMiscLoad: {
			if (cmd -> numArgs >= 2) {
				if (strcmpx (cmd -> args [0].arg, "wav") == 0) {
					filename = cmd -> args [1].arg;

					if (BKCompilerGetFilePath (compiler, filename, path) < 0) {
						return -1;
					}

					file = fopen (path, "rb");

					if (file == NULL) {
						fprintf (stderr, "File '%s' not found for line %u:%u\n", filename, cmd -> lineno, cmd -> colno);
						return -1;
					}

					if (BKDataInitAndLoadWAVE (sample, file) < 0) {
						fprintf (stderr, "Failed to load WAVE file '%s' for line %u:%u\n", filename, cmd -> lineno, cmd -> colno);
						return -1;
					}

					fclose (file);
				}
			}

			break;
		}
		case BKCompilerMiscPitch: {
			if (cmd -> numArgs >= 1) {
				BKDataSetAttr (sample, BK_SAMPLE_PITCH, atoix (cmd -> args [0].arg, 0) * PITCH_UNIT);
			}
			break;
		}
		case BKCompilerMiscRaw: {
			break;
		}
	}

	return 0;
}

static BKInt BKCompilerPushCommandTrack (BKCompiler * compiler, BKSTCmd const * cmd)
{
	char const      * arg0str;
	BKInt             values [2];
	BKInt             args [3];
	BKInt             numArgs, value;
	BKInstruction     instr;
	BKCompilerGroup * group = BKArrayGetLastItem (& compiler -> groupStack);
	BKByteBuffer    * cmds  = group -> cmdBuffer;

	arg0str = cmd -> args [0].arg;
	numArgs = cmd -> numArgs;

	if (BKCompilerStrvalTableLookup (cmdNames, NUM_CMD_NAMES, cmd -> name, & value, NULL) == 0) {
		fprintf (stderr, "Unknown command '%s' on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
		return 0;
	}

	instr = value;

	switch (instr) {
		// command:8
		// group number:8
		case BKIntrGroupJump: {
			args [0] = atoix (cmd -> args [1].arg, 0);

			// jump to group
			// command will be replaced with BKIntrCall + Offset
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt32 (cmds, atoix (arg0str, 0));
			break;
		}
		// command:8
		// note:32
		// --- arpeggio
		// command:8
		// num values:8
		// values:32*
		case BKIntrAttack: {
			values [0] = BKCompilerParseNote (arg0str);

			if (values [0] > -1) {
				BKByteBufferAppendInt8 (cmds, instr);
				BKByteBufferAppendInt32 (cmds, values [0]);

				// set arpeggio
				if (numArgs > 1) {
					numArgs = BKMin (numArgs, BK_MAX_ARPEGGIO);

					BKByteBufferAppendInt8 (cmds, BKIntrArpeggio);
					BKByteBufferAppendInt8 (cmds, numArgs);

					for (BKInt j = 0; j < numArgs; j ++) {
						values [1] = BKCompilerParseNote (cmd -> args [j].arg);

						if (values [1] < 0) {
							values [1] = 0;
						}

						BKByteBufferAppendInt32 (cmds, values [1] - values [0]);
					}

					compiler -> flags |= BKCompilerFlagArpeggio;
				}
				// disable arpeggio
				else if (compiler -> flags & BKCompilerFlagArpeggio) {
					BKByteBufferAppendInt8 (cmds, BKIntrArpeggio);
					BKByteBufferAppendInt8 (cmds, 0);
					compiler -> flags &= ~BKCompilerFlagArpeggio;
				}
			}

			break;
		}
		// command:8
		// --- arpeggio
		// command:8
		// 0:8
		case BKIntrRelease:
		case BKIntrMute: {
			// disable arpeggio
			if (compiler -> flags & BKCompilerFlagArpeggio) {
				if (instr == BKIntrMute) {
					BKByteBufferAppendInt8 (cmds, BKIntrArpeggio);
					BKByteBufferAppendInt8 (cmds, 0);
					compiler -> flags &= ~BKCompilerFlagArpeggio;
				}
			}

			BKByteBufferAppendInt8 (cmds, instr);
			break;
		}
		// command:8
		// ticks:16
		case BKIntrAttackTicks:
		case BKIntrReleaseTicks:
		case BKIntrMuteTicks: {
			values [0] = atoix (arg0str, 0);

			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, values [0]);
			break;
		}
		// command:8
		// volume:16
		case BKIntrVolume: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, atoix (arg0str, 0) * VOLUME_UNIT);
			break;
		}
		// command:8
		// volume:16
		case BKIntrMasterVolume: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, atoix (arg0str, 0) * VOLUME_UNIT);
			break;
		}
		// command:8
		// value:16
		case BKIntrStep:
		case BKIntrTicks:
		case BKIntrStepTicks: {
			values [0] = atoix (arg0str, 0);

			if (values [0] == 0) {
				return 0;
			}

			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, values [0]);

			if (instr == BKIntrStepTicks) {
				compiler -> stepTicks = atoix (cmd -> args [0].arg, 0);
			}

			break;
		}
		// command:8
		// value[4]:32
		case BKIntrEffect: {
			if (BKCompilerStrvalTableLookup (effectNames, NUM_EFFECT_NAMES, arg0str, & values [0], NULL)) {
				args [0] = atoix (cmd -> args [1].arg, 0);
				args [1] = atoix (cmd -> args [2].arg, 0);
				args [2] = atoix (cmd -> args [3].arg, 0);

				switch (values [0]) {
					case BK_EFFECT_TREMOLO: {
						args [1] *= VOLUME_UNIT;
						break;
					}
					case BK_EFFECT_VIBRATO: {
						args [1] *= PITCH_UNIT;
						break;
					}
				}

				BKByteBufferAppendInt8 (cmds, instr);
				BKByteBufferAppendInt16 (cmds, values [0]);
				BKByteBufferAppendInt32 (cmds, args [0]);
				BKByteBufferAppendInt32 (cmds, args [1]);
				BKByteBufferAppendInt32 (cmds, args [2]);
			}
			break;
		}
		// command:8
		// duty cycle:8
		case BKIntrDutyCycle: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt8 (cmds, atoix (arg0str, 0));
			break;
		}
		// command:8
		// phase wrap:16
		case BKIntrPhaseWrap: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt32 (cmds, atoix (arg0str, 0));
			break;
		}
		// command:8
		// panning:16
		case BKIntrPanning: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, (int16_t) (atoix (arg0str, 0) * VOLUME_UNIT));
			break;
		}
		// command:8
		// pitch:32
		case BKIntrPitch: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt32 (cmds, atoix (arg0str, 0) * PITCH_UNIT);
			break;
		}
		// command:8
		// instrument:16
		case BKIntrInstrument: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, atoix (arg0str, -1));
			break;
		}
		// command:8
		// waveform:16
		case BKIntrWaveform: {
			if (arg0str) {
				BKByteBufferAppendInt8 (cmds, instr);

				if (BKCompilerStrvalTableLookup (waveformNames, NUM_WAVEFORM_NAMES, arg0str, & values [0], NULL) == 0) {
					values [0] = atoix (arg0str, 0) | BK_INTR_CUSTOM_WAVEFOMR_FLAG;
				}

				BKByteBufferAppendInt16 (cmds, values [0]);
			}
			break;
		}
		// command:8
		// sample:16
		case BKIntrSample: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, atoix (arg0str, -1));
			break;
		}
		// command:8
		// repeat:8
		case BKIntrSampleRepeat: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt8 (cmds, atoix (arg0str, 0));
			break;
		}
		// command:8
		// from:32
		// to:32
		case BKIntrSampleRange: {
			values [0] = atoix (cmd -> args [0].arg, 0);
			args   [0] = atoix (cmd -> args [1].arg, 0);

			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt32 (cmds, values [0]);
			BKByteBufferAppendInt32 (cmds, args [0]);
			break;
		}
		// command:8
		// speed:8
		case BKIntrArpeggioSpeed: {
			BKByteBufferAppendInt8 (cmds, instr);
			BKByteBufferAppendInt16 (cmds, BKMax (atoix (arg0str, 0), 1));
			break;
		}
		// command:8
		case BKIntrSetRepeatStart: {
			BKByteBufferAppendInt8 (cmds, instr);
			break;
		}
		// command:8
		// jump:8
		case BKIntrRepeat: {
			BKByteBufferAppendInt8 (cmds, BKIntrJump);
			BKByteBufferAppendInt32 (cmds, -1);
			break;
		}
		// command:8
		case BKIntrEnd: {
			BKByteBufferAppendInt8 (cmds, instr);
			break;
		}
		// ignore invalid command
		default: {
			break;
		}
	}

	return 0;
}

static BKInt BKCompilerPushCommandWaveform (BKCompiler * compiler, BKSTCmd const * cmd)
{
	BKInt    value;
	BKFrame  sequence [BK_MAX_WAVEFORM_LENGTH];
	BKInt    length = 0;
	BKData * waveform = compiler -> currentWaveform;

	// search for envelope type
	if (BKCompilerStrvalTableLookup (cmdNames, NUM_CMD_NAMES, cmd -> name, & value, NULL) == 0) {
		fprintf (stderr, "Unknown command '%s' on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
		return 0;
	}

	switch (value) {
		case BKIntrStep: {
			length = (BKInt) cmd -> numArgs;
			length = BKMin (length, BK_MAX_WAVEFORM_LENGTH);

			if (length < 2) {
				fprintf (stderr, "Waveform needs at least 2 values on line %u:%u\n", cmd -> lineno, cmd -> colno);
				return 0;
			}

			for (BKInt i = 0; i < length; i ++) {
				sequence [i] = atoix (cmd -> args [i].arg, 0) * (BK_MAX_VOLUME / 255);
			}

			if (BKDataInitWithFrames (waveform, sequence, length, 1, 1) != 0) {
				return -1;
			}

			break;
		}
	}

	return 0;
}

BKInt BKCompilerPushCommand (BKCompiler * compiler, BKSTCmd const * cmd)
{
	BKInt             index, value;
	BKUInt            flags;
	BKInstruction     instr;
	BKCompilerGroup * group, * newGroup;
	BKCompilerTrack * track;
	char const      * args [2];

	switch (cmd -> token) {
		case BKSTTokenValue: {
			// ignore current group
			if (compiler -> ignoreGroupLevel > -1) {
				return 0;
			}

			group = BKArrayGetLastItem (& compiler -> groupStack);

			switch (group -> groupType) {
				case BKIntrInstrumentDef: {
					if (BKCompilerPushCommandInstrument (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
				case BKIntrSampleDef: {
					if (BKCompilerPushCommandSample (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
				default:
				case BKIntrGroupDef:
				case BKIntrTrackDef: {
					if (BKCompilerPushCommandTrack (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
				case BKIntrWaveformDef: {
					if (BKCompilerPushCommandWaveform (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
			}

			break;
		}
		case BKSTTokenGrpBegin: {
			newGroup = BKArrayPushPtr (& compiler -> groupStack);

			if (newGroup == NULL) {
				fprintf (stderr, "Allocation error\n");
				return -1;
			}

			printf("[%s\n", cmd -> name);

			group = BKArrayGetItemAtIndex (& compiler -> groupStack, compiler -> groupStack.length - 2);

			memcpy (newGroup, group, sizeof (* group));
			newGroup -> level ++;

			printf("++ trk: %p grp: %p\n", newGroup -> track, newGroup);

			flags = 0;
			value = 0;

			if (BKCompilerStrvalTableLookup (cmdNames, NUM_CMD_NAMES, cmd -> name, & value, & flags) == 0) {
				fprintf (stderr, "Ignoring unknown group '%s' on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
				compiler -> ignoreGroupLevel = compiler -> groupStack.length - 1;
				return 0;
			}

			instr = value;

			switch (instr) {
				case BKIntrGroupDef: {
					BKByteBuffer * buffer;

					index = atoix (cmd -> args [0].arg, -1);

					if (index >= MAX_GROUPS) {
						fprintf (stderr, "Group number is limited to %u on line %u:%u\n", MAX_GROUPS, cmd -> lineno, cmd -> colno);
						compiler -> ignoreGroupLevel = compiler -> groupStack.length - 1;
						return 0;
					}

					buffer = BKCompilerGetCmdGroupForIndex (compiler, index);

					if (buffer == NULL) {
						return -1;
					}

					newGroup -> cmdBuffer = buffer;
					break;
				}
				case BKIntrInstrumentDef: {
					BKInstrument * instrument;

					index = atoix (cmd -> args [0].arg, -1);

					if (index >= MAX_GROUPS) {
						fprintf (stderr, "Instrument number is limited to %u on line %u:%u\n", MAX_GROUPS, cmd -> lineno, cmd -> colno);
						compiler -> ignoreGroupLevel = compiler -> groupStack.length - 1;
						return 0;
					}

					instrument = BKCompilerGetInstrumentForIndex (compiler, index);

					if (instrument == NULL) {
						return -1;
					}

					compiler -> currentInstrument = instrument;

					break;
				}
				case BKIntrSampleDef: {
					BKData * sample;

					index = atoix (cmd -> args [0].arg, -1);

					if (index >= MAX_GROUPS) {
						fprintf (stderr, "Sample number is limited to %u on line %u:%u\n", MAX_GROUPS, cmd -> lineno, cmd -> colno);
						compiler -> ignoreGroupLevel = compiler -> groupStack.length - 1;
						return 0;
					}

					sample = BKCompilerGetSampleForIndex (compiler, index);

					if (sample == NULL) {
						return -1;
					}

					compiler -> currentSample = sample;
					break;
				}
				case BKIntrTrackDef: {
					BKInt waveform = BK_SQUARE;

					if (BKCompilerTrackAlloc (& track) < 0) {
						return -1;
					}

					if (BKArrayPush (& compiler -> tracks, & track) < 0) {
						return -1;
					}

					newGroup -> track     = track;
					newGroup -> cmdBuffer = & track -> globalCmds;

					if (cmd -> numArgs > 0) {
						args [0] = cmd -> args [0].arg;

						// use custom waveform
						if (BKCompilerStrvalTableLookup (waveformNames, NUM_WAVEFORM_NAMES, args [0], & waveform, NULL) == 0) {
							waveform = atoix (args [0], 0) | BK_INTR_CUSTOM_WAVEFOMR_FLAG;
						}
					}

					// set initial waveform
					BKByteBufferAppendInt8 (& track -> globalCmds, BKIntrWaveform);
					BKByteBufferAppendInt16 (& track -> globalCmds, waveform);

					// set stepticks
					BKByteBufferAppendInt8 (& track -> globalCmds, BKIntrStepTicks);
					BKByteBufferAppendInt16 (& track -> globalCmds, compiler -> stepTicks);

					break;
				}
				case BKIntrWaveformDef: {
					BKData * waveform;

					index = atoix (cmd -> args [0].arg, -1);

					if (index >= MAX_GROUPS) {
						fprintf (stderr, "Waveform number is limited to %u on line %u:%u\n", MAX_GROUPS, cmd -> lineno, cmd -> colno);
						compiler -> ignoreGroupLevel = compiler -> groupStack.length - 1;
						return 0;
					}

					waveform = BKCompilerGetWaveformForIndex (compiler, index);

					if (waveform == NULL) {
						return -1;
					}

					compiler -> currentWaveform = waveform;
					break;
				}
				default: {
					fprintf (stderr, "Unknown group '%s' on line %u:%u\n", cmd -> name, cmd -> lineno, cmd -> colno);
					break;
				}
			}

			newGroup -> groupType = instr;

			break;
		}
		case BKSTTokenGrpEnd: {
			printf("]\n");

			if (compiler -> groupStack.length <= 1) { // needs at minimum 1 item
				fprintf (stderr, "Unbalanced group on line %d:%d\n", cmd -> lineno, cmd -> colno);
				return -1;
			}

			BKArrayPop (& compiler -> groupStack, NULL);

			if (compiler -> groupStack.length <= compiler -> ignoreGroupLevel) {
				compiler -> ignoreGroupLevel = -1;
			}

			break;
		}
		case BKSTTokenEnd:
		case BKSTTokenNone:
		case BKSTTokenComment:
		case BKSTTokenArgSep:
		case BKSTTokenCmdSep: {
			// ignore
			break;
		}
	}

	return 0;
}

static BKInt BKCompilerByteCodeLink (BKCompilerTrack * track, BKByteBuffer * group, BKArray * groupOffsets)
{
	void * opcode    = group -> data;
	void * opcodeEnd = group -> data + group -> size;
	uint8_t cmd;
	BKInt arg, idx;
	BKInt cmdSize;

	while (opcode < opcodeEnd) {
		cmd = * (uint8_t *) opcode ++;
		cmdSize = 0;

		switch (cmd) {
			case BKIntrGroupJump: {
				arg = * (uint32_t *) opcode;

				if (arg < groupOffsets -> length) {
					opcode --;
					cmdSize = 4;

					BKArrayGetItemAtIndexCopy (groupOffsets, arg, & idx);

					(* (uint8_t *) opcode ++) = BKIntrCall;
					(* (uint32_t *) opcode)   = idx;
				}
				else {
					fprintf (stderr, "Undefined group number %d (%ld)\n", arg, groupOffsets -> length);
					return -1;
				}
				break;
			}
			case BKIntrAttack: {
				cmdSize = 4;
				break;
			}
			case BKIntrArpeggio: {
				arg = * (uint8_t *) opcode ++;
				cmdSize = 1 + arg * 4;
				break;
			}
			case BKIntrRelease:
			case BKIntrMute: {
				cmdSize = 0;
				break;
			}
			case BKIntrAttackTicks:
			case BKIntrReleaseTicks: {
				cmdSize = 2;
				break;
			}
			case BKIntrVolume: {
				cmdSize = 2;
				break;
			}
			case BKIntrMasterVolume: {
				cmdSize = 2;
				break;
			}
			case BKIntrStep:
			case BKIntrTicks:
			case BKIntrStepTicks: {
				cmdSize = 2;
				break;
			}
			case BKIntrEffect: {
				cmdSize = 2 + 4 + 4 + 4;
				break;
			}
			case BKIntrDutyCycle: {
				cmdSize = 1;
				break;
			}
			case BKIntrPhaseWrap: {
				cmdSize = 4;
				break;
			}
			case BKIntrPanning: {
				cmdSize = 2;
				break;
			}
			case BKIntrPitch: {
				cmdSize = 4;
				break;
			}
			case BKIntrInstrument:
			case BKIntrWaveform:
			case BKIntrSample: {
				cmdSize = 2;
				break;
			}
			case BKIntrSampleRepeat: {
				cmdSize = 1;
				break;
			}
			case BKIntrSampleRange: {
				cmdSize = 4 + 4;
				break;
			}
			case BKIntrArpeggioSpeed: {
				cmdSize = 2;
				break;
			}
			case BKIntrSetRepeatStart: {
				cmdSize = 0;
				break;
			}
			case BKIntrRepeat: {
				cmdSize = 4;
				break;
			}
		}

		opcode += cmdSize;
	}

	return 0;
}

/**
 * Combine group commands into global commands of track
 */
static BKInt BKCompilerTrackLink (BKCompilerTrack * track)
{
	BKArray groupOffsets;
	BKByteBuffer * group;
	BKInt codeOffset;

	if (BKArrayInit (& groupOffsets, sizeof (BKInt), track -> cmdGroups.length) < 0) {
		return -1;
	}

	codeOffset = track -> globalCmds.size;

	for (BKInt i = 0; i < track -> cmdGroups.length; i ++) {
		BKArrayGetItemAtIndexCopy (& track -> cmdGroups, i, & group);

		if (BKArrayPush (& groupOffsets, & codeOffset) < 0) {
			return -1;
		}

		if (group) {
			// add return command
			BKByteBufferAppendInt8 (group, BKIntrReturn);

			codeOffset += group -> size;
		}
	}

	// link global commands
	if (BKCompilerByteCodeLink (track, & track -> globalCmds, & groupOffsets) < 0) {
		return -1;
	}

	for (BKInt i = 0; i < track -> cmdGroups.length; i ++) {
		BKArrayGetItemAtIndexCopy (& track -> cmdGroups, i, & group);

		if (group == NULL) {
			continue;
		}

		// link group
		if (BKCompilerByteCodeLink (track, group, & groupOffsets) < 0) {
			return -1;
		}

		// append group
		if (BKByteBufferAppendPtr (& track -> globalCmds, group -> data, group -> size) < 0) {
			return -1;
		}

		BKByteBufferEmpty (group, 0);
	}

	BKArrayDispose (& groupOffsets);

	return 0;
}

/**
 * Combine each track's commands into the global commands
 */
static BKInt BKCompilerLink (BKCompiler * compiler)
{
	BKCompilerTrack * track;

	track = & compiler -> globalTrack;

	// append end command
	BKByteBufferAppendInt8 (& track -> globalCmds, BKIntrEnd);

	if (BKCompilerTrackLink (track) < 0) {
		return -1;
	}

	// before linking
	for (BKInt i = 0; i < compiler -> tracks.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> tracks, i, & track);

		// append end command
		BKByteBufferAppendInt8 (& track -> globalCmds, BKIntrEnd);
	}

	// linking tracks
	for (BKInt i = 0; i < compiler -> tracks.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> tracks, i, & track);

		if (BKCompilerTrackLink (track) < 0) {
			return -1;
		}
	}

	return 0;
}

BKInt BKCompilerTerminate (BKCompiler * compiler, BKEnum options)
{
	if (compiler -> groupStack.length > 1) {
		fprintf (stderr, "Unterminated groups (%lu)\n", compiler -> groupStack.length);
		return -1;
	}

	// combine commands and group commands into one array
	if (BKCompilerLink (compiler) < 0) {
		return -1;
	}

	return 0;
}

static BKInt BKCompilerParse (BKCompiler * compiler, BKSTParser * parser)
{
	BKSTCmd cmd;

	while (BKSTParserNextCommand (parser, & cmd)) {
		if (BKCompilerPushCommand (compiler, & cmd) < 0) {
			return -1;
		}
	}

	return 0;
}

BKInt BKCompilerCompile (BKCompiler * compiler, BKSTParser * parser, BKEnum options)
{
	if (BKCompilerParse (compiler, parser) < 0) {
		return -1;
	}

	if (BKCompilerTerminate (compiler, options) < 0) {
		return -1;
	}

	return 0;
}

void BKCompilerReset (BKCompiler * compiler, BKInt keepData)
{
	BKCompilerGroup * group;
	BKCompilerTrack * track;
	BKInstrument    * instrument;
	BKData          * data;

	if (BKArrayGetItemAtIndexCopy (& compiler -> tracks, 0, & track) == 0) {
		BKArrayEmpty (& track -> cmdGroups, keepData);
		BKByteBufferEmpty (& track -> globalCmds, keepData);
	}

	for (BKInt i = 0; i < compiler -> tracks.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> tracks, i, & track);
		BKCompilerTrackDispose (track);
	}

	BKArrayEmpty (& compiler -> groupStack, keepData);
	group = BKArrayPushPtr (& compiler -> groupStack);
	group -> track = & compiler -> globalTrack;
	group -> cmdBuffer = & group -> track -> globalCmds;
	group -> groupType = BKIntrGroupDef;

	compiler -> ignoreGroupLevel = -1;

	for (BKInt i = 0; i < compiler -> instruments.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> instruments, i, & instrument);

		if (instrument) {
			BKInstrumentDispose (instrument);
			free (instrument);
		}
	}

	for (BKInt i = 0; i < compiler -> waveforms.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> waveforms, i, & data);

		if (data) {
			BKDataDispose (data);
			free (data);
		}
	}

	for (BKInt i = 0; i < compiler -> samples.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> samples, i, & data);

		if (data) {
			BKDataDispose (data);
			free (data);
		}
	}

	BKArrayEmpty (& compiler -> instruments, keepData);
	BKArrayEmpty (& compiler -> waveforms, keepData);
	BKArrayEmpty (& compiler -> samples, keepData);

	compiler -> stepTicks = 24;
}
