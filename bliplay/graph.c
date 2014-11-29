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

#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <ncurses.h>
#include <SDL/SDL.h>
#include "BlipKit.h"
#include "BKFFT.h"

// undef SDL main
#ifdef main
#undef main
#endif

BKContext ctx;
BKTrack track;
BKTrack track2;
BKData data;
BKFFT * fft;
double secs = -1.0;
double frac = 1.0/30.0;

static void fill_audio (BKContext * ctx, Uint8 * stream, int len)
{
	BKUInt numChannels = ctx -> numChannels;
	BKUInt numFrames   = len / sizeof (BKFrame) / numChannels;

	BKFrame * frames = (BKFrame *) stream;

	BKContextGenerate (ctx, frames, numFrames);

	if (secs < frac) {
		BKInt ret = 0;

		if (secs < 0) {
			secs = 0;
		}
		else {
			ret = 1;
		}

		secs += (double) numFrames / ctx -> sampleRate;

		if (ret) {
			return;
		}
	}
	else {
		while (secs > frac) {
			secs -= frac;
		}
	}

	BKInt divFactor = 4;
	BKComplexComp compFrames [numFrames];

	for (int i = 0; i < numFrames * 2; i += 2) {
		compFrames [i / 2] = frames [i] + frames [i + 1];
		compFrames [i / 2] /= BK_MAX_VOLUME * 2;
	}

	BKFFTSamplesLoad (fft, compFrames, numFrames, 0);
	//BKFFTTransform (fft, BK_FFT_TRANS_NORMALIZE | BK_FFT_TRANS_POLAR);

	BKFFTTransform (fft, 0);

	for (int i = 1; i < numFrames / 2 - 1; i ++) {
		fft -> output [i] = fft -> output [i + 1];
	}
	for (int i = numFrames - 1; i > numFrames / 2; i --) {
		fft -> output [i] = fft -> output [i - 1];
	}
	fft -> output [numFrames / 2].re = 0;
	fft -> output [numFrames / 2].im = 0;

	BKFFTTransform (fft, BK_FFT_TRANS_INVERT);

	for (int i = 0; i < numFrames; i ++) {
		frames [i * 2]     = fft -> input [i] * BK_MAX_VOLUME;
		frames [i * 2 + 1] = fft -> input [i] * BK_MAX_VOLUME;
	}

	/*BKComplexComp max = 0;

	for (int i = 1; i < numFrames; i += divFactor) {
		BKComplexComp v = 0;

		for (int j = i; j < i + divFactor; j ++) {
			v += fft -> output [j].re;
		}

		compFrames [i / divFactor] = v / divFactor;
	}

	max = 0;

	for (int i = 1; i < numFrames / divFactor; i ++) {
		max = BKMax (compFrames [i], max);
	}

	if (max == 0) {
		max = 1;
	}

	for (int i = 1; i < numFrames / divFactor; i ++) {
		compFrames [i] /= max;
	}

	for (int i = 1; i < numFrames / divFactor / 2; i ++) {
		int j = 0;
		int in = i;
		char buffer [33];
		BKComplexComp x = compFrames [i];

		x *= 32;

		memset (buffer, ' ', 32);
		buffer [32] = '\0';

		for (j = 0; j < x; j ++) {
			buffer [j] = '*';
		}

		buffer [32] = '\0';
		mvprintw (in, 0, "%s", buffer);
	}*/

	refresh();

	//memset (stream, 0, len);
}

int main (int argc, char const * argv [])
{
	initscr();

	BKContextInit (& ctx, 2, 44100);

	BKTrackInit (& track, BK_SAWTOOTH);
	BKTrackInit (& track2, BK_SINE);

	FILE * file = fopen ("/Users/simon/Dropbox/Musik/talk-less.blip/Insane Things.wav", "r");
	BKDataInit (& data);
	BKDataLoadWAVE (& data, file);
	fclose (file);

	BKTrackAttach (& track, & ctx);
	//BKTrackAttach (& track2, & ctx);

	BKSetAttr (& track, BK_MASTER_VOLUME, 0.3 * BK_MAX_VOLUME);
	BKSetAttr (& track, BK_VOLUME, BK_MAX_VOLUME);
	BKSetPtr (& track, BK_SAMPLE, & data, sizeof (& data));
	BKSetAttr (& track, BK_SAMPLE_REPEAT, BK_REPEAT);
	BKSetAttr (& track, BK_NOTE, BK_C_4 * BK_FINT20_UNIT);

	BKSetAttr (& track2, BK_MASTER_VOLUME, 0.3 * BK_MAX_VOLUME);
	BKSetAttr (& track2, BK_VOLUME, BK_MAX_VOLUME);
	BKSetAttr (& track2, BK_EFFECT_PORTAMENTO, 720);
	BKSetAttr (& track2, BK_DUTY_CYCLE, 8);
	BKSetAttr (& track2, BK_NOTE, BK_C_0 * BK_FINT20_UNIT);
	BKSetAttr (& track2, BK_NOTE, BK_C_8 * BK_FINT20_UNIT);

	BKFFTAlloc (& fft, 512);

	SDL_Init (SDL_INIT_AUDIO);

	SDL_AudioSpec wanted;

	wanted.freq     = ctx.sampleRate;
	wanted.format   = AUDIO_S16SYS;
	wanted.channels = ctx.numChannels;
	wanted.samples  = 512;
	wanted.callback = (void *) fill_audio;
	wanted.userdata = & ctx;

	if (SDL_OpenAudio (& wanted, NULL) < 0) {
		return -1;
	}

	SDL_PauseAudio (0);

	getch();

	SDL_PauseAudio (1);
	SDL_CloseAudio ();

	BKDispose (& track);
	BKDispose (& ctx);
	BKDispose (fft);

	endwin();

	return 0;
}
