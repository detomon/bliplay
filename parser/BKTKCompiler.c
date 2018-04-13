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

#include "BKTone.h"
#include "BKWaveFileReader.h"
#include "BKTKCompiler.h"
#include "BKTKTokenizer.h"
#include "BKTKContext.h"

#define MAX_TRACKS     256
#define MAX_GROUPS     256
#define MAX_SEQ_LENGTH 256

#define VOLUME_UNIT (BK_MAX_VOLUME / 255)
#define PITCH_UNIT (BK_FINT20_UNIT / 100)

enum BKCompilerEnvelopeType
{
	BKTKEnvelopeTypeVolumeSeq,
	BKTKEnvelopeTypePitchSeq,
	BKTKEnvelopeTypePanningSeq,
	BKTKEnvelopeTypeDutyCycleSeq,
	BKTKEnvelopeTypeADSR,
	BKTKEnvelopeTypeVolumeEnv,
	BKTKEnvelopeTypePitchEnv,
	BKTKEnvelopeTypePanningEnv,
	BKTKEnvelopeTypeDutyCycleEnv,
};

enum BKCompilerMiscCmds
{
	BKTKMiscLoad,
	BKTKMiscData,
	BKTKMiscPitch,
	BKTKMiscSampleRepeat,
	BKTKMiscSampleSustainRange,
	BKTKMiscSequence,
};

/**
 * Used for lookup tables to assign a string to a value
 */
struct keyval
{
	char const * name;
	BKInt value;
	BKInt flags;
};

extern BKClass const BKTKCompilerClass;

/**
 * Note lookup table
 */
static struct keyval const noteNames [] =
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
static struct keyval const cmdNames [] =
{
	{"a",           BKIntrAttack},
	{"as",          BKIntrArpeggioSpeed},
	{"at",          BKIntrAttackTicks},
	{"d",           BKIntrSample},
	{"dc",          BKIntrDutyCycle},
	{"dn",          BKIntrSampleRange},
	{"dr",          BKIntrSampleRepeat},
	{"ds",          BKIntrSampleSustainRange},
	{"e",           BKIntrEffect},
	{"g",           BKIntrGroupJump},
	{"grp",         BKIntrGroupDef, BKTKParserFlagIsGroup},
	{"i",           BKIntrInstrument},
	{"instr",       BKIntrInstrumentDef, BKTKParserFlagIsGroup},
	{"m",           BKIntrMute},
	{"mt",          BKIntrMuteTicks},
	{"p",           BKIntrPanning},
	{"pk",          BKIntrPulseKernel},
	{"pt",          BKIntrPitch},
	{"pulsekernel", BKIntrPulseKernel},
	{"pw",          BKIntrPhaseWrap},
	{"r",           BKIntrRelease},
	{"rt",          BKIntrReleaseTicks},
	{"s",           BKIntrStep},
	{"samp",        BKIntrSampleDef, BKTKParserFlagIsGroup},
	{"sample",      BKIntrSampleDef, BKTKParserFlagIsGroup},
	{"st",          BKIntrStepTicks},
	{"stepticks",   BKIntrStepTicks},
	{"stt",         BKIntrStepTicksTrack},
	{"t",           BKIntrTicks},
	{"tickrate",    BKIntrTickRate},
	{"tr",          BKIntrTickRate},
	{"track",       BKIntrTrackDef, BKTKParserFlagIsGroup},
	{"tstepticks",  BKIntrStepTicksTrack},
	{"v",           BKIntrVolume},
	{"vm",          BKIntrMasterVolume},
	{"w",           BKIntrWaveform},
	{"wave",        BKIntrWaveformDef, BKTKParserFlagIsGroup},
	{"x",           BKIntrRepeat},
	{"xb",          BKIntrRepeatStart},
	{"z",           BKIntrEnd},
};

/**
 * Effect lookup table
 */
static struct keyval const effectNames [] =
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
static struct keyval const waveformNames [] =
{
	{"noi",      BK_NOISE},
	{"noise",    BK_NOISE},
	{"sample",   BK_SAMPLE},
	{"saw",      BK_SAWTOOTH},
	{"sawtooth", BK_SAWTOOTH},
	{"sin",      BK_SINE},
	{"sine",     BK_SINE},
	{"smp",      BK_SAMPLE},
	{"sqr",      BK_SQUARE},
	{"square",   BK_SQUARE},
	{"tri",      BK_TRIANGLE},
	{"triangle", BK_TRIANGLE},
};

/**
 * Envelope lookup table
 */
static struct keyval const envelopeNames [] =
{
	{"a",    BKTKEnvelopeTypePitchSeq},
	{"adsr", BKTKEnvelopeTypeADSR},
	{"anv",  BKTKEnvelopeTypePitchEnv},
	{"dc",   BKTKEnvelopeTypeDutyCycleSeq},
	{"dcnv", BKTKEnvelopeTypeDutyCycleEnv},
	{"p",    BKTKEnvelopeTypePanningSeq},
	{"pnv",  BKTKEnvelopeTypePanningEnv},
	{"v",    BKTKEnvelopeTypeVolumeSeq},
	{"vnv",  BKTKEnvelopeTypeVolumeEnv},
};

/**
 * Miscellaneous lookup table
 */
static struct keyval const miscNames [] =
{
	{"data", BKTKMiscData},
	{"dr",   BKTKMiscSampleRepeat},
	{"ds",   BKTKMiscSampleSustainRange},
	{"load", BKTKMiscLoad},
	{"pt",   BKTKMiscPitch},
	{"s",    BKTKMiscSequence},
};

/**
 * Repeat modes lookup table
 */
static struct keyval const repeatNames [] =
{
	{"no",  BK_NO_REPEAT},
	{"pal", BK_PALINDROME},
	{"rep", BK_REPEAT},
};

/**
 * Pulse kernels
 */
static struct keyval const pulseNames [] =
{
	{"harm", BK_PULSE_KERNEL_HARM},
	{"sinc", BK_PULSE_KERNEL_SINC},
};

#define NUM_WAVEFORM_NAMES (sizeof (waveformNames) / sizeof (struct keyval))
#define NUM_CMD_NAMES (sizeof (cmdNames) / sizeof (struct keyval))
#define NUM_EFFECT_NAMES (sizeof (effectNames) / sizeof (struct keyval))
#define NUM_NOTE_NAMES (sizeof (noteNames) / sizeof (struct keyval))
#define NUM_ENVELOPE_NAMES (sizeof (envelopeNames) / sizeof (struct keyval))
#define NUM_MISC_NAMES (sizeof (miscNames) / sizeof (struct keyval))
#define NUM_REPEAT_NAMES (sizeof (repeatNames) / sizeof (struct keyval))
#define NUM_PULSE_NAMES (sizeof (pulseNames) / sizeof (struct keyval))

/**
 * Convert string to signed integer like `atoi`
 *
 * Returns alternative value if string is NULL
 */
static int strtolx (uint8_t const * str, int alt)
{
	uint8_t * end;
	long i = strtol ((char *) str, (char **) &end, 10);

	return end > str ? (int) i : alt;
}

/**
 * Compare name of command with string
 * Used as callback for `bsearch`
 */
