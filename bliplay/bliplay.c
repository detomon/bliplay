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
#include <SDL/SDL.h>
#include "BKContextWrapper.h"
#include "BKWaveFileWriter.h"

#ifndef FD_COPY
#define FD_COPY(src, dest) memcpy ((dest), (src), sizeof (*(dest)))
#endif

#define ANALYSE_FRAMES_COUNT 512

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "bliplay"
#endif

enum
{
	OUTPUT_TYPE_NONE,
	OUTPUT_TYPE_RAW,
	OUTPUT_TYPE_WAVE,
};

enum
{
	FLAG_INTERACTIVE   = 1 << 0,
	FLAG_HAS_SEEK_TIME = 1 << 1,
	FLAG_PRINT_NO_TIME = 1 << 2,
	FLAG_NO_SOUND      = 1 << 3,
};

static BKInt            flags;
static BKContextWrapper ctx;
static BKTime           seekTime;
static BKInt            numChannels = 2;
static char const     * filename;
static char const     * outputFilename;
static FILE           * outputFile;
static BKEnum           outputType = OUTPUT_TYPE_NONE;
static BKWaveFileWriter waveWriter;
static int              waitUSecs = 50000;
static char             seekTimeString [64];

struct option const options [] = {
	{"fast-forward", required_argument, NULL, 'f'},
	{"help",         no_argument,       NULL, 'h'},
	{"no-sound",     no_argument,       NULL, 'm'},
	{"no-time",      no_argument,       NULL, 'n'},
	{"output",       required_argument, NULL, 'o'},
	{"samplerate",   required_argument, NULL, 'r'},
	{NULL,           0,                 NULL, 0},
};

static int set_noecho (int nocanon)
{
	struct termios oldtc, newtc;

	tcgetattr (STDIN_FILENO, & oldtc);

	newtc = oldtc;

	if (nocanon) {
		newtc.c_lflag &= ~(ICANON | ECHO);
	} else {
		newtc.c_lflag |= (ICANON | ECHO);
	}

	tcsetattr (STDIN_FILENO, TCSANOW, & newtc);

	return 0;
}

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

static int string_ends_with (char const * str, char const * tail)
{
	char const * sc, * tc;
	BKSize sl, tl;

	if (str == NULL || tail == NULL) {
		return 0;
	}

	sl = strlen (str);
	tl = strlen (tail);

	if (sl == 0 || tl == 0 || tl > sl) {
		return 0;
	}

	sc = & str [sl - 1];
	tc = & tail [tl - 1];

	for (; tc > tail; tc --, sc --) {
		if (* tc != * sc)
			return 0;
	}

	return 1;
}

static void print_help (void)
{
	printf (
		"usage: %1$s [options] file\n"
		"       %1$s [-n | --no-time]\n"
		"             do not print play time\n"
		"       %1$s [-m | --no-sound]\n"
		"             do not output sound\n"
		"       %1$s [-o | --output file[.raw|.wav]]\n"
		"             write audio data to file\n"
		"       %1$s [-f | --fast-forward value]\n"
		"             format: value[f (frames) | b (beats) | t (ticks) | s (seconds)] (e.g. 12.4s, 24b, 760t, 45600f)\n"
		"       %1$s [-r | --samplerate value]\n"
		"             range: 22100 - 96000\n"
		"       %1$s [-h | --help]\n"
		"             print this screen\n",
		PROGRAM_NAME
	);
}

static void set_color (FILE * stream, int level)
{
	char const * color = "0";

	switch (level) {
		case 0: {
			break;
		}
		case 1: {
			color = "33";
			break;
		}
		case 2: {
			color = "31";
			break;
		}
	}

	fprintf (stream, "\033[%sm", color);
}

static void print_color_string (char const * format, va_list args, int level)
{
	FILE * stream = stdout;

	switch (level) {
		case 2: {
			stream = stderr;
			break;
		}
	}

	set_color (stream, level);
	vfprintf (stream, format, args);
	set_color (stream, 0);
	fflush (stream);
}

