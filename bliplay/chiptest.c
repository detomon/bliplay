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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "BlipKit.h"
#include "BKCompiler.h"
#include <SDL/SDL.h>

typedef struct {
	BKInt numChannels;
	BKInt sampleRate;
} BKSDLUserData;

typedef struct {
	BKTrack       track;
	BKInterpreter interpreter;
	BKDivider     divider;
	BKInt         masterVolume;
	BKInt         stepTickCount;
	BKInt         waveform;
} BKTrackState;

BKContext     ctx;
BKInstrument  instrument, instrument2;
BKData        pikachu;
BKSDLUserData userData;

BKCompiler    compiler;
BKBlipReader  reader;

BKTrackState square, triangle, noise, sample;

static int getchar_nocanon (unsigned tcflags)
{
	int c;
	struct termios oldtc, newtc;

	tcgetattr (STDIN_FILENO, & oldtc);

	newtc = oldtc;
	newtc.c_lflag &= ~(ICANON | ECHO | tcflags);

	tcsetattr (STDIN_FILENO, TCSANOW, & newtc);
	c = getchar ();
	tcsetattr (STDIN_FILENO, TCSANOW, & oldtc);

	return c;
}

static void fill_audio (BKSDLUserData * info, Uint8 * stream, int len)
{
	// calculate needed frames for one channel
	BKUInt numFrames = len / sizeof (BKFrame) / info -> numChannels;

	BKContextGenerate (& ctx, (BKFrame *) stream, numFrames);
}

static BKEnum BKTrackStateDividerCallback (BKCallbackInfo * info, void * userData)
{
	BKInt divider;
	BKTrackState * state = userData;

	BKInterpreterTrackAdvance (& state -> interpreter, & state -> track, & divider);

	info -> divider = divider;

	return 0;
}

static void BKTrackStateCompileData (BKTrackState * state, char const * data, BKInt clearTrack)
{
	BKCompilerReset (& compiler);
	BKInterpreterReset (& state -> interpreter);
	BKBlipReaderReset (& reader, data, strlen (data));

	BKCompilerCompile (& compiler, & state -> interpreter, & reader, 0);

	if (clearTrack)
		BKTrackClear (& state -> track);

	BKDividerReset (& state -> divider);

	state -> interpreter.stepTickCount = state -> stepTickCount;
	BKTrackSetAttr (& state -> track, BK_WAVEFORM, state -> waveform);
}

static void BKTrackStateInit (BKTrackState * state, BKContext * ctx, BKInt waveform)
{
	BKInt masterVolume;
	BKCallback callback;

	switch (waveform) {
		default:
		case BK_SQUARE: {
			masterVolume = BK_MAX_VOLUME * 0.15;
			break;
		}
		case BK_TRIANGLE: {
			masterVolume = BK_MAX_VOLUME * 0.30;
			break;
		}
		case BK_SAWTOOTH: {
			masterVolume = BK_MAX_VOLUME * 0.15;
			break;
		}
		case BK_NOISE: {
			masterVolume = BK_MAX_VOLUME * 0.15;
			break;
		}
		case BK_SAMPLE: {
			masterVolume = BK_MAX_VOLUME * 0.30;
			break;
		}
	}

	memset (state, 0, sizeof (BKTrackState));

	state -> stepTickCount = 18;

	callback.func     = BKTrackStateDividerCallback;
	callback.userInfo = state;

	BKTrackInit (& state -> track, waveform);

	state -> masterVolume = masterVolume;
	state -> waveform     = waveform;
	BKTrackSetAttr (& state -> track, BK_MASTER_VOLUME, masterVolume);

	BKInterpreterInit (& state -> interpreter);
	BKDividerInit (& state -> divider, 1, & callback);

	BKTrackAttach (& state -> track, ctx);
	BKContextAttachDivider (ctx, & state -> divider, BK_CLOCK_TYPE_BEAT);

	BKTrackStateCompileData (state, "", 1);
}

static void BKTrackStateDispose (BKTrackState * state)
{
	BKTrackDispose (& state -> track);
	BKInterpreterDispose (& state -> interpreter);
	BKDividerDispose (& state -> divider);
}

#ifdef main
#undef main
#endif