static int keyvalcmp (BKString const * name, struct keyval const * item)
{
	return strcmp ((void *) name -> str, item -> name);
}

static BKInt keyvalLookup (struct keyval const table [], BKSize size, BKString const * name, BKInt * outValue, BKUInt * outFlags)
{
	struct keyval * item;

	if (name == NULL) {
		return 0;
	}

	item = bsearch (name, table, size, sizeof (struct keyval), (void *) keyvalcmp);

	if (item == NULL) {
		return 0;
	}

	*outValue = item -> value;

	if (outFlags) {
		*outFlags = item -> flags;
	}

	return 1;
}

/**
 * Escape string and limit output
 */
static char const * BKTKCompilerEscapeString (BKTKCompiler * compiler, BKString const * string)
{
	BKStringEscapeString (&compiler -> auxString, string);

	if (compiler -> auxString.len > 64) {
		compiler -> auxString.str [64] = '\0';
		compiler -> auxString.len = 64;
		BKStringAppend (&compiler -> auxString, "...");
	}

	return (char *) compiler -> auxString.str;
}

BK_INLINE uint32_t BKInstrMaskArg1Make (BKUInt cmd, BKInt arg1)
{
	BKInstrMask mask = (BKInstrMask) {
		.arg1 = {cmd, arg1},
	};

	return mask.value;
}

BK_INLINE uint32_t BKInstrMaskArg2Make (BKUInt cmd, BKInt arg1, BKInt arg2)
{
	BKInstrMask mask = (BKInstrMask) {
		.arg2 = {cmd, arg1, arg2},
	};

	return mask.value;
}

BK_INLINE uint32_t BKInstrMaskGrpMake (BKUInt cmd, BKUInt idx1, BKUInt idx2, BKUInt type)
{
	BKInstrMask mask = (BKInstrMask) {
		.grp = {cmd, type, idx1, idx2},
	};

	return mask.value;
}

BK_INLINE BKInstrMask BKReadIntrMask (void ** opcode)
{
	BKInstrMask mask = *(BKInstrMask *) *opcode;
	(* opcode) += sizeof (uint32_t);

	return mask;
}

static void printErrorUnexpectedCommand (BKTKCompiler * compiler, BKTKParserNode const * node)
{
	char const * type = (node -> flags & BKTKParserFlagIsGroup) ? "group" : "command";

	BKStringAppendFormat (&compiler -> error, "Warning: unexpected %s '%s' on line %u:%u\n",
		type, BKTKCompilerEscapeString (compiler, &node -> name),
		node -> offset.lineno, node -> offset.colno);
}

static void printError (BKTKCompiler * compiler, BKTKParserNode const * node, char const * format, ...)
{
	va_list args;

	va_start (args, format);
	BKStringAppendFormatArgs (&compiler -> error, format, args);

	if (node) {
		BKStringAppendFormat (&compiler -> error, " on line %d:%d\n", node -> offset.lineno, node -> offset.colno);
	}

	va_end (args);
}

/**
 * Get note value for note string
 *
 * note octave [+-pitch]
 * Examples: c#3, e2+56, a#2-26
 */
static BKInt parseNote (BKString const * string, BKInt * outNote, BKInt * outPitch)
{
	char noteStr [4];
	BKInt value  = 0;
	BKInt octave = 0;
	BKInt pitch  = 0;
	BKInt res;
	BKString note = (BKString) {
		.str = (uint8_t *) noteStr,
		.len = 0,
		.cap = 4,
	};

	BKStringEmpty (&note);
	res = sscanf ((char *) string -> str, "%2[a-z#]%zn%d%d", note.str, &note.len, &octave, &pitch); // d#3[+-p] => "d#", 3, p

	if (keyvalLookup (noteNames, NUM_NOTE_NAMES, &note, & value, NULL)) {
		value += octave * 12;
		*outNote = BKClamp (value, BK_MIN_NOTE, BK_MAX_NOTE);
		*outPitch = pitch;

		return res;
	}

	return 0;
}

/**
 * Get group index and type
 *
 * index type [index]
 * Examples: 12, 3g, 7t2
 */
static BKInt parseGroupIndex (BKString const * string, BKInt * outIdx, BKInt * outIdx2, BKInt * outType)
{
	BKInt res;
	BKInt idx = 0, idx2 = 0;
	uint8_t type = BKGroupIndexTypeLocal;

	res = sscanf ((char *) string -> str, "%d%c%d", &idx, &type, &idx2); // 12t17 => 12, 't', 17

	switch (type) {
		case 'g': {
			type = BKGroupIndexTypeGlobal;
			break;
		}
		case 't': {
			type = BKGroupIndexTypeTrack;
			break;
		}
	}

	*outIdx = idx;
	*outIdx2 = idx2;
	*outType = type;

	return res;
}

static void parseTicksFormat (BKString const * format, BKInt args [2])
{
	args [0] = 0;
	args [1] = 0;

	if (sscanf ((char *) format -> str, "%d/%d", &args [0], &args [1]) >= 2) {
		args [0] = BKClamp (args [0], 1, 1 << 12);
		args [1] = BKClamp (args [1], 1, 1 << 12);
	}
}

static BKInt parseDataParams (BKString const * arg)
{
	BKUInt params = 0;
	BKInt endian = 0;
	BKInt bits = 16;
	uint8_t sign = 's';
	uint8_t endianChar = 0;

	sscanf ((char *) arg -> str, "%d%c%c", &bits, &sign, &endianChar);

	switch (bits) {
		case 1:  params |= BK_1_BIT_UNSIGNED; break;
		case 2:  params |= BK_2_BIT_UNSIGNED; break;
		case 4:  params |= BK_4_BIT_UNSIGNED; break;
		case 8:  params |= (sign == 's' ? BK_8_BIT_SIGNED : BK_8_BIT_UNSIGNED);	break;
		case 16: params |= BK_16_BIT_SIGNED; break;
	}

	switch (endianChar) {
		case 'b': endian = BK_BIG_ENDIAN; break;
		case 'l': endian = BK_LITTLE_ENDIAN; break;
	}

	params |= endian;

	return params;
}

/**
 * Get node argument at `offset`
 *
 * Returns empty string if no argument exists at given offset
 */
static BKString const * nodeArgString (BKTKParserNode const * node, BKUSize offset)
{
	// BK_STRING_INIT not working with 'static' in GCC
	static BKString empty = {(uint8_t *) "", 0, 0};

	if (offset >= node -> argCount) {
		return &empty;
	}

	return &node -> args [offset];
}

BK_INLINE BKInt value2Volume (BKInt value)
{
	return (BKInt) ((int64_t) value * BK_MAX_VOLUME / 255);
}

BK_INLINE BKInt value2Pitch (BKInt value)
{
	return (BKInt) ((int64_t) value * BK_FINT20_UNIT / 100);
}

/**
 * Get node argument at `offset`
 *
 * Returns `alt` if no argument exists at given offset
 */
static BKInt nodeArgInt (BKTKParserNode const * node, BKUSize offset, BKInt alt)
{
	return strtolx ((uint8_t *) nodeArgString (node, offset) -> str, alt);
}

