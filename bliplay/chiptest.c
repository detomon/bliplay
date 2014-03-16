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

BKContext     ctx;
BKTrack       square, triangle, noise;
BKInstrument  instrument;
BKSDLUserData userData;
BKDivider     divider, divider2, divider3;

BKCompiler    compiler;
BKInterpreter interpreter, interpreter2, interpreter3;
BKBlipReader  reader;

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

static void compileData (BKInterpreter * interpreter, BKTrack * track, char const * data)
{
	BKCompilerReset (& compiler);
	BKInterpreterReset (interpreter);
	BKBlipReaderReset (& reader, data, strlen (data));

	BKCompilerCompile (& compiler, interpreter, & reader, 0);
	BKTrackClear (track);
}

BKEnum dividerCallback (BKCallbackInfo * info, void * userData)
{
	BKInt divider;

	BKInterpreterTrackAdvance (& interpreter, & square, & divider);

	info -> divider = divider;

	return 0;
}

BKEnum dividerCallback2 (BKCallbackInfo * info, void * userData)
{
	BKInt divider;

	BKInterpreterTrackAdvance (& interpreter2, & triangle, & divider);

	info -> divider = divider;

	return 0;
}

BKEnum dividerCallback3 (BKCallbackInfo * info, void * userData)
{
	BKInt divider;

	BKInterpreterTrackAdvance (& interpreter3, & noise, & divider);

	info -> divider = divider;

	return 0;
}

#ifdef main
#undef main
#endif

int main (int argc, char * argv [])
{
	BKInt const numChannels = 2;
	BKInt const sampleRate  = 44100;

	BKCompilerInit (& compiler);
	BKInterpreterInit (& interpreter);
	BKInterpreterInit (& interpreter2);
	BKBlipReaderInit (& reader, NULL, 0);

	BKContextInit (& ctx, numChannels, sampleRate);

	BKTrackInit (& square, BK_SQUARE);
	BKTrackInit (& triangle, BK_TRIANGLE);
	BKTrackInit (& noise, BK_NOISE);

	BKTrackSetAttr (& square, BK_MASTER_VOLUME, 0.15 * BK_MAX_VOLUME);
	BKTrackSetAttr (& triangle, BK_MASTER_VOLUME, 0.33 * BK_MAX_VOLUME);
	BKTrackSetAttr (& noise, BK_MASTER_VOLUME, 0.15 * BK_MAX_VOLUME);

	BKTrackAttach (& square, & ctx);
	BKTrackAttach (& triangle, & ctx);
	BKTrackAttach (& noise, & ctx);

	compileData (& interpreter, & square, "");
	compileData (& interpreter2, & triangle, "");
	compileData (& interpreter3, & noise, "");

	//// instrument with release sequence
	BKInstrumentInit (& instrument);

	#define NUM_VOLUME_PHASES 15

	BKInt volumeSequence [NUM_VOLUME_PHASES];

	// Create descending sequence
	for (BKInt i = 0; i < NUM_VOLUME_PHASES; i ++)
		volumeSequence [i] = ((float) BK_MAX_VOLUME * (NUM_VOLUME_PHASES - i) / NUM_VOLUME_PHASES);

	// Set volume sequence of instrument
	BKInstrumentSetSequence (& instrument, BK_SEQUENCE_VOLUME, volumeSequence, NUM_VOLUME_PHASES, 0, 1);

	BKInstrument * instruments [] = {
		& instrument,
	};

	interpreter.instruments    = instruments;
	interpreter.stepTickCount  = 18;
	interpreter2.instruments   = instruments;
	interpreter2.stepTickCount = 18;
	interpreter3.instruments   = instruments;
	interpreter3.stepTickCount = 18;

	// // Attach instrument to track
	// BKTrackSetPtr (& sampleTrack, BK_INSTRUMENT, & instrument);

	// Callback struct used for initializing divider
	BKCallback callback;

	callback.func     = dividerCallback;
	callback.userInfo = NULL;

	// Initialize divider with divider value and callback
	BKDividerInit (& divider, 1, & callback);

	callback.func     = dividerCallback2;
	callback.userInfo = NULL;

	BKDividerInit (& divider2, 1, & callback);

	callback.func     = dividerCallback3;
	callback.userInfo = NULL;

	BKDividerInit (& divider3, 1, & callback);


	// Attach the divider to the master clock
	// When samples are generated the callback is called at the defined interval
	BKContextAttachDivider (& ctx, & divider, BK_CLOCK_TYPE_BEAT);
	BKContextAttachDivider (& ctx, & divider2, BK_CLOCK_TYPE_BEAT);
	BKContextAttachDivider (& ctx, & divider3, BK_CLOCK_TYPE_BEAT);

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
			compileData (& interpreter2, & triangle, "e:pr:36;a:c3;a:c1;s:3;r;s:1;");
			BKDividerReset (& divider2);
			compileData (& interpreter, & square, "i:0;v:192;dc:8;e:vb:4:-300;a:c3:d#3:c4;s:1;e:vb;a:c1;s:1;dc:4;r;e:pr:20;a:c4;a:c1;s:1;r;");
			BKDividerReset (& divider);
			compileData (& interpreter3, & noise, "i:0;v:178;a:c6;r;s:2;v:64;pw:64;a:c4;r;");
			BKDividerReset (& divider3);
		}
		if (c == 'r') {
			compileData (& interpreter2, & triangle, "e:pr:36;a:c2;a:c1;s:3;r;s:1;");
			BKDividerReset (& divider2);
			compileData (& interpreter, & square, "e:pr:200;e:vb;v:220;e:tr:7:168;dc:4;a:c2;a:c4:d#4:g4;s:5;r;");
			BKDividerReset (& divider);
		}
		if (c == 'f') {
			compileData (& interpreter3, & noise, "i:0;v:192;pw:32;a:c2;r;s:2;pw:16;");
			BKDividerReset (& divider3);
		}

		SDL_UnlockAudio ();

		if (c == 'q')
			break;
	}

	printf ("\n");

	SDL_PauseAudio (1);
	SDL_CloseAudio ();

	BKDividerDispose (& divider);
	BKDividerDispose (& divider2);
	BKDividerDispose (& divider3);

	BKInstrumentDispose (& instrument);
	BKTrackDispose (& square);
	BKTrackDispose (& triangle);
	BKTrackDispose (& noise);
	BKContextDispose (& ctx);

	BKCompilerDispose (& compiler);
	BKBlipReaderDispose (& reader);

	BKInterpreterDispose (& interpreter);
	BKInterpreterDispose (& interpreter2);
	BKInterpreterDispose (& interpreter3);

    return 0;
}