static void print_message (char const * format, ...)
{
	va_list args;

	va_start (args, format);
	print_color_string (format, args, 0);
	va_end (args);
}

static void print_notice (char const * format, ...)
{
	va_list args;

	va_start (args, format);
	print_color_string (format, args, 1);
	va_end (args);

}

static void print_error (char const * format, ...)
{
	va_list args;

	va_start (args, format);
	print_color_string (format, args, 2);
	va_end (args);
}

static void output_chunk (BKFrame const frames [], BKInt numFrames)
{
	switch (outputType) {
		case OUTPUT_TYPE_RAW: {
			fwrite (frames, numFrames, sizeof (BKFrame), outputFile);
			break;
		}
		case OUTPUT_TYPE_WAVE: {
			BKWaveFileWriterAppendFrames (& waveWriter, frames, numFrames);
			break;
		}
	}
}

static void fill_audio (BKContextWrapper * ctx, Uint8 * stream, int len)
{
	BKUInt numChannels = ctx -> ctx.numChannels;
	BKUInt numFrames   = len / sizeof (BKFrame) / numChannels;

	BKContextGenerate (& ctx -> ctx, (BKFrame *) stream, numFrames);
	output_chunk ((BKFrame *) stream, numFrames * numChannels);
}

static BKInt push_frames (BKFrame inFrames [], BKUInt size, void * info)
{
	return 0;
}

static void seek_context (BKContextWrapper * ctx, BKTime time)
{
	BKContextGenerateToTime (& ctx -> ctx, time, push_frames, NULL);
}

static BKInt init_sdl (BKContextWrapper * ctx, char const ** error)
{
	SDL_Init (SDL_INIT_AUDIO);

	SDL_AudioSpec wanted;

	wanted.freq     = ctx -> ctx.sampleRate;
	wanted.format   = AUDIO_S16SYS;
	wanted.channels = ctx -> ctx.numChannels;
	wanted.samples  = 512;
	wanted.callback = (void *) fill_audio;
	wanted.userdata = ctx;

	if (SDL_OpenAudio (& wanted, NULL) < 0) {
		* error = SDL_GetError ();
		return -1;
	}

	return 0;
}

static BKInt check_tracks_running (BKContextWrapper * ctx)
{
	BKTrackWrapper * track;
	BKInt numActive = ctx -> tracks.length;

	for (BKInt i = 0; i < ctx -> tracks.length; i ++) {
		track = BKArrayGetItemAtIndex (& ctx -> tracks, i);

		// exit if tracks have repeated
		if (flags & FLAG_NO_SOUND) {
			if (track -> interpreter.flags & (BKInterpreterFlagHasStopped | BKInterpreterFlagHasRepeated)) {
				numActive --;
			}
		}
		// exit if tracks have stopped
		else if (track -> interpreter.flags & BKInterpreterFlagHasStopped) {
			numActive --;
		}
	}

	if (numActive == 0) {
		return 0;
	}

	return 1;
}

static BKInt parse_seek_time (char const * string, BKTime * outTime, BKInt speed)
{
	double value;
	char   type;
	BKTime time;
	BKInt  argc;

	argc = sscanf (string, "%lf%c", & value, & type);

	if (argc < 1) {
		print_error ("--fast-forward: invalid format\n");
		return -1;
	}

	if (value < 0) {
		print_error ("--fast-forward: must be a positive\n");
		return -1;
	}

	if (argc < 2) {
		type = 't';
	}

	switch (type) {
		case 'f': {
			time = BKTimeMake (value, 0);
			break;
		}
		case 'b': {
			time = BKTimeFromSeconds (& ctx.ctx, (1.0 / 240) * speed * value);
			break;
		}
		case 't': {
			time = BKTimeFromSeconds (& ctx.ctx, (1.0 / 240) * value);
			break;
		}
		case 's': {
			time = BKTimeFromSeconds (& ctx.ctx, value);
			break;
		}
		default: {
			print_error ("--fast-forward: unknown unit '%c'\n", type);
			return -1;
		}
	}

	* outTime = time;

	return 0;
}