static BKInt firstUnusedSlot (BKArray * list)
{
	BKInt i;
	BKTKObject const * object;

	for (i = 0; i < list -> len; i ++) {
		object = *(BKTKObject **) BKArrayItemAt (list, i);

		if (!object || !(object -> object.flags & BKTKFlagUsed)) {
			break;
		}
	}

	return i;
}

static BKTKTrack * BKTKCompilerTrackAtOffset (BKTKCompiler * compiler, BKUSize offset, BKInt create)
{
	BKTKTrack * track;
	BKTKTrack ** trackRef;

	if (BKArrayResize (&compiler -> tracks, offset + 1) != 0) {
		return NULL;
	}

	trackRef = BKArrayItemAt (&compiler -> tracks, offset);
	track = *trackRef;

	if (!track && create) {
		track = calloc (1, sizeof (BKTKTrack));

		if (!track) {
			return NULL;
		}

		*trackRef = track;

		track -> byteCode = BK_BYTE_BUFFER_INIT;
		track -> groups = BK_ARRAY_INIT (sizeof (BKTKGroup *));
	}

	return track;
}

static BKTKGroup * BKTKCompilerTrackGroupAtOffset (BKTKTrack * track, BKUSize offset, BKInt create)
{
	BKTKGroup * group;
	BKTKGroup ** groupRef;

	if (BKArrayResize (&track -> groups, offset + 1) != 0) {
		return NULL;
	}

	groupRef = BKArrayItemAt (&track -> groups, offset);
	group = *groupRef;

	if (!group && create) {
		group = calloc (1, sizeof (BKTKGroup));

		if (!group) {
			return NULL;
		}

		*groupRef = group;

		group -> byteCode = BK_BYTE_BUFFER_INIT;
	}

	return group;
}

BKInt BKTKCompilerInit (BKTKCompiler * compiler)
{
	BKInt res;

	if ((res = BKObjectInit (compiler, &BKTKCompilerClass, sizeof (*compiler))) != 0) {
		return res;
	}

	compiler -> tracks      = BK_ARRAY_INIT (sizeof (BKTKTrack *));
	compiler -> instruments = BK_HASH_TABLE_INIT;
	compiler -> waveforms   = BK_HASH_TABLE_INIT;
	compiler -> samples     = BK_HASH_TABLE_INIT;
	compiler -> auxString   = BK_STRING_INIT;
	compiler -> error       = BK_STRING_INIT;

	if ((res = BKTKCompilerReset (compiler)) != 0) {
		return -1;
	}

	return 0;
}

static BKInt BKTKCompilerCompileCommand (BKTKCompiler * compiler, BKTKParserNode const * node, BKByteBuffer * byteCode, BKInt cmd, BKInt level)
{
	BKInt arg;
	BKInt args [8];
	BKString const * name;
	BKInt note, note2;

	if (BKByteBufferReserve (byteCode, 64 * sizeof (uint32_t)) != 0) {
		goto allocationError;
	}

	if (compiler -> lineno != node -> offset.lineno) {
		compiler -> lineno = node -> offset.lineno;
		BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (BKIntrLineNo, (BKInt) compiler -> lineno));
	}

	switch (cmd) {
		case BKIntrAttack: {
			BKUSize i;

			if (node -> argCount < 1) {
				printError (compiler, node, "Error: attack command needs at least one argument");
				goto error;
			}
			else if (node -> argCount > BK_MAX_ARPEGGIO) {
				printError (compiler, node, "Error: too many arpeggio notes (%u)", BK_MAX_ARPEGGIO);
				goto error;
			}

			args [0] = 0;
			args [1] = 0;
			name = nodeArgString (node, 0);
			arg = parseNote (name, &args [0], &args [1]);

			if (arg < 2) {
				printError (compiler, node, "Error: note '%s' has no valid format",
					BKTKCompilerEscapeString (compiler, name));
				goto error;
			}

			note = args [0] * 100 + args [1];
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, note));

			if (node -> argCount > 1) {
				BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (BKIntrArpeggio, (BKInt) node -> argCount - 1));

				for (i = 1; i < node -> argCount; i ++) {
					name = nodeArgString (node, i);
					arg = parseNote (name, &args [0], &args [1]);

					if (arg < 2) {
						printError (compiler, node, "Error: note '%s' has no valid format",
							BKTKCompilerEscapeString (compiler, name));
						goto error;
					}

					note2 = args [0] * 100 + args [1] - note;
					BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (0, note2));
				}
			}

			break;
		}
		case BKIntrStepTicks: {
			parseTicksFormat (nodeArgString (node, 0), args);
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg2Make (cmd, args [0], args [1]));

			if (level == 0 && !compiler -> info.stepTicks) {
				compiler -> info.stepTicks = args [0];
			}
			break;
		}
		// single arguments "n" or "n/m"
		case BKIntrStepTicksTrack:
		case BKIntrArpeggioSpeed:
		case BKIntrReleaseTicks:
		case BKIntrMuteTicks:
		case BKIntrAttackTicks: {
			parseTicksFormat (nodeArgString (node, 0), args);
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg2Make (cmd, args [0], args [1]));
			break;
		}
		case BKIntrDutyCycle: {
			args [0] = BKClamp (nodeArgInt (node, 0, 0), 1, BK_MAX_DUTY_CYCLE);
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			break;
		}
			// range commands
		case BKIntrSampleRange:
		case BKIntrSampleSustainRange: {
			args [0] = nodeArgInt (node, 0, 0);
			args [1] = nodeArgInt (node, 1, 0);
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, 0));
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (0, args [0]));
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (0, args [1]));
			break;
		}
		case BKIntrSampleRepeat: {
			name = nodeArgString (node, 0);

			if (!keyvalLookup (repeatNames, NUM_REPEAT_NAMES, name, &arg, NULL)) {
				printError (compiler, node, "Error: expected repeat mode: 'no', 'rep', 'pal'");
				goto error;
			}

			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, arg));
			break;
		}
		case BKIntrEffect: {
			name = nodeArgString (node, 0);

			if (!keyvalLookup (effectNames, NUM_EFFECT_NAMES, name, &args [0], NULL)) {
				printError (compiler, node, "Error: expected effect name: 'pr', 'ps', 'tr', 'vb', 'vs'");
				goto error;
			}

			parseTicksFormat (nodeArgString (node, 1), &args [1]);
			args [3] = nodeArgInt (node, 2, 0);
			parseTicksFormat (nodeArgString (node, 3), &args [4]);

			switch (args [0]) {
				case BK_EFFECT_TREMOLO: {
					args [3] = BKClamp (args [3], 0, 255);
					args [3] = value2Volume (args [3]);
					break;
				}
				case BK_EFFECT_VIBRATO: {
					args [3] = BKClamp (args [3], -9600, 9600);
					args [3] = value2Pitch (args [3]);
					break;
				}
			}

			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg2Make (0, args [1], args [2]));
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (0, args [3]));
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg2Make (0, args [4], args [5]));

			break;
		}
		case BKIntrGroupJump: {
			name = nodeArgString (node, 0);
			parseGroupIndex (name, &args [0], &args [1], &args [2]);
			args [1] ++; // 1 based index (0 is root)
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskGrpMake (BKIntrCall, args [0], args [1], args [2]));
			// save file offset for error output
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (0, node -> offset.lineno));
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (0, node -> offset.colno));
			break;
		}
		case BKIntrPanning: {
			args [0] = nodeArgInt (node, 0, 0);
			args [0] = BKClamp (args [0], -255, 255) * VOLUME_UNIT;
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			break;
		}
		case BKIntrPitch: {
			args [0] = nodeArgInt (node, 0, 0);
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			break;
		}
		case BKIntrPhaseWrap: {
			args [0] = nodeArgInt (node, 0, 0);
			args [0] = BKClamp (args [0], 0, 1 << 20);
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			break;
		}
		case BKIntrPulseKernel: {
			name = nodeArgString (node, 0);

			if (!keyvalLookup (pulseNames, NUM_PULSE_NAMES, name, &args [0], NULL)) {
				printError (compiler, node, "Error: expected pulse kernel name: 'harm', 'sinc'");
				goto error;
			}

			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			break;
		}
		// commands without arguments
		case BKIntrRelease:
		case BKIntrMute:
		case BKIntrRepeatStart:
		case BKIntrEnd: {
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, 0));
			break;
		}
		case BKIntrRepeat: {
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (BKIntrJump, -1));
			break;
		}
			// step commands
		case BKIntrTicks:
		case BKIntrStep: {
			args [0] = nodeArgInt (node, 0, 0);

			if (args [0] > 0) {
				BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			}
			break;
		}
			// volume commands
		case BKIntrVolume:
		case BKIntrMasterVolume: {
			args [0] = nodeArgInt (node, 0, 0);
			args [0] = BKClamp (args [0], 0, 255) * VOLUME_UNIT;
			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, args [0]));
			break;
		}
		case BKIntrTickRate: {
			parseTicksFormat (nodeArgString (node, 0), &args [0]);
			args [0] = BKClamp (args [0], 1, (1 << 12) - 1);
			args [1] = BKClamp (args [1], 0, (1 << 12) - 1);

			if (args [1] <= 0) {
				args [1] = args [0];
				args [0] = 1;
			}

			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg2Make (cmd, args [0], args [1]));

			if (level == 0 && !compiler -> info.tickRate.factor) {
				compiler -> info.tickRate.factor = args [0];
				compiler -> info.tickRate.divisor = args [1];
			}
			break;
		}
		case BKIntrInstrument: {
			BKTKInstrument * instrument;

			name = nodeArgString (node, 0);

			if (name -> len) {
				if (!BKHashTableLookup (&compiler -> instruments, (char *) name -> str, (void **) &instrument)) {
					printError (compiler, node, "Error: undefined instrument '%s'",
						BKTKCompilerEscapeString (compiler, name));
					goto error;
				}

				BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, instrument -> object.index));
			}
			else {
				BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, -1));
			}

			break;
		}
		case BKIntrWaveform: {
			BKInt value = -1;
			BKTKWaveform * waveform = NULL;

			name = nodeArgString (node, 0);

			if (!BKHashTableLookup (&compiler -> waveforms, (char *) name -> str, (void **) &waveform)) {
				keyvalLookup (waveformNames, NUM_WAVEFORM_NAMES, name, &value, NULL);
			}

			if (waveform) {
				BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, waveform -> object.index | BK_INTR_CUSTOM_WAVEFORM_FLAG));
			}
			else if (value > 0) {
				BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, value));
			}
			else {
				printError (compiler, node, "Error: undefined waveform '%s'",
					BKTKCompilerEscapeString (compiler, name));
				goto error;
			}

			break;
		}
		case BKIntrSample: {
			BKTKSample * sample;

			name = nodeArgString (node, 0);

			if (!BKHashTableLookup (&compiler -> samples, (char *) name -> str, (void **) &sample)) {
				printError (compiler, node, "Error: undefined sample '%s'",
					BKTKCompilerEscapeString (compiler, name));
				goto error;
			}

			BKByteBufferAppendInt32 (byteCode, BKInstrMaskArg1Make (cmd, sample -> object.index));

			break;
		}
		default: {
			printErrorUnexpectedCommand (compiler, node);
			break;
		}
	}

	return 0;

	allocationError: {
		printError (compiler, node, "Error: allocation failed");
		goto error;
	}

	error: {
		return -1;
	}
}

