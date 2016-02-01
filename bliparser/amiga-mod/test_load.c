/**
 * Copyright (c) 2015 Simon Schoenenberger
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
#include "BlipKit.h"
#include "BKFileAmigaMod.h"
#include <SDL/SDL.h>

typedef struct {
	BKInt numChannels;
	BKInt sampleRate;
} BKSDLUserData;

BKTrack        track;
BKFileAmigaMod mod;
BKSDLUserData  userData;

static void fill_audio (BKSDLUserData * info, Uint8 * stream, int len)
{
	// calculate needed frames for one channel
	BKUInt numFrames = len / sizeof (BKFrame) / info -> numChannels;

	BKContextGenerate (& mod.contextWrapper.ctx, (BKFrame *) stream, numFrames);
}

#ifdef main
#undef main
#endif

int main (int argc, char const * argv [])
{
	BKInt res;
	FILE * file;

	file = fopen ("FlatOutLies.mod", "r");

	if (file == NULL) {
		printf ("File not found\n");
		return 1;
	}

	res = BKFileAmigaModInit (& mod);

	if (res < 0) {
		printf ("Res: %d (%d)\n", res, __LINE__);
		return 1;
	}

	res = BKFileAmigaModRead (& mod, file);

	if (res < 0) {
		printf ("Res: %d (%d)\n", res, __LINE__);
		return 1;
	}

	fclose (file);



	printf("num patterns: %u\n", mod.numPatterns);
	return 0;




	BKTrackInit (& track, BK_SQUARE);

	BKSetAttr (& track, BK_MASTER_VOLUME, 0.2 * BK_MAX_VOLUME);
	BKSetAttr (& track, BK_VOLUME,        0.5 * BK_MAX_VOLUME);

	BKTrackAttach (& track, & mod.contextWrapper.ctx);

	BKSetPtr (& track, BK_SAMPLE, & mod.sample, sizeof (& mod.sample));
	BKSetAttr (& track, BK_NOTE, BK_C_3 * BK_FINT20_UNIT);



	SDL_Init (SDL_INIT_AUDIO);

	SDL_AudioSpec wanted;

	userData.numChannels = mod.contextWrapper.ctx.numChannels;
	userData.sampleRate  = mod.contextWrapper.ctx.sampleRate;

	wanted.freq     = userData.sampleRate;
	wanted.format   = AUDIO_S16SYS;
	wanted.channels = userData.numChannels;
	wanted.samples  = 512;
	wanted.callback = (void *) fill_audio;
	wanted.userdata = & userData;

	if (SDL_OpenAudio (& wanted, NULL) < 0) {
		fprintf (stderr, "Couldn't open audio: %s\n", SDL_GetError ());
		return 1;
	}

	SDL_PauseAudio (0);

	while (1) {
	}

	printf ("\n");

	SDL_PauseAudio (1);
	SDL_CloseAudio ();




	BKDispose (& mod);

	return 0;
}