int main (int argc, char * argv [])
{
	BKInt const numChannels = 2;
	BKInt const sampleRate  = 44100;

	BKCompilerInit (& compiler);
	BKBlipReaderInit (& reader, NULL, 0);

	BKContextInit (& ctx, numChannels, sampleRate);

	BKTrackStateInit (& square, & ctx, BK_SQUARE);
	BKTrackStateInit (& triangle, & ctx, BK_TRIANGLE);
	BKTrackStateInit (& noise, & ctx, BK_NOISE);
	BKTrackStateInit (& sample, & ctx, BK_SAMPLE);

	//BKTrackStateCompileData (& square, "st:18;dc:8;e:vs;v:255;a:c5;s:1;a:c6;e:vs:160;v:0;s:31;e:vs;");

	// instrument with release sequence
	BKInstrumentInit (& instrument);
	BKInstrumentInit (& instrument2);

	#define NUM_VOLUME_PHASES 15
	#define NUM_VOLUME_PHASES2 13

	BKInt volumeSequence [NUM_VOLUME_PHASES];

	// Create descending sequence
	for (BKInt i = 0; i < NUM_VOLUME_PHASES; i ++)
		volumeSequence [i] = ((float) BK_MAX_VOLUME * (NUM_VOLUME_PHASES - i) / NUM_VOLUME_PHASES);

	// Set volume sequence of instrument
	BKInstrumentSetSequence (& instrument, BK_SEQUENCE_VOLUME, volumeSequence, NUM_VOLUME_PHASES, 0, 1);

	// Create descending sequence
	for (BKInt i = 0; i < NUM_VOLUME_PHASES2; i ++)
		volumeSequence [i] = ((float) BK_MAX_VOLUME * (NUM_VOLUME_PHASES2 - i) / NUM_VOLUME_PHASES2);

	// Set volume sequence of instrument
	BKInstrumentSetSequence (& instrument2, BK_SEQUENCE_VOLUME, volumeSequence, NUM_VOLUME_PHASES2, 0, 1);

	BKDataInitAndLoadRawAudio (& pikachu, "../BlipKit/examples/Pika.raw", 16, 1, BK_LITTLE_ENDIAN);

	BKDataNormalize (& pikachu);

	//BKInt const n = 20;
	//
	//for (BKInt i = 0; i < n; i ++)
	//	printf (":%d", (int) (178.0 * (n - i) / n));

	BKInstrument * instruments [] = {
		& instrument,
		& instrument2,
	};

	BKData * samples [] = {
		& pikachu,
	};

	square.interpreter.instruments   = instruments;
	triangle.interpreter.instruments = instruments;
	noise.interpreter.instruments    = instruments;
	sample.interpreter.instruments   = instruments;
	square.interpreter.samples       = samples;
	triangle.interpreter.samples     = samples;
	noise.interpreter.samples        = samples;
	sample.interpreter.samples       = samples;

	SDL_Init (SDL_INIT_AUDIO);

	SDL_AudioSpec wanted;

	userData.numChannels = numChannels;
	userData.sampleRate  = sampleRate;

	wanted.freq     = sampleRate;
	wanted.format   = AUDIO_S16SYS;
	wanted.channels = numChannels;
	wanted.samples  = 512;
	wanted.callback = (void *) fill_audio;
	wanted.userdata = & userData;

	if (SDL_OpenAudio (& wanted, NULL) < 0) {
		fprintf (stderr, "Couldn't open audio: %s\n", SDL_GetError ());
		return 1;
	}

	SDL_PauseAudio (0);

	printf ("Press [q] to stop\n");

	while (1) {
		int c = getchar_nocanon (0);

		// Use lock when setting attributes outside of divider callbacks
		SDL_LockAudio ();

		if (c == 'e') {
			BKTrackStateCompileData (& triangle, "e:pr:36;a:c3;a:c1;s:3;r;s:1;", 1);
			BKTrackStateCompileData (& square, "i:0;v:192;dc:8;e:vb:4:-300;a:c3:d#3:c4;s:1;e:vb;a:c1;s:1;dc:4;r;e:pr:20;a:c4;a:c1;s:1;r;", 1);
			BKTrackStateCompileData (& noise, "i:0;v:178;a:c6;r;s:2;v:64;pw:64;a:c4;r;", 1);
			BKTrackStateCompileData (& sample, "d:0;e:pr:80;a:c4;a:g#3;s:12;r;", 1);
		}
		if (c == 's') {
			BKTrackStateCompileData (& triangle, "grp:begin; e:pr:36; a:c3;a:c1;s:3;r;s:1; a:f1;s:1;r;s:1;r;a:f1;s:1;r;s:1; a:c3;a:g1;s:3;r;s:1; a:c1;s:3;r;s:1; a:c3;a:c1;s:3;r;s:1;r; a:f1;s:1;r;s:1;r;a:f1;s:1;r;s:1; a:c3;a:a0;s:3;r;s:1; a:c1;s:3;r;s:1; grp:end; s:16; s:32;g:0;g:0; xb; g:0;g:0;g:0;g:0; g:0; g:0;g:0;g:0;g:0; g:0; x;", 1);
			BKTrackStateCompileData (& square, "grp:begin; i:0;a:f3;s:2;r;s:2;a:g3;s:2;r;s:2;a:a3;s:2;r;s:2;a:c4;s:2;r;s:2;r;a:g#3;s:2;r;s:2;a:g3;s:2;r;s:2;a:f3;s:2;r;s:2;a:c3;s:2;r;s:2; grp:end; grp:begin; i:0;a:f3;s:2;r;s:2;a:g3;s:2;r;s:2;a:a3;s:2;r;s:2;a:c4;s:2;r;s:2;r;a:g#3;s:1;r;s:1;at:9;a:g#3;s:1;r;s:1;at:9;a:g3;s:2;r;s:2;a:f3;s:2;r;e:pr:36;s:2;a:g#3;s:2;r;a:g#3;a:g3;r;s:2; e:pr; grp:end; grp:begin; e:pr:200;e:vb;e:tr:7:168;dc:4;a:c2;a:c4:d#4:g4;s:8;r;e:pr;e:tr; s:8; grp:end; grp:begin; s:8; e:vb:4:-300;a:c3:d#3:c4;s:1;e:vb;a:f1;s:1;dc:4;r;e:pr:20;a:c4;a:c1;s:1;r; s:9;s:4; e:vb:4:-300;a:c3:d#3:c4;s:1;e:vb;a:c1;s:1;dc:4;r;e:pr:20;a:c4;a:c1;s:1;r; s:5; grp:end; g:2; s:32;s:32;s:32; xb; v:164;pt:-1200;dc:4; g:0;g:1;g:0;g:1; g:3; dc:2;g:0;g:1; pt:0;g:0;g:1;s:32; x;", 1);
			BKTrackStateCompileData (& noise, "v:96; grp:begin; rt:8;a:c5;s:1;r;s:3 rt:16;a:c6;s:1;r;s:3; a:c5;s:2;r;s:2; rt:16;a:c6;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:8;a:c5;s:1;r;s:3; rt:16;a:c6;s:1;r;s:3; a:e5;s:2;r;s:2; rt:16;a:c6;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; grp:end; grp:begin; rt:16;at:9;a:c5;s:1;r;s:3; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:3; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:3; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1;  s:2;a:c6;s:2;rt:24;a:c4:c5;s:2;r; grp:end; s:16; g:1;g:0;g:0; xb; g:0;g:0;g:0;g:0; s:32; g:0;g:0; g:0;g:0;g:1; x;", 1);
		}
		if (c == 'r') {
			BKTrackStateCompileData (& triangle, "e:pr:36;a:c2;a:c1;s:3;r;s:1;", 1);
			BKTrackStateCompileData (& square, "e:pr:200;e:vb;v:220;e:tr:7:168;dc:4;a:c2;a:c4:d#4:g4;s:5;r;e:pr;", 1);
		}
		if (c == 'f') {
			BKTrackStateCompileData (& noise, "i:0;v:192;pw:32;a:c2;r;s:2;pw:16;", 1);
		}
		if (c == 'i') {
			BKTrackStateCompileData (& square, "i:0; dc:8;v:96;a:a#3;s:2;r;s:2; v:127;dc:4;a:c4;s:2;r;s:2;a:c4; a:a#3;s:2;r;s:2;a:c4; s:1;r;s:1;a:d4;s:1;r;s:1;e:pr:40; a:g4;s:2;a:g#4;s:2;a:g4;e:vs:80;e:vb:9:20;v:178;s:8;r;s:4;e:vb;e:pr; e:vs;v:192;e:vs:54;dc:4;e:pr:90;a:c2;a:c3;s:3;v:0;s:3;r;", 1);
			BKTrackStateCompileData (& triangle, "e:pr:30; a:f2; s:4; a:g#2;s:4;a:a#2;s:8;r;a:a#1;s:4;a:c2;s:10;r;s:2;", 1);
		}
		if (c == 'u') {
			BKTrackStateCompileData (& square, "e:vs:36;e:pr:72;v:127;dc:4;a:c3;a:g3;s:3;v:0;s:2;r;", 1);
		}
		if (c == 'z') {
			BKTrackStateCompileData (& square, "e:vs:36;e:pr:90;v:127;dc:4;a:c2;a:c3;s:3;v:0;s:3;r;", 1);
		}
		if (c == 'j') {
			BKTrackStateCompileData (& square, "v:0;s:2;r;", 0);
		}
		if (c == 'h') {
			BKTrackStateCompileData (& square, "v:0;s:3;r;", 0);
		}

		if (c == 'n') {
			BKTrackStateCompileData (& square, "grp:begin; e:pr:36; a:f3;a:g3;s:4;r; s:4; a:d#3;s:2;r;a:c3;s:2;r;a:d#3;s:2;r; s:2; a:c3;s:3;a:a#2;s:1;r; s:4; a:g2;s:1;r;s:1;a:g2;s:1;r;s:1; a:g#2;s:2;r;a:a2;s:1;r;s:1; grp:end; grp:begin; rt:9;a:g3;s:1;at:6;rt:20;a:d#3;s:1;at:12;a:c3;s:1;rt:6;s:1; rt:9;a:g3;s:1;at:6;rt:20;a:d#3;s:1;at:12;a:c3;s:1;rt:6;s:1; rt:9;a:g3;s:1;at:6;rt:20;a:a#3;s:1;at:12;a:a#3;s:1;rt:6;s:1; rt:9;a:g3;s:2;at:6;rt:9;a:f3;s:2; grp:end; grp:begin; a:f3;s:2;r;s:2;a:g3;s:2;r;s:2;a:a3;s:2;r;s:2;a:c4;s:2;r;s:2;r;a:g#3;s:2;r;s:2;a:g3;s:2;r;s:2;a:f3;s:2;r;s:2;a:g#3;s:2;r;a:g#3;a:g3;r;s:2; grp:end; s:32;s:32; xb; i:1; dc:4;v:156; g:0;g:0;g:0;g:0; dc:2;g:1;g:1; dc:4;g:2; x;", 1);
			BKTrackStateCompileData (& triangle, "grp:begin; e:pr:18; a:c3;a:c1;s:2;r;at:12;a:c1;s:1;rt:9;s:1;  rt:12;a:c3;a:c1;s:1;at:6;rt:23;a:d#1;s:1;at:12;a:g1;s:1;rt:9;s:1; a:c3;a:c1;s:1;at:6;rt:15;a:d#1;s:1;at:12;a:g1;s:1;rt:9;s:1; rt:12;a:c3;a:c1;s:1;at:6;rt:23;a:d#1;s:1;at:12;a:g1;s:1;rt:9;s:1; grp:end; grp:begin; e:pr:18; a:c3;a:c1;s:2;r;s:2; a:c3;a:c1;s:2;r;s:2;  a:c3;a:c1;s:2;r;s:2; a:c3;a:c1;s:2;r;s:2; a:c3;a:c1;s:2;r;s:2; a:c3;a:c1;s:2;r;s:2;  a:c3;a:c1;s:2;r; a:c3;a:c1;s:2;r;a:c3;a:c1;s:2;r;s:2; grp:end; s:32; g:1; xb; pt:0;g:0;g:0;g:0;g:0;pt:-400;g:0;g:0;pt:-200;g:0;pt:0;g:0;g:0;g:0;pt:-100;g:0;pt:0;g:0; x;", 1);
			BKTrackStateCompileData (& noise, "v:96; grp:begin; rt:8;a:c5;s:1;r;s:3 rt:16;a:c6;s:1;r;s:3; a:c5;s:2;r;s:2; rt:16;a:c6;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:8;a:c5;s:1;r;s:3; rt:16;a:c6;s:1;r;s:3; a:c5;s:2;r;s:2; rt:16;a:c6;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; grp:end; grp:begin; rt:16;at:9;a:c5;s:1;r;s:3; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:3; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:3; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1; rt:16;at:9;a:c5;s:1;r;s:1;  s:2;a:c6;s:2;rt:24;a:c4:c5;s:2;r; grp:end; g:1;s:32; xb; g:0;g:0;g:0;g:0; x;", 1);
		}

		SDL_UnlockAudio ();

		if (c == 'q')
			break;
	}

	printf ("\n");

	SDL_PauseAudio (1);
	SDL_CloseAudio ();

	BKTrackStateDispose (& square);
	BKTrackStateDispose (& triangle);
	BKTrackStateDispose (& noise);

	BKInstrumentDispose (& instrument);
	BKContextDispose (& ctx);

	BKCompilerDispose (& compiler);
	BKBlipReaderDispose (& reader);

    return 0;
}