static void print_time (BKContextWrapper * ctx)
{
	char const * chars [8] = {
		"▘", "▝", "▗", "▖",
		"▟", "▙", "▛", "▜",
	};

	int frames = BKTimeGetTime (ctx -> ctx.currentTime) * 100 / ctx -> ctx.sampleRate;
	int frac   = frames % 100;
	int hsecs  = frames / 100;

	int secs = hsecs % 60;
	int mins = hsecs / 60;

	char const * c1 = chars [(frames & 0xE000) >> 13];
	char const * c2 = chars [(frames & 0x1C00) >> 10];
	char const * c3 = chars [(frames & 0x380) >> 7];
	char const * c4 = chars [(frames & 0x70) >> 4];

	printf ("\r\033[32m%s%s%s%s  %d:%02d.%02d\033[0m", c1, c2, c3, c4, mins, secs, frac);
	fflush (stdout);
}

static BKInt count_slots (BKArray * array)
{
	void * ptr;
	BKInt count = 0;

	for (BKInt i = 0; i < array -> length; i ++) {
		BKArrayGetItemAtIndexCopy (array, i, & ptr);

		if (ptr) {
			count ++;
		}
	}

	return count;
}

static void print_info (BKContextWrapper * ctx)
{
	print_message ("          Tracks: %d\n", ctx -> tracks.length);
	print_message ("     Instruments: %d\n", count_slots (& ctx -> instruments));
	print_message ("Custom waveforms: %d\n", count_slots (& ctx -> waveforms));
	print_message ("         Samples: %d\n", count_slots (& ctx -> samples));
	print_message ("      Step ticks: %d\n", ctx -> stepTicks);
}

static int handle_options (BKContextWrapper * ctx, int argc, char * argv [])
{
	int    opt;
	int    longoptind = 1;
	BKUInt sampleRate = 44100;
	BKUInt speed      = 0;
	FILE * file;

	char const * error = NULL;

	opterr = 0;

	while ((opt = getopt_long (argc, (void *) argv, "f:hmno:pr:", options, & longoptind)) != -1) {
		switch (opt) {
			case 'h': {
				print_help ();
				exit (0);
				break;
			}
			case 'o': {
				outputFilename = optarg;
				break;
			}
			case 'r': {
				sampleRate = atoi (optarg);
				break;
			}
			case 'f': {
				flags |= FLAG_HAS_SEEK_TIME;
				strncpy (seekTimeString, optarg, 64);
				break;
			}
			case 'n': {
				flags |= FLAG_PRINT_NO_TIME;
				break;
			}
			case 'm': {
				flags |= FLAG_NO_SOUND;
				break;
			}
			default: {
				print_error ("Unknown option %c near %s\n", opt, argv [longoptind]);
				print_help ();
				exit (1);
				break;
			}
		}
	}

	if (optind <= argc) {
		filename = argv [optind];
	}
	else {
		print_help ();
		return -1;
	}

	if (filename == NULL) {
		print_error ("No input file given\n");
		return -1;
	}

	if (outputFilename) {
		char const * mode = "wb+";

		outputFile = fopen (outputFilename, mode);

		if (outputFile == NULL) {
			print_error ("Couldn't open file for output: %s\n", outputFilename);
			return -1;
		}

		if (string_ends_with (outputFilename, ".wav")) {
			outputType = OUTPUT_TYPE_WAVE;
			BKWaveFileWriterInit (& waveWriter, outputFile, numChannels, sampleRate);
		}
		else if (string_ends_with (outputFilename, ".raw")) {
			outputType = OUTPUT_TYPE_RAW;
		}
		else {
			print_error ("Only .raw and .wav is supported for output\n");
			return -1;
		}
	}

	if (BKContextWrapperInit (ctx, numChannels, sampleRate) < 0) {
		print_error ("Couldn't initialize BKContext\n");
		return -1;
	}

	if ((flags & FLAG_NO_SOUND) == 0) {
		if (init_sdl (ctx, & error) < 0) {
			print_error ("Couldn't initialize SDL: %s\n", error);
			return -1;
		}
	}

	struct stat st;

	if (stat (filename, & st) < 0) {
		print_error ("No such file: %s\n", filename);
		return -1;
	}

	if (S_ISDIR (st.st_mode)) {
		print_error ("Is directory: %s\n", filename);
		return -1;
	}

	file = fopen (filename, "rb");

	if (file == NULL) {
		print_error ("No such file: %s\n", filename);
		return -1;
	}

	set_color (stderr, 2);

	if (BKContextWrapperLoadFile (ctx, file, NULL) < 0) {
		print_error ("Failed to load file: %s\n", filename);
		set_color (stderr, 0);
		return -1;
	}

	set_color (stderr, 0);

	fclose (file);

	if (flags & FLAG_HAS_SEEK_TIME) {
		speed = ctx -> stepTicks;

		if (parse_seek_time (seekTimeString, & seekTime, speed) < 0) {
			return -1;
		}
	}

	return 0;
}

