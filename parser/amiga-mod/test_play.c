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

BKTrack        tracks [4];
BKFileAmigaMod mod;
BKSDLUserData  userData;

static void fill_audio (BKSDLUserData * info, Uint8 * stream, int len)
{
	// calculate needed frames for one channel
	BKUInt numFrames = len / sizeof (BKFrame) / info -> numChannels;

	BKContextGenerate (& mod.contextWrapper.ctx, (BKFrame *) stream, numFrames);
}

BKInt patternIdx;
BKInt rowIdx;

static BKEnum dividerCallback (BKCallbackInfo * info, void * userData)
{
	BKInt offset = mod.songOffsets [patternIdx];
	BKFileAmigaModPattern const * pattern = &mod.patterns [offset];
	BKFileAmigaModNote const * row = &pattern -> rows [rowIdx];
	BKInt next = 0;

	for (BKInt i = 0; i < 1; i ++) {
		BKTrack * track = &tracks [i];
		BKFileAmigaModNote const * note = &row [i];

		if (note -> sample) {
			BKFileAmigaModSample const * sample = &mod.samples [note -> sample - 1];

			//printf("!! %u %u %u\n", note -> sample, sample -> sample -> sampleOffset, sample -> sampleLength);

			BKInt range [2] = {sample -> sampleOffset, sample -> sampleOffset + sample -> sampleLength};

			BKSetPtr (track, BK_SAMPLE_RANGE, range, sizeof (range));

			if (sample -> sampleOffset > 0 && sample -> repeatLength > 2) {
				BKInt sustain [2] = {sample -> repeatStart, sample -> repeatStart + sample -> repeatLength};
				BKSetPtr (track, BK_SAMPLE_SUSTAIN_RANGE, sustain, sizeof (sustain));
			}
		}

		if (note -> note) {
			BKSetAttr (track, BK_NOTE, BK_NOTE_MUTE);
			BKSetAttr (track, BK_NOTE, note -> note * BK_FINT20_UNIT);
		}

		if (note -> effect == 0xD) {
			next = 1;
		}

		printf ("%02x %02x %02x %3u    ", patternIdx, rowIdx, note -> sample, note -> note);
	}

	printf ("\n");

	rowIdx ++;

	if (rowIdx >= 64) {
		rowIdx = 0;
		patternIdx ++;
	}

	if (next) {
		rowIdx = 0;
		patternIdx ++;
	}
}

#ifdef main
#undef main
#endif

int main (int argc, char const * argv [])
{
	BKInt res;
	FILE * file;
	BKDivider divider;

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


	BKCallback callback;

	callback.func     = dividerCallback;
	callback.userInfo = NULL;

	BKInt dividerValue = 5;

	BKDividerInit (& divider, dividerValue, & callback);
	BKContextAttachDivider (& mod.contextWrapper.ctx, & divider, BK_CLOCK_TYPE_BEAT);



	for (BKInt i = 0; i < 4; i ++) {
		BKTrack * track = &tracks [i];

		BKTrackInit (track, BK_SQUARE);

		BKSetAttr (track, BK_MASTER_VOLUME, 0.1 * BK_MAX_VOLUME);
		BKSetAttr (track, BK_VOLUME,        0.5 * BK_MAX_VOLUME);

		BKTrackAttach (track, & mod.contextWrapper.ctx);

		BKSetPtr (track, BK_SAMPLE, & mod.sample, sizeof (& mod.sample));
	}



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
		SDL_Delay (1000);
	}

	printf ("\n");

	SDL_PauseAudio (1);
	SDL_CloseAudio ();




	BKDispose (& mod);

	return 0;
}
