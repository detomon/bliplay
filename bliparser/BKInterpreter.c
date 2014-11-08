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

#include "BKInterpreter.h"
#include "BKTone.h"

enum {
	BKIntrEventStep    = 1 << 0,
	BKIntrEventAttack  = 1 << 1,
	BKIntrEventRelease = 1 << 2,
	BKIntrEventMute    = 1 << 3,
	BKIntrEventJump    = 1 << 4,
};

extern BKClass BKInterpreterClass;

static BKTickEvent * BKInterpreterEventGet (BKInterpreter * interpreter, BKInt eventsMaks)
{
	BKTickEvent * tickEvent;

	for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
		tickEvent = & interpreter -> events [i];

		if (tickEvent -> event & eventsMaks)
			return tickEvent;
	}

	return NULL;
}

static void BKInterpreterEventsUnset (BKInterpreter * interpreter, BKInt eventMask)
{
	BKSize size;
	BKTickEvent * tickEvent;

	if (eventMask & BKIntrEventAttack) {
		interpreter -> object.flags &= ~BKInterpreterFlagHasAttackEvent;
		interpreter -> nextNoteIndex = 0;
	}

	// remove events
	for (BKInt i = 0; i < interpreter -> numEvents;) {
		tickEvent = & interpreter -> events [i];

		if (tickEvent -> event & eventMask) {
			size = sizeof (BKTickEvent) * (interpreter -> numEvents - i - 1);
			memmove (tickEvent,	& tickEvent [1], size);
			interpreter -> numEvents --;
		}
		else {
			i ++;
		}
	}
}

static BKInt BKInterpreterEventSet (BKInterpreter * interpreter, BKInt event, BKInt ticks)
{
	BKTickEvent * tickEvent, * otherEvent;

	if (ticks == 0) {
		BKInterpreterEventsUnset (interpreter, event);
		return 0;
	}

	tickEvent = BKInterpreterEventGet (interpreter, event);

	if (tickEvent == NULL) {
		if (interpreter -> numEvents < BK_INTR_MAX_EVENTS) {
			tickEvent = & interpreter -> events [interpreter -> numEvents];
			interpreter -> numEvents ++;
		}
		else {
			return -1;
		}
	}

	switch (event) {
		case BKIntrEventAttack: {
			interpreter -> object.flags |= BKInterpreterFlagHasAttackEvent;
			break;
		}
		case BKIntrEventStep: {
			// other events can't happen after step event
			for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
				otherEvent = & interpreter -> events [i];

				if (otherEvent -> event == BKIntrEventJump)
					continue;

				if (otherEvent -> ticks > ticks)
					otherEvent -> ticks = ticks;
			}
			break;
		}
	}

	tickEvent -> event = event;
	tickEvent -> ticks = ticks;

	return 0;
}

static BKTickEvent * BKInterpreterEventGetNext (BKInterpreter * interpreter)
{
	BKInt ticks = BK_INT_MAX;
	BKTickEvent * tickEvent, * nextEvent = NULL;

	for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
		tickEvent = & interpreter -> events [i];

		if (tickEvent -> ticks < ticks) {
			ticks     = tickEvent -> ticks;
			nextEvent = tickEvent;
		}
	}

	if (nextEvent) {
		if (nextEvent -> event == BKIntrEventJump && interpreter -> numEvents == 1)
			nextEvent = NULL;
	}

	return nextEvent;
}

static void BKInterpreterEventsAdvance (BKInterpreter * interpreter, BKInt ticks)
{
	BKTickEvent * tickEvent;

	for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
		tickEvent = & interpreter -> events [i];

		if (tickEvent -> ticks > 0)
			tickEvent -> ticks -= ticks;
	}
}

BKInt BKInterpreterInit (BKInterpreter * interpreter)
{
	if (BKObjectInit (interpreter, & BKInterpreterClass, sizeof (*interpreter))) {
		return -1;
	}

	interpreter -> stackPtr = interpreter -> stack;
	interpreter -> stackEnd = (void *) interpreter -> stack + sizeof (interpreter -> stack);

	return 0;
}

static uint8_t BKInterpreterOpcodeReadInt8 (void const ** opcode)
{
	uint16_t i = * (uint8_t *) (* opcode);
	(* opcode) += 1;

	return i;
}

static uint16_t BKInterpreterOpcodeReadInt16 (void const ** opcode)
{
	uint16_t i = * (uint16_t *) (* opcode);
	(* opcode) += 2;

	return i;
}

