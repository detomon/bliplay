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

#include <unistd.h>
#include <fcntl.h>
#include "BKContextWrapper.h"

static BKInt BKTrackWrapperTick (BKCallbackInfo * info, BKTrackWrapper * track)
{
	BKInt ticks;

	BKInterpreterTrackAdvance (& track -> interpreter, & track -> track, & ticks);
	info -> divider = ticks;

	return 0;
}

static BKInt BKTrackWrapperInit (BKTrackWrapper * track)
{
	memset (track, 0, sizeof (*track));

	if (BKInterpreterInit (& track -> interpreter) < 0) {
		return -1;
	}

	if (BKTrackInit (& track -> track, BK_SQUARE) < 0) {
		return -1;
	}

	BKCallback callback = {
		.func     = (void *) BKTrackWrapperTick,
		.userInfo = track,
	};

	if (BKDividerInit (& track -> divider, 1, & callback) < 0) {
		return -1;
	}

	return 0;
}

static void BKTrackWrapperDispose (BKTrackWrapper * track)
{
	BKInterpreterDispose (& track -> interpreter);
	BKTrackDispose (& track -> track);
	BKDividerDispose (& track -> divider);

	memset (track, 0, sizeof (*track));
}

BKInt BKContextWrapperInit (BKContextWrapper * wrapper, BKUInt numChannels, BKUInt sampleRate)
{
	memset (wrapper, 0, sizeof (*wrapper));

	if (BKContextInit (& wrapper -> ctx, numChannels, sampleRate) < 0) {
		return -1;
	}

	if (BKCompilerInit (& wrapper -> compiler) < 0) {
		return -1;
	}

	if (BKArrayInit (& wrapper -> tracks, sizeof (BKTrackWrapper), 0) < 0) {
		return -1;
	}

	return 0;
}

void BKContextWrapperDispose (BKContextWrapper * wrapper)
{
	BKInstrument * instrument;
	BKData * data;
	BKTrackWrapper * track;

	BKContextDispose (& wrapper -> ctx);

	for (BKInt i = 0; i < wrapper -> instruments.length; i ++) {
		BKArrayGetItemAtIndexCopy (& wrapper -> instruments, i, & instrument);

		if (instrument) {
			BKInstrumentDispose (instrument);
			free (instrument);
		}
	}

	for (BKInt i = 0; i < wrapper -> waveforms.length; i ++) {
		BKArrayGetItemAtIndexCopy (& wrapper -> waveforms, i, & data);

		if (data) {
			BKDataDispose (data);
			free (data);
		}
	}

	for (BKInt i = 0; i < wrapper -> samples.length; i ++) {
		BKArrayGetItemAtIndexCopy (& wrapper -> samples, i, & data);

		if (data) {
			BKDataDispose (data);
			free (data);
		}
	}

	for (BKInt i = 0; i < wrapper -> tracks.length; i ++) {
		track = BKArrayGetItemAtIndex (& wrapper -> tracks, i);
		BKTrackWrapperDispose (track);
	}

	BKArrayDispose (& wrapper -> tracks);

	memset (wrapper, 0, sizeof (*wrapper));
}

static BKInt BKContextWrapperMakeTrack (BKContextWrapper * wrapper, BKCompilerTrack * compilerTrack)
{
	BKTrackWrapper * track;

	track = BKArrayPushPtr (& wrapper -> tracks);

	if (track == NULL) {
		return -1;
	}

	if (BKTrackWrapperInit (track)) {
		return -1;
	}

	// copy track opcode
	memcpy (& track -> opcode, & compilerTrack -> globalCmds, sizeof (track -> opcode));
	memset (& compilerTrack -> globalCmds, 0, sizeof (compilerTrack -> globalCmds));

	track -> interpreter.stepTickCount = wrapper -> compiler.stepTicks;
	track -> interpreter.opcode        = track -> opcode.data;
	track -> interpreter.opcodePtr     = track -> interpreter.opcode;
	track -> interpreter.instruments   = & wrapper -> instruments;
	track -> interpreter.waveforms     = & wrapper -> waveforms;
	track -> interpreter.samples       = & wrapper -> samples;

	if (BKTrackAttach (& track -> track, & wrapper -> ctx) < 0) {
		return -1;
	}

	if (BKContextAttachDivider (& wrapper -> ctx, & track -> divider, BK_CLOCK_TYPE_BEAT) < 0) {
		return -1;
	}

	return 0;
}

static BKInt BKContextWrapperMakeTracks (BKContextWrapper * wrapper)
{
	BKCompilerTrack * compilerTrack;

	// copy arrays
	memcpy (& wrapper -> instruments, & wrapper -> compiler.instruments, sizeof (wrapper -> instruments));
	memset (& wrapper -> compiler.instruments, 0, sizeof (wrapper -> compiler.instruments));

	memcpy (& wrapper -> waveforms, & wrapper -> compiler.waveforms, sizeof (wrapper -> waveforms));
	memset (& wrapper -> compiler.waveforms, 0, sizeof (wrapper -> compiler.waveforms));

	memcpy (& wrapper -> samples, & wrapper -> compiler.samples, sizeof (wrapper -> samples));
	memset (& wrapper -> compiler.samples, 0, sizeof (wrapper -> compiler.samples));

	if (wrapper -> compiler.tracks.length) {
		for (BKInt i = 0; i < wrapper -> compiler.tracks.length; i ++) {
			BKArrayGetItemAtIndexCopy (& wrapper -> compiler.tracks, i, & compilerTrack);

			if (BKContextWrapperMakeTrack (wrapper, compilerTrack)) {
				return -1;
			}
		}
	}
	// use global track
	else {
		if (BKContextWrapperMakeTrack (wrapper, & wrapper -> compiler.globalTrack) < 0) {
			return -1;
		}
	}

	wrapper -> stepTicks = wrapper -> compiler.stepTicks;

	BKCompilerDispose (& wrapper -> compiler);

	return 0;
}

BKInt BKContextWrapperLoadData (BKContextWrapper * wrapper, char const * data, size_t size, char const * loadPath)
{
	BKSTParser parser;

	if (BKSTParserInit (& parser, data, size) < 0) {
		return -1;
	}

	if (BKCompilerCompile (& wrapper -> compiler, & parser, 0) < 0) {
		BKSTParserDispose (& parser);
		return -1;
	}

	BKSTParserDispose (& parser);

	if (loadPath) {
		wrapper -> compiler.loadPath = strdup (loadPath);
	}

	if (BKContextWrapperMakeTracks (wrapper) < 0) {
		return -1;
	}

	return 0;
}

BKInt BKContextWrapperLoadFile (BKContextWrapper * wrapper, FILE * file, char const * loadPath)
{
	BKSTParser parser;

	if (BKSTParserInitWithFile (& parser, file) < 0) {
		return -1;
	}

	if (BKCompilerCompile (& wrapper -> compiler, & parser, 0) < 0) {
		BKSTParserDispose (& parser);
		return -1;
	}

	BKSTParserDispose (& parser);

	if (loadPath) {
		wrapper -> compiler.loadPath = strdup (loadPath);
	}

	if (BKContextWrapperMakeTracks (wrapper) < 0) {
		return -1;
	}

	return 0;
}