static BKInt BKTKCompilerParseSequence (BKTKParserNode const * node, BKInt sequence [], BKInt * outLength, BKInt * outRepeatBegin, BKInt * outRepeatLength, BKInt multiplier, BKInt divider)
{
	BKInt value;
	BKInt length = 0;
	BKInt repeatBegin = -1;
	BKInt repeatEnd = -1;
	BKString const * arg;

	for (BKUSize i = 0; i < node -> argCount; i ++) {
		arg = nodeArgString (node, i);

		if (strcmp ((char *) arg -> str, "<") == 0) {
			repeatBegin = (BKInt) length;
		}
		else if (strcmp ((char *) arg -> str, ">") == 0) {
			repeatEnd = (BKInt) length;
		}
		else {
			value = nodeArgInt (node, i, 0);
			sequence [length ++] = (BKInt) (((int64_t) value * multiplier) / divider);
		}

		if (length >= MAX_SEQ_LENGTH) {
			break;
		}
	}

	if (repeatBegin < 0) {
		repeatBegin = length;
		repeatEnd = length;
	}
	else if (repeatEnd < 0) {
		repeatEnd = length;
	}

	*outLength = length;
	*outRepeatBegin = repeatBegin;
	*outRepeatLength = repeatEnd - repeatBegin;

	return 0;
}

static BKInt BKTKCompilerParseEnvelope (BKTKParserNode const * node, BKSequencePhase phases [], BKInt * outLength, BKInt * outRepeatBegin, BKInt * outRepeatLength, BKInt multiplier, BKInt divider)
{
	BKInt value;
	BKInt length = 0;
	BKInt repeatBegin = -1;
	BKInt repeatEnd = -1;
	BKString const * arg;

	for (BKUSize i = 0; i < node -> argCount; i ++) {
		arg = nodeArgString (node, i);

		if (strcmp ((char *) arg -> str, "<") == 0) {
			repeatBegin = (BKInt) length;
		}
		else if (strcmp ((char *) arg -> str, ">") == 0) {
			repeatEnd = (BKInt) length;
		}
		else {
			if (i + 1 >= node -> argCount) {
				break;
			}

			value = nodeArgInt (node, i, 0);
			phases [length].steps = value;
			value = nodeArgInt (node, i + 1, 0);
			phases [length].value = (BKInt) (((int64_t) value * multiplier) / divider);
			length ++;
			i ++;
		}

		if (length >= MAX_SEQ_LENGTH) {
			break;
		}
	}

	if (repeatBegin < 0) {
		repeatBegin = length;
		repeatEnd = length;
	}
	else if (repeatEnd < 0) {
		repeatEnd = length;
	}

	*outLength = length;
	*outRepeatBegin = repeatBegin;
	*outRepeatLength = repeatEnd - repeatBegin;

	return 0;
}

