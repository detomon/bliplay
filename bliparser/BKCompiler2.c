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
#include "BKTone.h"
#include "BKCompiler2.h"

#define MAX_GROUPS 255
#define VOLUME_UNIT (BK_MAX_VOLUME / 255)
#define PITCH_UNIT (BK_FINT20_UNIT / 100)

enum
{
	BKCompiler2FlagAllocated = 1 << 0,
	BKCompiler2FlagOpenGroup = 1 << 1,
	BKCompiler2FlagArpeggio  = 1 << 2,
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
} BKCompiler2Group;

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
	{"a",     BKIntrAttack},
	{"as",    BKIntrArpeggioSpeed},
	{"at",    BKIntrAttackTicks},
	{"d",     BKIntrSample},
	{"dc",    BKIntrDutyCycle},
	{"dn",    BKIntrSampleRange},
	{"dr",    BKIntrSampleRepeat},
	{"e",     BKIntrEffect},
	{"g",     BKIntrGroupJump},
	{"grp",   BKIntrGroupDef, BKCompiler2FlagOpenGroup},
	{"i",     BKIntrInstrument},
	{"instr", BKIntrInstrumentDef, BKCompiler2FlagOpenGroup},
	{"m",     BKIntrMute},
	{"mt",    BKIntrMuteTicks},
	{"p",     BKIntrPanning},
	{"pt",    BKIntrPitch},
	{"pw",    BKIntrPhaseWrap},
	{"r",     BKIntrRelease},
	{"rt",    BKIntrReleaseTicks},
	{"s",     BKIntrStep},
	{"samp",  BKIntrSampleDef},
	{"st",    BKIntrStepTicks},
	{"t",     BKIntrTicks},
	{"track", BKIntrTrackDef, BKCompiler2FlagOpenGroup},
	{"v",     BKIntrVolume},
	{"vm",    BKIntrMasterVolume},
	{"w",     BKIntrWaveform},
	{"wave",  BKIntrWaveformDef, BKCompiler2FlagOpenGroup},
	{"x",     BKIntrRepeat},
	{"xb",    BKIntrSetRepeatStart},
	{"z",     BKIntrEnd},
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

#define NUM_WAVEFORM_NAMES (sizeof (waveformNames) / sizeof (strval))
#define NUM_CMD_NAMES (sizeof (cmdNames) / sizeof (strval))
#define NUM_EFFECT_NAMES (sizeof (effectNames) / sizeof (strval))
#define NUM_NOTE_NAMES (sizeof (noteNames) / sizeof (strval))

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

	track -> flags |= BKCompiler2FlagAllocated;
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

BKInt BKCompiler2Init (BKCompiler2 * compiler)
{
	memset (compiler, 0, sizeof (* compiler));

	if (BKArrayInit (& compiler -> groupStack, sizeof (BKCompiler2Group), 8) < 0) {
		BKCompiler2Dispose (& compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> tracks, sizeof (BKCompilerTrack *), 8) < 0) {
		BKCompiler2Dispose (& compiler);
		return -1;
	}

	if (BKCompilerTrackInit (& compiler -> globalTrack) < 0) {
		BKCompiler2Dispose (& compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> instruments, sizeof (BKInstrument *), 4) < 0) {
		BKCompiler2Dispose (& compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> waveforms, sizeof (BKData *), 4) < 0) {
		BKCompiler2Dispose (& compiler);
		return -1;
	}

	if (BKArrayInit (& compiler -> samples, sizeof (BKData *), 4) < 0) {
		BKCompiler2Dispose (& compiler);
		return -1;
	}

	BKCompiler2Reset (compiler, 0);

	return 0;
}

void BKCompiler2Dispose (BKCompiler2 * compiler)
{
	BKCompiler2Reset (compiler, 0);
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
static BKInt BKCompiler2ParseNote (char const * name)
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

static BKByteBuffer * BKCompiler2GetCmdGroupForIndex (BKCompiler2 * compiler, BKInt index)
{
	BKByteBuffer     * buffer = NULL;
	BKCompiler2Group * group  = BKArrayGetLastItem (& compiler -> groupStack);
	BKCompilerTrack  * track  = group -> track;

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
				return -1;
			}
		}

		if (BKByteBufferAlloc (& buffer, 0) < 0) {
			return -1;
		}

		// append new buffer
		if (BKArrayPush (& track -> cmdGroups, & buffer) < 0) {
			return -1;
		}
	}

	return buffer;
}

