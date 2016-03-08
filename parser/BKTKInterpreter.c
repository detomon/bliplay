#include "BKTone.h"
#include "BKTKContext.h"
#include "BKTKInterpreter.h"

extern BKClass const BKTKInterpreterClass;

enum {
	BKIntrEventStep    = 1 << 0,
	BKIntrEventAttack  = 1 << 1,
	BKIntrEventRelease = 1 << 2,
	BKIntrEventMute    = 1 << 3,
};

static BKTKTickEvent * BKTKInterpreterEventGet (BKTKInterpreter * interpreter, BKInt eventsMaks)
{
	BKTKTickEvent * tickEvent;

	for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
		tickEvent = &interpreter -> events [i];

		if (tickEvent -> event & eventsMaks)
			return tickEvent;
	}

	return NULL;
}

static void BKTKInterpreterEventsUnset (BKTKInterpreter * interpreter, BKInt eventMask)
{
	BKSize size;
	BKTKTickEvent * tickEvent;

	if (eventMask & BKIntrEventAttack) {
		interpreter -> object.flags &= ~BKTKInterpreterFlagHasAttackEvent;
		interpreter -> nextNoteIndex = 0;
	}

	// remove events
	for (BKInt i = 0; i < interpreter -> numEvents;) {
		tickEvent = &interpreter -> events [i];

		if (tickEvent -> event & eventMask) {
			size = sizeof (BKTKTickEvent) * (interpreter -> numEvents - i - 1);
			memmove (tickEvent,	&tickEvent [1], size);
			interpreter -> numEvents --;
		}
		else {
			i ++;
		}
	}
}

static BKInt BKTKInterpreterEventSet (BKTKInterpreter * interpreter, BKInt event, BKInt ticks)
{
	BKTKTickEvent * tickEvent, * otherEvent;

	if (ticks == 0) {
		BKTKInterpreterEventsUnset (interpreter, event);
		return 0;
	}

	tickEvent = BKTKInterpreterEventGet (interpreter, event);

	if (tickEvent == NULL) {
		if (interpreter -> numEvents < BK_INTR_MAX_EVENTS) {
			tickEvent = &interpreter -> events [interpreter -> numEvents];
			interpreter -> numEvents ++;
		}
		else {
			return -1;
		}
	}

	switch (event) {
		case BKIntrEventAttack: {
			interpreter -> object.flags |= BKTKInterpreterFlagHasAttackEvent;
			break;
		}
		case BKIntrEventStep: {
			// other events can't happen after step event
			for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
				otherEvent = &interpreter -> events [i];

				if (otherEvent -> ticks > ticks) {
					otherEvent -> ticks = ticks;
				}
			}
			break;
		}
	}

	tickEvent -> event = event;
	tickEvent -> ticks = ticks;

	return 0;
}

static BKTKTickEvent * BKTKInterpreterEventGetNext (BKTKInterpreter * interpreter)
{
	BKInt ticks = BK_INT_MAX;
	BKTKTickEvent * tickEvent, * nextEvent = NULL;

	for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
		tickEvent = &interpreter -> events [i];

		if (tickEvent -> ticks < ticks) {
			ticks     = tickEvent -> ticks;
			nextEvent = tickEvent;
		}
	}

	return nextEvent;
}

static void BKTKInterpreterEventsAdvance (BKTKInterpreter * interpreter, BKInt ticks)
{
	BKTKTickEvent * tickEvent;

	for (BKInt i = 0; i < interpreter -> numEvents; i ++) {
		tickEvent = &interpreter -> events [i];

		if (tickEvent -> ticks > 0)
			tickEvent -> ticks -= ticks;
	}
}

BKInt BKTKInterpreterInit (BKTKInterpreter * interpreter)
{
	if (BKObjectInit (interpreter, &BKTKInterpreterClass, sizeof (*interpreter))) {
		return -1;
	}

	BKTKInterpreterReset (interpreter);

	return 0;
}