static BKInt BKTKCompilerCompileInstrument (BKTKCompiler * compiler, BKTKParserNode const * tree)
{
	BKInt res = 0;
	BKUInt flags;
	BKTKParserNode const * node;
	BKTKInstrument ** instrument;
	BKString const * name;
	BKString auxString = BK_STRING_INIT;
	BKInt seqType, type, isEnv;
	BKInt length = 0, repeatBegin = 0, repeatLength = 0;
	BKSequencePhase sequence [MAX_SEQ_LENGTH];
	BKInt autoindex = 0;

	name = nodeArgString (tree, 0);

	if (name -> len) {
		BKStringAppendString (&auxString, name);
	}
	else {
		BKStringAppendFormat (&auxString, "%ld", BKHashTableSize (&compiler -> instruments));
		autoindex = 1;
	}

	if (BKHashTableLookupOrInsert (&compiler -> instruments, (char *) auxString.str, (void ***) &instrument) < 0) {
		printError (compiler, tree, "Error: allocation failed");
		res = -1;
		goto cleanup;
	}

	if (*instrument) {
		if (autoindex) {
			printError (compiler, tree, "Error: instrument '%s' already defined with autoindex on line %u:%u but redefined",
				BKTKCompilerEscapeString (compiler, &auxString),
				(*instrument) -> object.offset.lineno, (*instrument) -> object.offset.colno);
		}
		else {
			printError (compiler, tree, "Error: instrument '%s' already defined on line %u:%u but redefined",
				BKTKCompilerEscapeString (compiler, &auxString),
				(*instrument) -> object.offset.lineno, (*instrument) -> object.offset.colno);

		}
		res = -1;
		goto cleanup;
	}

	if ((res = BKTKInstrumentAlloc (instrument)) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		goto cleanup;
	}

	(*instrument) -> object.index = (BKUInt) BKHashTableSize (&compiler -> instruments) - 1;
	(*instrument) -> object.offset = tree -> offset;
	BKStringAppendString (&(*instrument) -> name, &auxString);

	for (node = tree -> subNode; node; node = node -> nextNode) {
		if (node -> type == BKTKTypeComment) {
			continue;
		}

		if (node -> flags & BKTKParserFlagIsGroup) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		if (!keyvalLookup (envelopeNames, NUM_ENVELOPE_NAMES, &node -> name, &seqType, &flags)) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		res = 0;
		type = -1;
		isEnv = 0;

		switch (seqType) {
			case BKTKEnvelopeTypePitchSeq: {
				res = BKTKCompilerParseSequence (node, (BKInt *) sequence, &length, &repeatBegin, &repeatLength, BK_FINT20_UNIT, 100);
				type = BK_SEQUENCE_PITCH;
				break;
			}
			case BKTKEnvelopeTypeADSR: {
				BKInt adsr [4];

				adsr [0] = nodeArgInt (node, 0, 0);
				adsr [1] = nodeArgInt (node, 1, 0);
				adsr [2] = nodeArgInt (node, 2, 0) * VOLUME_UNIT;
				adsr [3] = nodeArgInt (node, 3, 0);

				res = BKInstrumentSetEnvelopeADSR (&(*instrument) -> instr, adsr [0], adsr [1], adsr [2], adsr [3]);
				break;
			}
			case BKTKEnvelopeTypePitchEnv: {
				res = BKTKCompilerParseEnvelope (node, sequence, &length, &repeatBegin, &repeatLength, BK_FINT20_UNIT, 100);
				type = BK_SEQUENCE_PITCH;
				isEnv = 1;
				break;
			}
			case BKTKEnvelopeTypeDutyCycleSeq: {
				res = BKTKCompilerParseSequence (node, (BKInt *) sequence, &length, &repeatBegin, &repeatLength, 1, 1);
				type = BK_SEQUENCE_DUTY_CYCLE;
				break;
			}
			case BKTKEnvelopeTypeDutyCycleEnv: {
				res = BKTKCompilerParseEnvelope (node, sequence, &length, &repeatBegin, &repeatLength, 1, 1);
				type = BK_SEQUENCE_DUTY_CYCLE;
				isEnv = 1;
				break;
			}
			case BKTKEnvelopeTypePanningSeq: {
				res = BKTKCompilerParseSequence (node, (BKInt *) sequence, &length, &repeatBegin, &repeatLength, BK_MAX_VOLUME, 255);
				type = BK_SEQUENCE_PANNING;
				break;
			}
			case BKTKEnvelopeTypePanningEnv: {
				res = BKTKCompilerParseEnvelope (node, sequence, &length, &repeatBegin, &repeatLength, BK_MAX_VOLUME, 255);
				type = BK_SEQUENCE_PANNING;
				isEnv = 1;
				break;
			}
			case BKTKEnvelopeTypeVolumeSeq: {
				res = BKTKCompilerParseSequence (node, (BKInt *) sequence, &length, &repeatBegin, &repeatLength, BK_MAX_VOLUME, 255);
				type = BK_SEQUENCE_VOLUME;
				break;
			}
			case BKTKEnvelopeTypeVolumeEnv: {
				res = BKTKCompilerParseEnvelope (node, sequence, &length, &repeatBegin, &repeatLength, BK_MAX_VOLUME, 255);
				type = BK_SEQUENCE_VOLUME;
				isEnv = 1;
				break;
			}
			default: {
				printErrorUnexpectedCommand (compiler, node);
				break;
			}
		}

		if (res < 0) {
			printError (compiler, node, "Error: invalid sequence '%s' (%s)",
				BKTKCompilerEscapeString (compiler, &node -> name), BKStatusGetName (res));
		}

		if (type >= 0) {
			if (isEnv) {
				res = BKInstrumentSetEnvelope (&(*instrument) -> instr, type, sequence, length, repeatBegin, repeatLength);
			}
			else {
				res = BKInstrumentSetSequence (&(*instrument) -> instr, type, (BKInt *) sequence, length, repeatBegin, repeatLength);
			}
		}

		if (res < 0) {
			printError (compiler, node, "Error: malformed sequence '%s'; possibly invalid sustain range (%s)",
				BKTKCompilerEscapeString (compiler, &node -> name), BKStatusGetName (res));

		}
	}

	cleanup: {
		BKStringDispose (&auxString);

		return res;
	}
}