static void cleanup ()
{
	if (outputFile) {
		if (outputType == OUTPUT_TYPE_WAVE) {
			BKWaveFileWriterTerminate (& waveWriter);
			BKWaveFileWriterDispose (& waveWriter);
		}

		fclose (outputFile);
	}

	BKContextWrapperDispose (& ctx);
	SDL_CloseAudio ();
}

static BKInt write_output (BKContextWrapper * ctx)
{
	BKInt numFrames = 512;
	BKInt numChannels = ctx -> ctx.numChannels;
	BKFrame * frames = malloc (numFrames * numChannels * sizeof (BKFrame));

	if (frames == NULL) {
		return -1;
	}

	while (check_tracks_running (ctx)) {
		BKContextGenerate (& ctx -> ctx, frames, numFrames);
		output_chunk (frames, numFrames * numChannels);
	}

	free (frames);

	return 0;
}

static BKInt runloop (BKContextWrapper * ctx)
{
	int c;
	int res;
	int nfds;
	int flag = 1;
	fd_set fds, fdsc;
	struct timeval timeout;

	FD_ZERO (& fds);
	FD_SET (STDIN_FILENO, & fds);
	nfds = STDIN_FILENO + 1;

	set_noecho (1);
	print_notice ("Press [q] to quit\n");

	SDL_PauseAudio (0);

	do {
		FD_COPY (& fds, & fdsc);
		timeout.tv_sec  = 0;
		timeout.tv_usec = waitUSecs;

		res = select (nfds, & fdsc, NULL, NULL, & timeout);

		if (res < 0) {
			return -1;
		}
		else if (res > 0) {
			c = getchar_nocanon (0);

			switch (c) {
				case 'q': {
					flag = 0;
					break;
				}
			}
		}

		print_time (ctx);

		if (check_tracks_running (ctx) == 0) {
			break;
		}
	}
	while (flag);

	SDL_PauseAudio (1);

	set_noecho (0);

	printf ("\n");

	return 0;
}

#ifdef main
#undef main
#endif

int main (int argc, char * argv [])
{
	if (handle_options (& ctx, argc, argv) < 0) {
		return 1;
	}

	print_info (& ctx);

	if (flags & FLAG_HAS_SEEK_TIME) {
		print_notice ("Fast forward to %s\n", seekTimeString);
		seek_context (& ctx, seekTime);
	}

	if (flags & FLAG_NO_SOUND) {
		if (write_output (& ctx) < 0) {
			return 1;
		}
	}
	else {
		if (runloop (& ctx) < 0) {
			return -1;
		}
	}

	cleanup ();

    return 0;
}