BK_INLINE BKInstrMask BKReadIntrMask (void ** opcode)
{
	BKInstrMask mask = *(BKInstrMask *) *opcode;
	(* opcode) += sizeof (uint32_t);

	return mask;
}

BK_INLINE BKInt value2Pitch (BKInt value)
{
	return (BKInt) ((int64_t) value * BK_FINT20_UNIT / 100);
}

BKInt BKTKInterpreterAdvance (BKTKInterpreter * interpreter, BKTKTrack * ctx, BKInt * outTicks)
{
	BKInt           value0, value1;
	BKInt           numSteps = 1;
	BKInt           run = 1;
	void          * opcode;
	BKInt           result = 1;
	BKTKTickEvent * tickEvent;
	BKInstrMask     cmdMask, argMask;
	BKTrack       * track = &ctx -> renderTrack;

	opcode   = interpreter -> opcodePtr;
	numSteps = interpreter -> numSteps;

	if (numSteps) {
		BKTKInterpreterEventsAdvance (interpreter, numSteps);

		do {
			tickEvent = BKTKInterpreterEventGetNext (interpreter);

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

							if (interpreter -> object.flags & BKTKInterpreterFlagHasArpeggio) {
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
					}

					interpreter -> nextNoteIndex = 0;

					if (tickEvent) {
						BKTKInterpreterEventSet (interpreter, tickEvent -> event, 0);
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
			interpreter -> time += numSteps;
			(* outTicks) = numSteps;

			return 1;
		}
	}

	do {
		cmdMask = BKReadIntrMask (&opcode);

		switch (cmdMask.arg1.cmd) {
			case BKIntrAttack: {
				value0 = value2Pitch (cmdMask.arg1.arg1);

				if (interpreter -> object.flags & BKTKInterpreterFlagHasAttackEvent) {
					// overwrite last note value when more than 2
					interpreter -> nextNoteIndex = BKMin (interpreter -> nextNoteIndex, 1);
					interpreter -> nextNotes [interpreter -> nextNoteIndex] = value0;
					interpreter -> nextNoteIndex ++;
				}
				else {
					BKSetPtr (track, BK_ARPEGGIO, NULL, 0);
					BKSetAttr (track, BK_NOTE, value0);
				}

				interpreter -> object.flags &= ~BKTKInterpreterFlagHasArpeggio;

				break;
			}
			case BKIntrArpeggio: {
				BKInt arpeggio [1 + BK_MAX_ARPEGGIO];

				value0 = cmdMask.arg1.arg1;

				BKBitSetCond (interpreter -> object.flags, BKTKInterpreterFlagHasArpeggio, value0 != 0);

				arpeggio [0] = value0 + 1;
				arpeggio [1] = 0;

				for (BKInt i = 0; i < value0; i ++) {
					argMask = BKReadIntrMask (&opcode);
					arpeggio [i + 2] = value2Pitch (argMask.arg1.arg1);
				}

				if (interpreter -> object.flags & BKTKInterpreterFlagHasAttackEvent) {
					memcpy (interpreter -> nextArpeggio, arpeggio, (value0 + 2) * sizeof (BKInt));
				}
				else {
					BKSetPtr (track, BK_ARPEGGIO, arpeggio, sizeof (arpeggio));
				}

				break;
			}
			case BKIntrArpeggioSpeed: {
				value0 = cmdMask.arg1.arg1;

				if (value0 <= 0) {
					value0 = BK_DEFAULT_ARPEGGIO_DIVIDER;
				}

				BKSetAttr (track, BK_ARPEGGIO_DIVIDER, value0);
				break;
			}
			case BKIntrRelease: {
				BKTKInterpreterEventSet (interpreter, BKIntrEventRelease | BKIntrEventMute, 0);
				BKSetAttr (track, BK_NOTE, BK_NOTE_RELEASE);
				interpreter -> nextNoteIndex = 0;
				break;
			}
			case BKIntrMute: {
				BKTKInterpreterEventSet (interpreter, BKIntrEventRelease | BKIntrEventMute, 0);
				BKSetAttr (track, BK_NOTE, BK_NOTE_MUTE);
				interpreter -> nextNoteIndex = 0;
				break;
			}
			case BKIntrVolume: {
				value0 = cmdMask.arg1.arg1;
				BKSetAttr (track, BK_VOLUME, value0);
				break;
			}
			case BKIntrMasterVolume: {
				value0 = cmdMask.arg1.arg1;
				BKSetAttr (track, BK_MASTER_VOLUME, value0);
				break;
			}
			case BKIntrPanning: {
				value0 = cmdMask.arg1.arg1;
				BKSetAttr (track, BK_PANNING, value0);
				break;
			}
			case BKIntrPitch: {
				value0 = value2Pitch (cmdMask.arg1.arg1);
				BKSetAttr (track, BK_PITCH, value0);
				break;
			}
			case BKIntrAttackTicks: {
				value0 = cmdMask.arg2.arg1;
				value1 = cmdMask.arg2.arg2;

				if (value1) {
					value0 = interpreter -> stepTickCount * value0 / value1;
				}

				BKTKInterpreterEventSet (interpreter, BKIntrEventAttack, value0);
				break;
			}
			case BKIntrReleaseTicks: {
				value0 = cmdMask.arg2.arg1;
				value1 = cmdMask.arg2.arg2;

				if (value1) {
					value0 = interpreter -> stepTickCount * value0 / value1;
				}

				BKTKInterpreterEventSet (interpreter, BKIntrEventMute, 0);
				BKTKInterpreterEventSet (interpreter, BKIntrEventRelease, value0);
				break;
			}
			case BKIntrMuteTicks: {
				value0 = cmdMask.arg2.arg1;
				value1 = cmdMask.arg2.arg2;

				if (value1) {
					value0 = interpreter -> stepTickCount * value0 / value1;
				}

				BKTKInterpreterEventSet (interpreter, BKIntrEventRelease, 0);
				BKTKInterpreterEventSet (interpreter, BKIntrEventMute, value0);
				break;
			}
			case BKIntrTicks: {
				value0 = cmdMask.arg2.arg1;
				value1 = cmdMask.arg2.arg2;

				if (value1) {
					value0 = interpreter -> stepTickCount * value0 / value1;
				}

				BKTKInterpreterEventSet (interpreter, BKIntrEventStep, value0);
				run = 0;
				break;
			}
			case BKIntrStep: {
				value0 = cmdMask.arg1.arg1;
				BKTKInterpreterEventSet (interpreter, BKIntrEventStep, value0 * interpreter -> stepTickCount);
				run = 0;
				break;
			}
			case BKIntrStepTicks: {
				BKTKTrack * track;

				value0 = cmdMask.arg1.arg1;
				interpreter -> stepTickCount = value0;

				for (BKUSize i = 0; i < ctx -> ctx -> tracks.len; i ++) {
					track = *(BKTKTrack **) BKArrayItemAt (&ctx -> ctx -> tracks, i);

					if (track) {
						track -> interpreter.stepTickCount = value0;
					}
				}
				break;
			}
			case BKIntrStepTicksTrack: {
				value0 = cmdMask.arg1.arg1;
				interpreter -> stepTickCount = value0;
				break;
			}
			case BKIntrTickRate: {
				BKTime time;
				BKContext * ctx = track -> unit.ctx;

				value0 = cmdMask.arg2.arg1;
				value1 = cmdMask.arg2.arg2;

				if (value1) {
					time = BKTimeFromSeconds (ctx, (float) value0 / (float) value1);
					BKSetPtr (ctx, BK_CLOCK_PERIOD, &time, sizeof (time));
				}
				break;
			}
			case BKIntrEffect: {
				BKInt args [8];

				argMask = BKReadIntrMask (&opcode);
				args [0] = argMask.arg2.arg1;
				args [3] = argMask.arg2.arg2;

				args [1] = BKReadIntrMask (&opcode).arg1.arg1;

				argMask = BKReadIntrMask (&opcode);
				args [2] = argMask.arg2.arg1;
				args [4] = argMask.arg2.arg2;

				if (args [3]) {
					args [0] = interpreter -> stepTickCount * args [0] / args [3];
				}

				if (args [4]) {
					args [2] = interpreter -> stepTickCount * args [2] / args [4];
				}

				BKTrackSetEffect (track, cmdMask.arg1.arg1, args, sizeof (BKInt [3]));
				break;
			}
			case BKIntrDutyCycle: {
				value0 = cmdMask.arg1.arg1;
				BKSetAttr (track, BK_DUTY_CYCLE, value0);
				break;
			}
			case BKIntrPhaseWrap: {
				value0 = cmdMask.arg1.arg1;
				BKSetAttr (track, BK_PHASE_WRAP, value0);
				break;
			}
			case BKIntrInstrument: {
				BKTKInstrument ** instrRef;
				BKInstrument * instr = NULL;

				value0 = cmdMask.arg1.arg1;
				instrRef = BKArrayItemAt (&ctx -> ctx -> instruments, value0);

				if (instrRef) {
					instr = &(*instrRef) -> instr;
				}

				BKSetPtr (track, BK_INSTRUMENT, instr, sizeof (void *));
				break;
			}
			case BKIntrWaveform: {
				BKTKWaveform * waveform = NULL;
				BKInt masterVolume = 0;

				value0 = cmdMask.arg1.arg1;

				if (value0 & BK_INTR_CUSTOM_WAVEFORM_FLAG) {
					value0 &= ~BK_INTR_CUSTOM_WAVEFORM_FLAG;

					waveform = *(BKTKWaveform **) BKArrayItemAt (&ctx -> ctx -> waveforms, value0);
					value0 = BK_CUSTOM;

					if (waveform == NULL) {
						value0 = BK_SQUARE;
					}
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
					// special waveform type
					case BK_SAMPLE: {
						masterVolume = BK_MAX_VOLUME * 0.30;
						value0 = BK_SQUARE;
						break;
					}
				}

				if (value0 == BK_CUSTOM) {
					BKSetPtr (track, BK_WAVEFORM, &waveform -> data, sizeof (void *));
				}
				else {
					BKSetAttr (track, BK_WAVEFORM, value0);
				}

				BKSetAttr (track, BK_MASTER_VOLUME, masterVolume);

				break;
			}
			case BKIntrSample: {
				BKTKSample * sample;

				value0 = cmdMask.arg1.arg1;
				sample = *(BKTKSample **) BKArrayItemAt (&ctx -> ctx -> samples, value0);

				if (sample) {
					BKSetPtr (track, BK_SAMPLE, sample ? &sample -> data : NULL, sizeof (void *));
					BKSetAttr (track, BK_SAMPLE_REPEAT, sample -> repeat);

					if (sample -> sustainRange [0] != sample -> sustainRange [1]) {
						BKSetPtr (track, BK_SAMPLE_SUSTAIN_RANGE, sample -> sustainRange, sizeof (sample -> sustainRange));
					}
				}

				break;
			}
			case BKIntrSampleRepeat: {
				value0 = cmdMask.arg1.arg1;
				BKSetAttr (track, BK_SAMPLE_REPEAT, value0);
				break;
			}
			case BKIntrSampleRange: {
				BKInt range [2];

				range [0] = BKReadIntrMask (&opcode).arg1.arg1;
				range [1] = BKReadIntrMask (&opcode).arg1.arg1;

				BKSetPtr (track, BK_SAMPLE_RANGE, range, sizeof (range));
				break;
			}
			case BKIntrSampleSustainRange: {
				BKInt range [2];

				range [0] = BKReadIntrMask (&opcode).arg1.arg1;
				range [1] = BKReadIntrMask (&opcode).arg1.arg1;

				BKSetPtr (track, BK_SAMPLE_SUSTAIN_RANGE, range, sizeof (range));
				break;
			}
			case BKIntrReturn: {
				if (interpreter -> stackPtr > interpreter -> stack) {
					opcode = (void *) (-- interpreter -> stackPtr) -> ptr;
				}
				break;
			}
			case BKIntrCall: {
				BKTKGroup * group = NULL;
				BKTKTrack * track = NULL;
				BKTKStackItem * prevItem = NULL;
				BKTKStackItem * item;

				value0 = cmdMask.grp.idx1;
				value1 = cmdMask.grp.idx2 + 1;

				if (interpreter -> stackPtr >= interpreter -> stackEnd) {
					break;
				}

				if (interpreter -> stackPtr > interpreter -> stack) {
					prevItem = interpreter -> stackPtr - 1;
				}

				item = interpreter -> stackPtr ++;
				item -> ptr = (uintptr_t) opcode;

				switch (cmdMask.grp.type) {
					case BKGroupIndexTypeLocal: {
						if (prevItem) {
							track = *(BKTKTrack **) BKArrayItemAt (&ctx -> ctx -> tracks, prevItem -> trackIdx + 1);
						}
						else {
							track = ctx;
						}
						break;
					}
					case BKGroupIndexTypeGlobal: {
						track = *(BKTKTrack **) BKArrayItemAt (&ctx -> ctx -> tracks, 0);
						break;
					}
					case BKGroupIndexTypeTrack: {
						track = *(BKTKTrack **) BKArrayItemAt (&ctx -> ctx -> tracks, value1);
						break;
					}
				}

				if (track) {
					group = *(BKTKGroup **) BKArrayItemAt (&track -> groups, value0);
					opcode = group -> byteCode.first -> data;
					item -> trackIdx = track -> object.index;
				}

				break;
			}
			case BKIntrRepeatStart: {
				interpreter -> repeatStartAddr = (uintptr_t) opcode;
				break;
			}
			case BKIntrJump: {
				value0 = cmdMask.arg1.arg1;

				// jump to repeat mark
				if (value0 == -1) {
					if (interpreter -> repeatStartAddr) {
						opcode = (void *) interpreter -> repeatStartAddr;
						interpreter -> object.flags |= BKTKInterpreterFlagHasRepeated;
					}
				}
				else {
					// unused
				}
				break;
			}
			case BKIntrEnd: {
				BKTKInterpreterEventSet (interpreter, BKIntrEventStep, BK_INT_MAX);
				interpreter -> object.flags |= BKTKInterpreterFlagHasStopped;
				opcode = ((uint32_t *) opcode) - 1; // repeat command forever
				run = 0;
				result = 0;
				break;
			}
			case BKIntrLineNo: {
				value0 = cmdMask.arg1.arg1;
				interpreter -> lineno = value0;
				interpreter -> lineTime = interpreter -> time;
				break;
			}
		}
	}
	while (run);

	numSteps  = 1; // default steps
	tickEvent = BKTKInterpreterEventGetNext (interpreter);

	if (tickEvent) {
		numSteps = tickEvent -> ticks;
	}

	interpreter -> numSteps = numSteps;
	interpreter -> opcodePtr = opcode;
	interpreter -> time += numSteps;

	(* outTicks) = numSteps;

	return result;
}

void BKTKInterpreterReset (BKTKInterpreter * interpreter)
{
	interpreter -> object.flags   &= ~BKObjectFlagUsableMask;
	interpreter -> numSteps        = 0;
	interpreter -> opcodePtr       = interpreter -> opcode;
	interpreter -> stackPtr        = interpreter -> stack;
	interpreter -> stackEnd        = (void *) interpreter -> stack + sizeof (interpreter -> stack);
	interpreter -> numEvents       = 0;
	interpreter -> nextNoteIndex   = 0;
	interpreter -> repeatStartAddr = 0;
	interpreter -> time            = 0;
	interpreter -> lineTime        = 0;
	interpreter -> lineno          = -1;
	interpreter -> lastLineno      = 0;
	interpreter -> stepTickCount   = 24;
}

BKClass const BKTKInterpreterClass =
{
	.instanceSize = sizeof (BKTKInterpreter),
};