static BKInt BKTKCompilerCompileWaveform (BKTKCompiler * compiler, BKTKParserNode const * tree)
{
	BKInt res = 0;
	BKInt value;
	BKUInt flags;
	BKTKParserNode const * node;
	BKTKWaveform ** waveform;
	BKString const * name;
	BKString auxString = BK_STRING_INIT;
	BKInt autoindex = 0;

	name = nodeArgString (tree, 0);

	if (name -> len) {
		BKStringAppendString (&auxString, name);
	}
	else {
		BKStringAppendFormat (&auxString, "%ld", BKHashTableSize (&compiler -> waveforms));
		autoindex = 1;
	}

	if (keyvalLookup (waveformNames, NUM_WAVEFORM_NAMES, name, &value, NULL)) {
		printError (compiler, tree, "Error: waveform '%s' overwrites default waveform",
			BKTKCompilerEscapeString (compiler, name));
		res = -1;
		goto cleanup;
	}

	if (BKHashTableLookupOrInsert (&compiler -> waveforms, (char *) auxString.str, (void ***) &waveform) < 0) {
		printError (compiler, tree, "Error: allocation failed");
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}

	if (*waveform) {
		if (autoindex) {
			printError (compiler, tree, "Error: waveform '%s' already defined with autoindex on line %u:%u but redefined",
				BKTKCompilerEscapeString (compiler, name),
				(*waveform) -> object.offset.lineno, (*waveform) -> object.offset.colno);
		}
		else {
			printError (compiler, tree, "Error: waveform '%s' already defined on line %u:%u but redefined",
				BKTKCompilerEscapeString (compiler, name),
				(*waveform) -> object.offset.lineno, (*waveform) -> object.offset.colno);

		}
		res = -1;
		goto cleanup;
	}

	if ((res = BKTKWaveformAlloc (waveform)) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		goto cleanup;
	}

	(*waveform) -> object.index = (BKUInt) BKHashTableSize (&compiler -> waveforms) - 1;
	(*waveform) -> object.offset = tree -> offset;
	BKStringAppendString (&(*waveform) -> name, &auxString);

	for (node = tree -> subNode; node; node = node -> nextNode) {
		BKInt arg;
		BKInt length = 0;
		BKFrame sequence [MAX_SEQ_LENGTH];

		if (node -> type == BKTKTypeComment) {
			continue;
		}

		if (!keyvalLookup (miscNames, NUM_MISC_NAMES, &node -> name, &value, &flags)) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		if (flags & BKTKParserFlagIsGroup) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		switch (value) {
			case BKTKMiscSequence: {
				for (BKUSize i = 0; i < node -> argCount; i ++) {
					arg = nodeArgInt (node, i, 0);
					sequence [length ++] = arg * VOLUME_UNIT;

					if (length >= MAX_SEQ_LENGTH) {
						break;
					}
				}

				res = BKDataSetFrames (&(*waveform) -> data, sequence, length, 1, 1);

				if (res != 0) {
					printError (compiler, tree, "Error: failed to set waveform (%s)", BKStatusGetName (res));
					res = -1;
					goto cleanup;

				}

				break;
			}
			default: {
				printErrorUnexpectedCommand (compiler, node);
			}
		}
	}

	cleanup: {
		BKStringDispose (&auxString);

		return res;
	}
}

static BKInt BKTKCompilerCompileSample (BKTKCompiler * compiler, BKTKParserNode const * tree)
{
	BKInt res = 0;
	BKInt value;
	BKUInt flags;
	BKTKParserNode const * node;
	BKTKSample ** sample;
	BKString const * name;
	BKString auxString = BK_STRING_INIT;
	BKInt autoindex = 0;

	name = nodeArgString (tree, 0);

	if (name -> len) {
		BKStringAppendString (&auxString, name);
	}
	else {
		BKStringAppendFormat (&auxString, "%ld", BKHashTableSize (&compiler -> samples));
		autoindex = 1;
	}

	if (BKHashTableLookupOrInsert (&compiler -> samples, (char *) auxString.str, (void ***) &sample) < 0) {
		printError (compiler, tree, "Error: allocation failed");
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}

	if (*sample) {
		if (autoindex) {
			printError (compiler, tree, "Error: sample '%s' already defined with autoindex on line %u:%u but redefined",
				BKTKCompilerEscapeString (compiler, name),
				(*sample) -> object.offset.lineno, (*sample) -> object.offset.colno);
		}
		else {
			printError (compiler, tree, "Error: sample '%s' already defined on line %u:%u but redefined",
				BKTKCompilerEscapeString (compiler, name),
				(*sample) -> object.offset.lineno, (*sample) -> object.offset.colno);
		}
		res = -1;
		goto cleanup;
	}

	if ((res = BKTKSampleAlloc (sample)) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		goto cleanup;
	}

	(*sample) -> object.index = (BKUInt) BKHashTableSize (&compiler -> samples) - 1;
	(*sample) -> object.offset = tree -> offset;
	(*sample) -> path = BK_STRING_INIT;
	BKStringAppendString (&(*sample) -> name, &auxString);

	for (node = tree -> subNode; node; node = node -> nextNode) {
		BKInt arg1, arg2;
		BKString const * str;
		BKString const * data;

		if (node -> type == BKTKTypeComment) {
			continue;
		}

		if (!keyvalLookup (miscNames, NUM_MISC_NAMES, &node -> name, &value, &flags)) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		if (flags & BKTKParserFlagIsGroup) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		switch (value) {
			case BKTKMiscData: {
				arg1 = nodeArgInt (node, 0, 0);
				data = nodeArgString (node, 2);
				value = parseDataParams (nodeArgString (node, 1));

				res = BKDataSetData (&(*sample) -> data, data -> str, (BKUInt) data -> len, arg1, value);

				if (res != 0) {
					printError (compiler, tree, "Error: failed to set waveform (%s)", BKStatusGetName (res));
					res = -1;
					goto cleanup;
				}

				break;
			}
			case BKTKMiscLoad: {
				str = nodeArgString (node, 0);

				if (strcmp ((char *) str -> str, "wav") != 0) {
					printError (compiler, node, "Error: unknown file format '%s'; only 'wav' is supported at the moment",
						BKTKCompilerEscapeString (compiler, str));
					res = -1;
					goto cleanup;
					break;
				}

				str = nodeArgString (node, 1);
				BKStringEmpty (&(*sample) -> path);
				BKStringAppendString (&(*sample) -> path, str);
				break;
			}
			case BKTKMiscPitch: {
				arg1 = nodeArgInt (node, 0, 0);
				(*sample) -> pitch = arg1 * PITCH_UNIT;
				break;
			}
			case BKTKMiscSampleRepeat: {
				name = nodeArgString (node, 0);

				if (!keyvalLookup (repeatNames, NUM_REPEAT_NAMES, name, &arg1, NULL)) {
					printError (compiler, node, "Error: expected repeat mode: 'no', 'rep', 'pal'");
					res = -1;
					goto cleanup;
				}

				(*sample) -> repeat = arg1;
				break;
			}
			case BKTKMiscSampleSustainRange: {
				arg1 = nodeArgInt (node, 0, 0);
				arg2 = nodeArgInt (node, 1, 0);
				(*sample) -> sustainRange [0] = arg1;
				(*sample) -> sustainRange [1] = arg2;
				break;
			}
			default: {
				printErrorUnexpectedCommand (compiler, node);
				break;
			}
		}
	}

	cleanup: {
		BKStringDispose (&auxString);

		return res;
	}
}