static uint32_t BKInterpreterOpcodeReadInt32 (void const ** opcode)
{
	uint32_t i = * (uint32_t *) (* opcode);
	(* opcode) += 4;

	return i;
}

BKInt BKInterpreterTrackAdvance (BKInterpreter * interpreter, BKTrack * track, BKInt * outTicks)
{
	BKInt         cmd;
	BKInt         value0;
	BKInt         numSteps = 1;
	BKInt         run = 1;
	void        * opcode;
	BKInt         result = 1;
	BKTickEvent * tickEvent;

	opcode   = interpreter -> opcodePtr;
	numSteps = interpreter -> numSteps;

	if (numSteps) {
		BKInterpreterEventsAdvance (interpreter, numSteps);

		do {
			tickEvent = BKInterpreterEventGetNext (interpreter);

			if (tickEvent) {
				numSteps = tickEvent -> ticks;

				if (tickEvent -> ticks <= 0) {
					switch (tickEvent -> event) {
						case BKIntrEventStep: {
							// do nothing
							break;
						}
						case BKIntrEventAttack: {
							BKSetPtr (track, BK_ARPEGGIO, NULL, 0);

							for (BKInt i = 0; i < interpreter -> nextNoteIndex; i ++) {
								BKSetAttr (track, BK_NOTE, interpreter -> nextNotes [i]);
							}

							if (interpreter -> object.flags & BKInterpreterFlagHasArpeggio) {
								BKSetPtr (track, BK_ARPEGGIO, interpreter -> nextArpeggio, sizeof (interpreter -> nextArpeggio));
							}

							break;
						}
						case BKIntrEventRelease: {
							BKSetAttr (track, BK_NOTE, BK_NOTE_RELEASE);
							break;
						}
						case BKIntrEventMute: {
							BKSetAttr (track, BK_NOTE, BK_NOTE_MUTE);
							BKSetPtr (track, BK_ARPEGGIO, NULL, 0);
							break;
						}
						/*case BKIntrEventJump: {
							BKInterpreterEventSet (interpreter, ~0, 0);

							interpreter -> jumpStackPtr --;
							interpreter -> opcodePtr = interpreter -> opcode + interpreter -> jumpStackPtr -> opcodeAddr;
							interpreter -> stackPtr  = interpreter -> jumpStackPtr -> stackPtr;

							opcode = interpreter -> opcodePtr;

							BKTrackClear (track);

							numSteps = 0;
							tickEvent = NULL;

							break;
						}*/
					}

					interpreter -> nextNoteIndex = 0;

					if (tickEvent) {
						BKInterpreterEventSet (interpreter, tickEvent -> event, 0);
					}
				}
				else {
					break;
				}
			}
			else {
				numSteps = 0;
			}
		}
		while (tickEvent);

		if (numSteps) {
			interpreter -> numSteps = numSteps;
			(* outTicks) = numSteps;

			return 1;
		}
	}

	do {
		cmd = BKInterpreterOpcodeReadInt8 ((void *) & opcode);

		switch (cmd) {
			case BKIntrAttack: {
				value0 = BKInterpreterOpcodeReadInt32 ((void *) & opcode);

				if (interpreter -> object.flags & BKInterpreterFlagHasAttackEvent) {
					// overwrite last note value when more than 2
					interpreter -> nextNoteIndex = BKMin (interpreter -> nextNoteIndex, 1);
					interpreter -> nextNotes [interpreter -> nextNoteIndex] = value0;
					interpreter -> nextNoteIndex ++;
				}
				else {
					BKSetPtr (track, BK_ARPEGGIO, NULL, 0);
					BKSetAttr (track, BK_NOTE, value0);
				}

				break;
			}
			case BKIntrAttackTicks: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKInterpreterEventSet (interpreter, BKIntrEventAttack, value0);
				break;
			}
			case BKIntrArpeggio: {
				BKInt arpeggio [1 + BK_MAX_ARPEGGIO];

				value0 = BKInterpreterOpcodeReadInt8 ((void *) & opcode);

				if (value0) {
					interpreter -> object.flags |= BKInterpreterFlagHasArpeggio;
				} else {
					interpreter -> object.flags &= ~BKInterpreterFlagHasArpeggio;
				}

				if (interpreter -> object.flags & BKInterpreterFlagHasAttackEvent) {
					interpreter -> nextArpeggio [0] = value0;
					memcpy (& interpreter -> nextArpeggio [1], opcode, value0 * sizeof (BKInt));
				}
				else {
					arpeggio [0] = value0;
					memcpy (& arpeggio [1], opcode, value0 * sizeof (BKInt));
					BKSetPtr (track, BK_ARPEGGIO, arpeggio, sizeof (arpeggio));
				}

				opcode += 4 * value0;
				break;
			}
			case BKIntrArpeggioSpeed: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKSetAttr (track, BK_ARPEGGIO_DIVIDER, value0);
				break;
			}
			case BKIntrRelease: {
				BKInterpreterEventSet (interpreter, BKIntrEventRelease | BKIntrEventMute, 0);
				BKSetAttr (track, BK_NOTE, BK_NOTE_RELEASE);
				interpreter -> nextNoteIndex = 0;
				break;
			}
			case BKIntrReleaseTicks: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKInterpreterEventSet (interpreter, BKIntrEventMute, 0);
				BKInterpreterEventSet (interpreter, BKIntrEventRelease, value0);
				break;
			}
			case BKIntrMute: {
				BKInterpreterEventSet (interpreter, BKIntrEventRelease | BKIntrEventMute, 0);
				BKSetAttr (track, BK_NOTE, BK_NOTE_MUTE);
				interpreter -> nextNoteIndex = 0;
				break;
			}
			case BKIntrMuteTicks: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKInterpreterEventSet (interpreter, BKIntrEventRelease, 0);
				BKInterpreterEventSet (interpreter, BKIntrEventMute, value0);
				break;
			}
			case BKIntrVolume: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKSetAttr (track, BK_VOLUME, value0);
				break;
			}
			case BKIntrMasterVolume: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKSetAttr (track, BK_MASTER_VOLUME, value0);
				break;
			}
			case BKIntrPanning: {
				value0 = (int16_t) BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKSetAttr (track, BK_PANNING, value0);
				break;
			}
			case BKIntrPitch: {
				value0 = BKInterpreterOpcodeReadInt32 ((void *) & opcode);
				BKSetAttr (track, BK_PITCH, value0);
				break;
			}
			case BKIntrTicks: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKInterpreterEventSet (interpreter, BKIntrEventStep, value0);
				run = 0;
				break;
			}
			case BKIntrStep: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKInterpreterEventSet (interpreter, BKIntrEventStep, value0 * interpreter -> stepTickCount);
				run = 0;
				break;
			}
			case BKIntrStepTicks: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				interpreter -> stepTickCount = value0;
				break;
			}
			case BKIntrEffect: {
				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);
				BKTrackSetEffect (track, value0, opcode, sizeof (BKInt [3]));
				opcode += 4 * 3;
				break;
			}
			case BKIntrDutyCycle: {
				value0 = BKInterpreterOpcodeReadInt8 ((void *) & opcode);
				BKSetAttr (track, BK_DUTY_CYCLE, value0);
				break;
			}
			case BKIntrPhaseWrap: {
				value0 = BKInterpreterOpcodeReadInt32 ((void *) & opcode);
				BKSetAttr (track, BK_PHASE_WRAP, value0);
				break;
			}
			case BKIntrInstrument: {
				BKInstrument * instr = NULL;

				value0 = (int16_t) BKInterpreterOpcodeReadInt16 ((void *) & opcode);

				if (value0 > -1) {
					if (value0 < interpreter -> instruments -> length) {
						instr = ((BKInstrument **) interpreter -> instruments -> items)[value0];
					}
				}

				BKSetPtr (track, BK_INSTRUMENT, instr, sizeof (void *));
				break;
			}
			case BKIntrWaveform: {
				BKData * waveform = NULL;
				BKInt masterVolume = 0;

				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);

				if (value0 & BK_INTR_CUSTOM_WAVEFORM_FLAG) {
					value0 &= ~BK_INTR_CUSTOM_WAVEFORM_FLAG;

					if (value0 < interpreter -> waveforms -> length) {
						waveform = ((BKData **) interpreter -> waveforms -> items)[value0];
						value0 = BK_CUSTOM;
					}

					if (waveform == NULL) {
						value0 = BK_SQUARE;
					}
				}

				if (value0 == BK_CUSTOM) {
					BKSetPtr (track, BK_WAVEFORM, waveform, sizeof (void *));
				}
				else {
					BKSetAttr (track, BK_WAVEFORM, value0);
				}

				switch (value0) {
					case BK_SQUARE:
					case BK_NOISE:
					case BK_SAWTOOTH:
					case BK_CUSTOM: {
						masterVolume = BK_MAX_VOLUME * 0.15;
						break;
					}
					case BK_TRIANGLE:
					case BK_SINE: {
						masterVolume = BK_MAX_VOLUME * 0.30;
						break;
					}
				}

				BKSetAttr (track, BK_MASTER_VOLUME, masterVolume);

				break;
			}
			case BKIntrSample: {
				BKData * sample = NULL;

				value0 = BKInterpreterOpcodeReadInt16 ((void *) & opcode);

				if (value0 > -1) {
					if (value0 < interpreter -> samples -> length) {
						sample = ((BKData **) interpreter -> samples -> items)[value0];
					}
				}

				BKSetPtr (track, BK_SAMPLE, sample, sizeof (void *));
				break;
			}
			case BKIntrSampleRepeat: {
				value0 = BKInterpreterOpcodeReadInt8 ((void *) & opcode);
				BKSetAttr (track, BK_SAMPLE_REPEAT, value0);
				break;
			}
			case BKIntrSampleRange: {
				BKSetPtr (track, BK_SAMPLE_RANGE, opcode, sizeof (BKInt [2]));
				opcode += 4 * 2;
				break;
			}
			case BKIntrReturn: {
				if (interpreter -> stackPtr > interpreter -> stack) {
					value0 = * (-- interpreter -> stackPtr);
					opcode = interpreter -> opcode + value0;
				}
				break;
			}
			case BKIntrCall: {
				value0 = BKInterpreterOpcodeReadInt32 ((void *) & opcode);
				//value1 = BKInterpreterOpcodeReadInt32 ((void *) & opcode);

				/*if (value1) {
					if (interpreter -> jumpStackPtr < interpreter -> jumpStackEnd) {
						BKInterpreterEventSet (interpreter, BKIntrEventJump, value1 * interpreter -> stepTickCount);

						interpreter -> jumpStackPtr -> ticks      = value1;
						interpreter -> jumpStackPtr -> opcodeAddr = opcode - interpreter -> opcode;
						interpreter -> jumpStackPtr -> stackPtr   = interpreter -> stackPtr;
						interpreter -> jumpStackPtr ++;
					}
				}*/

				if (interpreter -> stackPtr < interpreter -> stackEnd) {
					* (interpreter -> stackPtr ++) = opcode - interpreter -> opcode;
					opcode = interpreter -> opcode + value0;
				}

				break;
			}
			case BKIntrSetRepeatStart: {
				interpreter -> repeatStartAddr = opcode - interpreter -> opcode;
				break;
			}
			case BKIntrJump: {
				value0 = BKInterpreterOpcodeReadInt32 ((void *) & opcode);

				// jump to repeat mark
				if (value0 == -1) {
					value0 = interpreter -> repeatStartAddr;
					interpreter -> object.flags |= BKInterpreterFlagHasRepeated;
				}

				opcode = interpreter -> opcode + value0;
				break;
			}
			case BKIntrEnd: {
				BKInterpreterEventSet (interpreter, BKIntrEventStep, BK_INT_MAX);
				interpreter -> object.flags |= BKInterpreterFlagHasStopped;
				opcode --; // Repeat command forever
				run = 0;
				result = 0;
				break;
			}
		}
	}
	while (run);

	numSteps  = 1;  // default steps
	tickEvent = BKInterpreterEventGetNext (interpreter);

	if (tickEvent)
		numSteps = tickEvent -> ticks;

	interpreter -> numSteps  = numSteps;
	interpreter -> opcodePtr = opcode;

	(* outTicks) = numSteps;

	return result;
}

static void BKInterpreterDispose (BKInterpreter * interpreter)
{
}

void BKInterpreterReset (BKInterpreter * interpreter)
{
	interpreter -> object.flags   &= ~BKObjectFlagUsableMask;
	interpreter -> numSteps        = 0;
	interpreter -> opcodePtr       = interpreter -> opcode;
	interpreter -> stackPtr        = interpreter -> stack;
	interpreter -> stackEnd        = (void *) interpreter -> stack + sizeof (interpreter -> stack);
	//interpreter -> jumpStackPtr    = interpreter -> jumpStack;
	//interpreter -> jumpStackEnd    = (void *) interpreter -> jumpStack + sizeof (interpreter -> jumpStack);
	interpreter -> numEvents       = 0;
	interpreter -> nextNoteIndex   = 0;
	interpreter -> repeatStartAddr = 0;
}

BKClass BKInterpreterClass =
{
	.instanceSize = sizeof (BKInterpreter),
	.dispose      = (BKDisposeFunc) BKInterpreterDispose,
};