static BKInt BKCompiler2PushCommandInstrument (BKCompiler2 * compiler, BKSTCmd const * cmd)
{
	printf("-instrument\n");
	return 0;
}

static BKInt BKCompiler2PushCommandSample (BKCompiler2 * compiler, BKSTCmd const * cmd)
{
	printf("-sample\n");
	return 0;
}

static BKInt BKCompiler2PushCommandTrack (BKCompiler2 * compiler, BKSTCmd const * cmd)
{
	char const       * arg0str;
	BKInt              values [2];
	BKInt              args [3];
	BKInt              numArgs, value;
	BKInstruction      instr;
	BKCompiler2Group * group = BKArrayGetLastItem (& compiler -> groupStack);
	BKByteBuffer     * cmds  = group -> cmdBuffer;

	printf(">> grp: %lu %s\n", group, cmd -> name);

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
			values [0] = BKCompiler2ParseNote (arg0str);

			if (values [0] > -1) {
				BKByteBufferAppendInt8 (cmds, instr);
				BKByteBufferAppendInt32 (cmds, values [0]);

				// set arpeggio
				if (numArgs > 1) {
					numArgs = BKMin (numArgs, BK_MAX_ARPEGGIO);

					BKByteBufferAppendInt8 (cmds, BKIntrArpeggio);
					BKByteBufferAppendInt8 (cmds, (BKInt) numArgs);

					for (BKInt j = 0; j < numArgs; j ++) {
						values [1] = BKCompiler2ParseNote (cmd -> args [j].arg);

						if (values [1] < 0) {
							values [1] = 0;
						}

						BKByteBufferAppendInt32 (cmds, values [1] - values [0]);
					}

					compiler -> flags |= BKCompiler2FlagArpeggio;
				}
				// disable arpeggio
				else {
					BKByteBufferAppendInt8 (cmds, BKIntrArpeggio);
					BKByteBufferAppendInt8 (cmds, 0);
					compiler -> flags &= ~BKCompiler2FlagArpeggio;
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
			if (compiler -> flags & BKCompiler2FlagArpeggio) {
				if (instr == BKIntrMute) {
					BKByteBufferAppendInt8 (cmds, BKIntrArpeggio);
					BKByteBufferAppendInt8 (cmds, 0);
					compiler -> flags &= ~BKCompiler2FlagArpeggio;
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
				BKByteBufferAppendInt32 (cmds, values [0]);
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
			BKByteBufferAppendInt16 (cmds, atoix (arg0str, 0) * VOLUME_UNIT);
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

static BKInt BKCompiler2PushCommandWaveform (BKCompiler2 * compiler, BKSTCmd const * cmd)
{
	printf("-waveform\n");
	return 0;
}

BKInt BKCompiler2PushCommand (BKCompiler2 * compiler, BKSTCmd const * cmd)
{
	BKInt              index, value;
	BKUInt             flags;
	BKByteBuffer     * buffer;
	BKInstruction      instr;
	BKCompiler2Group * group, * newGroup;
	BKCompilerTrack  * track;

	switch (cmd -> token) {
		case BKSTTokenValue: {
			// ignore current group
			if (compiler -> ignoreGroupLevel > -1) {
				return 0;
			}

			group = BKArrayGetLastItem (& compiler -> groupStack);

			switch (group -> groupType) {
				case BKIntrInstrumentDef: {
					if (BKCompiler2PushCommandInstrument (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
				case BKIntrSampleDef: {
					if (BKCompiler2PushCommandSample (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
				default:
				case BKIntrGroupDef:
				case BKIntrTrackDef: {
					if (BKCompiler2PushCommandTrack (compiler, cmd) < 0) {
						return -1;
					}
					break;
				}
				case BKIntrWaveformDef: {
					if (BKCompiler2PushCommandWaveform (compiler, cmd) < 0) {
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

			printf("++ trk: %lu grp: %lu\n", newGroup -> track, newGroup);

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
					index = atoix (cmd -> args [0].arg, -1);

					if (index >= MAX_GROUPS) {
						fprintf (stderr, "Group number is limited to %u on line %u:%u\n", MAX_GROUPS, cmd -> lineno, cmd -> colno);
						compiler -> ignoreGroupLevel = compiler -> groupStack.length - 1;
						return 0;
					}

					buffer = BKCompiler2GetCmdGroupForIndex (compiler, index);

					if (buffer == NULL) {
						return -1;
					}

					newGroup -> cmdBuffer = buffer;
					break;
				}
				case BKIntrInstrumentDef: {

					break;
				}
				case BKIntrSampleDef: {

					break;
				}
				case BKIntrTrackDef: {
					if (BKCompilerTrackAlloc (& track) < 0) {
						return -1;
					}

					if (BKArrayPush (& compiler -> tracks, & track) < 0) {
						return -1;
					}

					newGroup -> track     = track;
					newGroup -> cmdBuffer = & track -> globalCmds;
					break;
				}
				case BKIntrWaveformDef: {

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

static BKInt BKCompiler2ByteCodeLink (BKCompilerTrack * track, BKByteBuffer * group, BKArray * groupOffsets)
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

					(* (uint8_t *) opcode ++) = BKIntrJump;
					(* (uint32_t *) opcode)   = idx;
				}
				else {
					fprintf (stderr, "Undefined group number %u (%u)\n", arg, groupOffsets -> length);
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
				cmdSize = 4 + 4 + 4 + 4;
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
			case BKIntrInstrument: {
				cmdSize = 2;
				break;
			}
			case BKIntrWaveform: {
				cmdSize = 2;
				break;
			}
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
 * Combine group commands into globa commands of track
 */
static BKInt BKCompiler2TrackLink (BKCompilerTrack * track)
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
	if (BKCompiler2ByteCodeLink (track, & track -> globalCmds, & groupOffsets) < 0) {
		return -1;
	}

	for (BKInt i = 0; i < track -> cmdGroups.length; i ++) {
		BKArrayGetItemAtIndexCopy (& track -> cmdGroups, i, & group);

		if (group == NULL) {
			continue;
		}

		// link group
		if (BKCompiler2ByteCodeLink (track, group, & groupOffsets) < 0) {
			return -1;
		}

		// append group
		if (BKByteBufferAppendPtr (& track -> globalCmds, group -> data, group -> size) < 0) {
			return -1;
		}

		//BKByteBufferEmpty (group, 0);
	}

	BKArrayDispose (& groupOffsets);

	return 0;
}

/**
 * Combine each track's commands into the global commands
 */
static BKInt BKCompiler2Link (BKCompiler2 * compiler)
{
	BKCompilerTrack * track;

	track = & compiler -> globalTrack;

	if (BKCompiler2TrackLink (track) < 0) {
		return -1;
	}

	for (BKInt i = 0; i < compiler -> tracks.length; i ++) {
		BKArrayGetItemAtIndexCopy (& compiler -> tracks, i, & track);

		if (BKCompiler2TrackLink (track) < 0) {
			return -1;
		}
	}

	return 0;
}

BKInt BKCompiler2Terminate (BKCompiler2 * compiler, BKEnum options)
{
	if (compiler -> groupStack.length > 1) {
		fprintf (stderr, "Unterminated groups (%lu)\n", compiler -> groupStack.length);
		return -1;
	}

	// combine commands and group commands into one array
	if (BKCompiler2Link (compiler) < 0) {
		return -1;
	}

	return 0;
}

static BKInt BKCompiler2Parse (BKCompiler2 * compiler, BKSTParser * parser)
{
	BKSTCmd cmd;

	while (BKSTParserNextCommand (parser, & cmd)) {
		if (BKCompiler2PushCommand (compiler, & cmd) < 0) {
			return -1;
		}
	}

	return 0;
}

BKInt BKCompiler2Compile (BKCompiler2 * compiler, BKSTParser * parser, BKEnum options)
{
	if (BKCompiler2Parse (compiler, parser) < 0) {
		return -1;
	}

	if (BKCompiler2Terminate (compiler, options) < 0) {
		return -1;
	}

	return 0;
}

void BKCompiler2Reset (BKCompiler2 * compiler, BKInt keepData)
{
	BKByteBuffer     * buffer;
	BKCompiler2Group * group;
	BKCompilerTrack  * track;

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

	BKArrayEmpty (& compiler -> instruments, keepData);
	BKArrayEmpty (& compiler -> waveforms, keepData);
	BKArrayEmpty (& compiler -> samples, keepData);
}