static BKInt BKTKCompilerCompileGroup (BKTKCompiler * compiler, BKTKParserNode const * tree, BKTKTrack * track, BKInt level)
{
	BKInt res;
	BKInt value;
	BKUInt flags;
	BKTKParserNode const * node;
	BKTKGroup * group;
	BKInt offset;
	uint32_t cmd;
	BKInt autoindex = 0;

	offset = nodeArgInt (tree, 0, -1);

	if (offset >= 0) {
		if (offset >= MAX_GROUPS) {
			printError (compiler, tree, "Error: group number %d exceeds maximum of %d", offset, MAX_GROUPS - 1);
			return -1;
		}
	}
	else {
		offset = firstUnusedSlot (&track -> groups);
		autoindex = 1;
	}

	group = BKTKCompilerTrackGroupAtOffset (track, offset, 1);

	if (!group) {
		printError (compiler, tree, "Error: allocation failed");
		return -1;
	}

	if (group -> object.object.flags & BKTKFlagUsed) {
		if (group -> object.object.flags & BKTKFlagAutoIndex) {
			printError (compiler, tree, "Error: group number '%d' defined with autoindex on line %d:%d but redefined",
				offset, group -> object.offset.lineno, group -> object.offset.colno);
		}
		else {
			printError (compiler, tree, "Error: group number '%d' defined on line %d:%d but redefined",
				offset, group -> object.offset.lineno, group -> object.offset.colno);
		}

		return -1;
	}

	group -> object.object.flags |= BKTKFlagUsed | BKBitCond (BKTKFlagAutoIndex, autoindex);
	group -> object.index  = offset;
	group -> object.offset = tree -> offset;

	for (node = tree -> subNode; node; node = node -> nextNode) {
		if (node -> type == BKTKTypeComment) {
			continue;
		}

		if (!keyvalLookup (cmdNames, NUM_CMD_NAMES, &node -> name, &value, &flags)) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		if (flags & BKTKParserFlagIsGroup) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		if ((res = BKTKCompilerCompileCommand (compiler, node, &group -> byteCode, value, level)) != 0) {
			return res;
		}
	}

	cmd = BKInstrMaskArg1Make (BKIntrReturn, 0);

	if (BKByteBufferAppendInt32 (&group -> byteCode, cmd) != 0) {
		printError (compiler, node, "Error: allocation failed");
		return -1;
	}

	return 0;
}

static BKInt BKTKCompilerCompileTrack (BKTKCompiler * compiler, BKTKParserNode const * tree, BKInt level)
{
	BKInt res;
	BKInt value = -1;
	BKUInt flags;
	BKTKParserNode const * node;
	BKTKTrack * track;
	BKString const * wavename;
	BKInt offset;
	uint32_t cmd;
	BKInt autoindex = 0;
	BKTKWaveform * waveform = NULL;
	BKInt waveformIdx;

	wavename = nodeArgString (tree, 0);

	if (!BKHashTableLookup (&compiler -> waveforms, (char *) wavename -> str, (void **) &waveform)) {
		keyvalLookup (waveformNames, NUM_WAVEFORM_NAMES, wavename, &value, NULL);
	}

	if (waveform) {
		waveformIdx = waveform -> object.index | BK_INTR_CUSTOM_WAVEFORM_FLAG;
	}
	else if (value > 0) {
		waveformIdx = value;
	}
	else {
		printError (compiler, tree, "Error: undefined waveform '%s'",
			BKTKCompilerEscapeString (compiler, wavename));
		return -1;
	}

	// 1 based index (0 is root)
	offset = nodeArgInt (tree, 1, -1) + 1;

	if (offset >= 1) {
		if (offset > MAX_TRACKS) {
			printError (compiler, tree, "Error: track number %d exceeds maximum of %d", offset - 1, MAX_TRACKS - 1);
			return -1;
		}
	}
	else {
		offset = firstUnusedSlot (&compiler -> tracks);
		autoindex = 1;
	}

	// offset 0 is global track
	track = BKTKCompilerTrackAtOffset (compiler, offset, 1);

	if (!track) {
		printError (compiler, tree, "Error: allocation failed");
		return -1;
	}

	if (track -> object.object.flags & BKTKFlagUsed) {
		if (track -> object.object.flags & BKTKFlagAutoIndex) {
			printError (compiler, tree, "Error: track number '%d' defined with autoindex on line %d:%d but redefined",
				offset - 1, track -> object.offset.lineno, track -> object.offset.colno);
		}
		else {
			printError (compiler, tree, "Error: track number '%d' defined on line %d:%d but redefined",
				offset - 1, track -> object.offset.lineno, track -> object.offset.colno);
		}

		return -1;
	}

	track -> object.object.flags |= BKTKFlagUsed | BKBitCond (BKTKFlagAutoIndex, autoindex);
	track -> object.index  = offset;
	track -> object.offset = tree -> offset;
	track -> waveform      = waveformIdx;

	cmd = BKInstrMaskArg1Make (BKIntrWaveform, waveformIdx);

	if (BKByteBufferAppendInt32 (&track -> byteCode, cmd) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		return -1;
	}

	cmd = BKInstrMaskArg1Make (BKIntrRepeatStart, 0);

	if (BKByteBufferAppendInt32 (&track -> byteCode, cmd) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		return -1;
	}

	for (node = tree -> subNode; node; node = node -> nextNode) {
		if (node -> type == BKTKTypeComment) {
			continue;
		}

		if (keyvalLookup (cmdNames, NUM_CMD_NAMES, &node -> name, &value, &flags)) {
			switch (value) {
				case BKIntrGroupDef: {
					if ((res = BKTKCompilerCompileGroup (compiler, node, track, level)) != 0) {
						return res;
					}
					break;
				}
				default: {
					if (node -> flags & BKTKParserFlagIsGroup) {
						printErrorUnexpectedCommand (compiler, node);
					}
					else {
						if ((res = BKTKCompilerCompileCommand (compiler, node, &track -> byteCode, value, level)) != 0) {
							return res;
						}
					}
				}
			}
		}
		else {
			printErrorUnexpectedCommand (compiler, node);
		}
	}

	cmd = BKInstrMaskArg1Make (BKIntrEnd, 0);

	if (BKByteBufferAppendInt32 (&track -> byteCode, cmd) != 0) {
		printError (compiler, node, "Error: allocation failed");
		return -1;
	}

	return 0;
}

static BKInt BKTKCompilerLinkByteCode (BKTKCompiler * compiler, BKByteBuffer * byteCode, BKTKTrack * track)
{
	void * opcode;
	void * opcodeEnd;
	BKInstrMask mask;
	BKTKOffset offset;
	BKInt index, index2;
	BKTKGroup * group;
	BKTKTrack * groupTrack;

	// make continous byte array for interpreter
	if (BKByteBufferMakeContinuous (byteCode) != 0) {
		return -1;
	}

	opcode = byteCode -> first->data;
	opcodeEnd = opcode + BKByteBufferSize (byteCode);

	while (opcode < opcodeEnd) {
		mask = BKReadIntrMask (&opcode);

		if (mask.arg1.cmd == BKIntrCall) {
			offset.lineno = BKReadIntrMask (&opcode).arg1.arg1;
			offset.colno = BKReadIntrMask (&opcode).arg1.arg1;
			index = mask.grp.idx1;
			index2 = mask.grp.idx2;

			switch (mask.grp.type) {
				case BKGroupIndexTypeLocal: {
					group = BKTKCompilerTrackGroupAtOffset (track, index, 0);

					if (!group || !(group -> object.object.flags & BKTKFlagUsed)) {
						BKStringAppendFormat (&compiler -> error, "Local group '%d' not defined on line %d:%d\n", index, offset.lineno, offset.colno);
						return -1;
					}
					break;
				}
				case BKGroupIndexTypeGlobal: {
					groupTrack = BKTKCompilerTrackAtOffset (compiler, 0, 0);
					group = BKTKCompilerTrackGroupAtOffset (groupTrack, index, 0);

					if (!group || !(group -> object.object.flags & BKTKFlagUsed)) {
						BKStringAppendFormat (&compiler -> error, "Global group '%d' not defined on line %d:%d\n", index, offset.lineno, offset.colno);
						return -1;
					}
					break;
				}
				case BKGroupIndexTypeTrack: {
					groupTrack = BKTKCompilerTrackAtOffset (compiler, index2, 0);

					if (!groupTrack|| !(groupTrack -> object.object.flags & BKTKFlagUsed)) {
						BKStringAppendFormat (&compiler -> error, "Track '%d' not defined on line %d:%d\n", index2 - 1, offset.lineno, offset.colno);
						return -1;
					}

					group = BKTKCompilerTrackGroupAtOffset (groupTrack, index, 0);

					if (!group|| !(group -> object.object.flags & BKTKFlagUsed)) {
						BKStringAppendFormat (&compiler -> error, "Group '%d' of track '%d' not defined on line %d:%d\n", index, index2 - 1, offset.lineno, offset.colno);
						return -1;
					}
					break;
				}
			}
		}
	}

	return 0;
}

static BKInt BKTKCompilerGroupLink (BKTKCompiler * compiler, BKTKGroup * group, BKTKTrack * track)
{
	return BKTKCompilerLinkByteCode (compiler, &group -> byteCode, track);
}

static BKInt BKTKCompilerTrackLink (BKTKCompiler * compiler, BKTKTrack * track)
{
	BKTKGroup * group;

	if (BKTKCompilerLinkByteCode (compiler, &track -> byteCode, track) != 0) {
		return -1;
	}

	for (BKUSize i = 0; i < track -> groups.len; i ++) {
		group = *(BKTKGroup **) BKArrayItemAt (&track -> groups, i);

		if (group) {
			if (BKTKCompilerGroupLink (compiler, group, track) != 0) {
				return -1;
			}
		}
	}

	return 0;
}

static BKInt BKTKCompilerLink (BKTKCompiler * compiler)
{
	BKTKTrack * track;

	for (BKUSize i = 0; i < compiler -> tracks.len; i ++) {
		track = *(BKTKTrack **) BKArrayItemAt (&compiler -> tracks, i);

		if (track) {
			if (BKTKCompilerTrackLink (compiler, track) != 0) {
				return -1;
			}
		}
	}

	return 0;
}

BKInt BKTKCompilerCompile (BKTKCompiler * compiler, BKTKParserNode const * tree)
{
	BKInt res = 0;
	BKInt value;
	BKUInt flags;
	BKTKParserNode const * node;
	BKTKTrack * globalTrack;
	uint32_t cmd;

	cmd = BKInstrMaskArg1Make (BKIntrWaveform, BK_SQUARE);
	globalTrack = BKTKCompilerTrackAtOffset (compiler, 0, 1);

	if (BKByteBufferAppendInt32 (&globalTrack -> byteCode, cmd) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}

	cmd = BKInstrMaskArg1Make (BKIntrRepeatStart, 0);

	if (BKByteBufferAppendInt32 (&globalTrack -> byteCode, cmd) != 0) {
		printError (compiler, tree, "Error: allocation failed");
		return -1;
	}

	for (node = tree; node; node = node -> nextNode) {
		if (node -> type == BKTKTypeComment) {
			continue;
		}

		if (!keyvalLookup (cmdNames, NUM_CMD_NAMES, &node -> name, &value, &flags)) {
			printErrorUnexpectedCommand (compiler, node);
			continue;
		}

		switch (value) {
			case BKIntrGroupDef: {
				if ((res = BKTKCompilerCompileGroup (compiler, node, globalTrack, 0)) != 0) {
					goto cleanup;
				}
				break;
			}
			case BKIntrInstrumentDef: {
				if ((res = BKTKCompilerCompileInstrument (compiler, node)) != 0) {
					goto cleanup;
				}
				break;
			}
			case BKIntrSampleDef: {
				if ((res = BKTKCompilerCompileSample (compiler, node)) != 0) {
					goto cleanup;
				}
				break;
			}
			case BKIntrTrackDef: {
				if ((res = BKTKCompilerCompileTrack (compiler, node, 1)) != 0) {
					goto cleanup;
				}
				break;
			}
			case BKIntrWaveformDef: {
				if ((res = BKTKCompilerCompileWaveform (compiler, node)) != 0) {
					goto cleanup;
				}
				break;
			}
			default: {
				if (node -> flags & BKTKParserFlagIsGroup) {
					printErrorUnexpectedCommand (compiler, node);
				}
				else if ((res = BKTKCompilerCompileCommand (compiler, node, &globalTrack -> byteCode, value, 0)) != 0) {
					goto cleanup;
				}
				break;
			}
		}
	}

	cmd = BKInstrMaskArg1Make (BKIntrEnd, 0);

	if (BKByteBufferAppendInt32 (&globalTrack -> byteCode, cmd) != 0) {
		printError (compiler, node, "Error: allocation failed");
		res = BK_ALLOCATION_ERROR;
		goto cleanup;
	}

	if (BKTKCompilerLink (compiler) != 0) {
		res = -1;
		goto cleanup;
	}

	cleanup: {
		return res;
	}
}

BKInt BKTKCompilerReset (BKTKCompiler * compiler)
{
	char const * key;
	BKTKTrack * track;
	BKTKInstrument * instr;
	BKTKWaveform * waveform;
	BKTKSample * sample;
	BKHashTableIterator itor;

	for (BKUSize i = 0; i < compiler -> tracks.len; i ++) {
		track = *(BKTKTrack **) BKArrayItemAt (&compiler -> tracks, i);
		BKDispose (track);
	}

	BKHashTableIteratorInit (&itor, &compiler -> instruments);

	while (BKHashTableIteratorNext (&itor, &key, (void **) &instr)) {
		BKDispose (instr);
	}

	BKHashTableIteratorInit (&itor, &compiler -> waveforms);

	while (BKHashTableIteratorNext (&itor, &key, (void **) &waveform)) {
		BKDispose (waveform);
	}

	BKHashTableIteratorInit (&itor, &compiler -> samples);

	while (BKHashTableIteratorNext (&itor, &key, (void **) &sample)) {
		BKDispose (sample);
	}

	BKArrayEmpty (&compiler -> tracks);
	BKHashTableEmpty (&compiler -> instruments);
	BKHashTableEmpty (&compiler -> waveforms);
	BKHashTableEmpty (&compiler -> samples);
	BKStringEmpty (&compiler -> auxString);
	BKStringEmpty (&compiler -> error);

	compiler -> lineno = 0;
	compiler -> info = (BKTKFileInfo) {0};

	// reserve global track
	track = BKTKCompilerTrackAtOffset (compiler, 0, 1);

	if (!track) {
		return -1;
	}

	track -> object.object.flags |= BKTKFlagUsed;

	return 0;
}

static void BKTKCompilerDispose (BKTKCompiler * compiler)
{
	BKTKCompilerReset (compiler);
	
	BKArrayDispose (&compiler -> tracks);
	BKHashTableDispose (&compiler -> instruments);
	BKHashTableDispose (&compiler -> waveforms);
	BKHashTableDispose (&compiler -> samples);
	BKStringDispose (&compiler -> auxString);
	BKStringDispose (&compiler -> error);
}

BKClass const BKTKCompilerClass =
{
	.instanceSize = sizeof (BKTKCompiler),
	.dispose      = (void *) BKTKCompilerDispose,
};
