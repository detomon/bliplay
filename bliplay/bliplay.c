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

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include "BKSDLTrack.h"
#include "BKWaveFileWriter.h"

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "bliplay"
#endif

typedef struct
{
	BKInterpreter interpreter;
	BKTrack       track;
	BKDivider     divider;
} BKTrackWrapper;

static BKTrackWrapper trackWrappers [8];

enum
{
	INTERACTIVE_FLAG  = 1 << 0,
	PLAY_FLAG         = 1 << 1,
	NO_PAUSE_SND_FLAG = 1 << 2,
	HAS_SEEK_TIME     = 1 << 3,
	PRINT_NO_TIME     = 1 << 4,
	NO_SOUND          = 1 << 5,
};

enum
{
	HELP_COMMAND,
	LOAD_COMMAND,
	QUIT_COMMAND,
	PAUSE_COMMAND,
	PLAY_COMMAND,
};

enum
{
	OUTPUT_TYPE_NONE,
	OUTPUT_TYPE_RAW,
	OUTPUT_TYPE_WAVE,
};

struct commandDef
{
	char const * name;
	char const * description;
	unsigned     command;
};

static const struct commandDef commands [] =
{
	{"exit", "Quit program", QUIT_COMMAND},
	{"help", "Print this screen", HELP_COMMAND},
	{"load", "Load file", LOAD_COMMAND},
	{"pause", "Pause loaded file", PAUSE_COMMAND},
	{"play", "Play loaded file", PLAY_COMMAND},
	{"quit", "Quit program", QUIT_COMMAND},
};

#define NUM_COMMAND_DEFS (sizeof (commands) / sizeof (struct commandDef))

static int            flags;
static char const   * filename;
static char const   * outputFilename;
static FILE         * outputFile;
static BKSDLContext   ctx, pauseCtx;
static BKSDLContext * runCtx;
static BKTime         seekTime;
static BKEnum         outputType = OUTPUT_TYPE_NONE;

static BKWaveFileWriter waveWriter;

static int set_nocanon (int nocanon)
{
	struct termios oldtc, newtc;

	tcgetattr (STDIN_FILENO, & oldtc);

	newtc = oldtc;

	if (nocanon) {
		newtc.c_lflag &= ~(ICANON);
	} else {
		newtc.c_lflag |= (ICANON);
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

static void printOptionHelp (void)
{
	printf (
		"usage: %1$s [-s speed | --speed speed] [-p | --play] [-n | --no-time] [-m | --no-sound] file\n"
		"       %1$s [-o | --output file[.raw|.wav]]\n"
		"       %1$s [-f | --fast-forward value[f (frames) | b (beats) | t (ticks) | s (seconds)]]\n"
		"       %1$s [-r | --samplerate value]\n"
		"       %1$s [-u | --no-pause-snd]\n"
		"       %1$s [-h | --help]\n"
		"\n"
		"       --no-sound implicitly sets --play\n",
		PROGRAM_NAME
	);
}

static void printString (char const * format, va_list args, int level)
{
	FILE * stream = stdout;
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
			stream = stderr;
			color = "31";
			break;
		}
	}

	fprintf (stream, "\033[%sm", color);
	vfprintf (stream, format, args);
	fprintf (stream, "\033[%sm", "0");
	fflush (stream);
}

static void printMessage (char const * format, ...)
{
	va_list args;

	va_start (args, format);
	printString (format, args, 0);
	va_end (args);
}

static void printNotice (char const * format, ...)
{
	va_list args;

	va_start (args, format);
	printString (format, args, 1);
	va_end (args);

}

static void printError (char const * format, ...)
{
	va_list args;

	va_start (args, format);
	printString (format, args, 2);
	va_end (args);
}

static int lookupCommand (char const * key, struct commandDef const * cmd)
{
	return strcmp (key, cmd -> name);
}

static int getCommand (char const * name)
{
	struct commandDef const * cmd;

	cmd = bsearch (name, commands, NUM_COMMAND_DEFS, sizeof (struct commandDef), (void *) lookupCommand);

	if (cmd)
		return cmd -> command;

	return -1;
}

static int loadFile (BKSDLContext * ctx, char * const args)
{
	char const * filename = args;

	if (BKSDLContextLoadFile (ctx, filename) == 0) {
		printf ("File '%s' loaded\n", args);
	}
	else {
		printError ("No such file: %s", filename);
		return -1;
	}

	return 0;
}

struct option const options [] = {
	{"speed",        required_argument, NULL, 's'},
	{"samplerate",   required_argument, NULL, 'r'},
	{"help",         no_argument,       NULL, 'h'},
	{"play",         optional_argument, NULL, 'p'},
	{"output",       required_argument, NULL, 'o'},
	{"no-pause-snd", no_argument,       NULL, 'u'},
	{"fast-forward", required_argument, NULL, 'f'},
	{"no-time",      no_argument,       NULL, 'n'},
	{"no-sound",     no_argument,       NULL, 'm'},
	{NULL,           0,                 NULL, 0},
};

static void outputChunk (BKFrame const * frames, BKInt numFrames)
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

static void fillAudio (BKSDLContext * ctx, Uint8 * stream, int len)
{
	BKUInt numChannels = ctx -> ctx.numChannels;
	BKUInt numFrames   = len / sizeof (BKFrame) / numChannels;

	BKContextGenerate (& runCtx -> ctx, (BKFrame *) stream, numFrames);
	outputChunk ((BKFrame *) stream, numFrames * numChannels);
}

static BKInt initSDL (BKSDLContext * ctx, char const ** error)
{
	SDL_Init (SDL_INIT_AUDIO);

	SDL_AudioSpec wanted;

	wanted.freq     = ctx -> ctx.sampleRate;
	wanted.format   = AUDIO_S16SYS;
	wanted.channels = ctx -> ctx.numChannels;
	wanted.samples  = 1024;
	wanted.callback = (void *) fillAudio;
	wanted.userdata = ctx;

	if (SDL_OpenAudio (& wanted, NULL) < 0) {
		* error = SDL_GetError ();
		return -1;
	}

	return 0;
}

static void setRunContext (BKSDLContext * ctx, BKInt reset)
{
	SDL_LockAudio ();

	runCtx = ctx;

	if (reset)
		BKSDLContextReset (runCtx, 0);

	SDL_UnlockAudio ();
}

static BKInt parseSeekTime (char const * string, BKTime * outTime, BKInt speed)
{
	double value;
	char   type;
	BKTime time;
	BKInt  argc;

	argc = sscanf (string, "%lf%c", & value, & type);

	if (argc < 1) {
		printError ("Fast forward option must define a numeric value (e.g. 12.4s, 760t, 45600f)\n");
		return -1;
	}

	if (value < 0) {
		printError ("Fast forward option must be a positive value\n");
		return -1;
	}

	if (argc < 2)
		type = 't';

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
			printError ("Unknown fast forward unit '%c'\n", type);
			return -1;
		}
	}

	* outTime = time;

	return 0;
}

static int stringEndsWith (char const * str, char const * tail)
{
	char const * sc, * tc;
	BKSize sl, tl;

	if (str == NULL || tail == NULL)
		return 0;

	sl = strlen (str);
	tl = strlen (tail);

	if (sl == 0 || tl == 0 || tl > sl)
		return 0;

	sc = & str [sl - 1];
	tc = & tail [tl - 1];

	for (; tc > tail; tc --, sc --) {
		if (* tc != * sc)
			return 0;
	}

	return 1;
}

static int handleOptions (BKSDLContext * ctx, int argc, const char * argv [])
{
	int    opt;
	int    longoptind = 1;
	BKUInt sampleRate = 44100;
	BKUInt speed      = 0;
	char seekTimeString [64];

	char const * error = NULL;

	opterr = 0;

	while ((opt = getopt_long (argc, (void *) argv, "hpo:s:r:uf:nm", options, & longoptind)) != -1) {
		switch (opt) {
			case 's': {
				speed = atoi (optarg);
				break;
			}
			case 'h': {
				printOptionHelp ();
				exit (0);
				break;
			}
			case 'p': {
				flags |= PLAY_FLAG;
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
			case 'u': {
				flags |= NO_PAUSE_SND_FLAG;
				break;
			}
			case 'f': {
				flags |= HAS_SEEK_TIME;
				strncpy (seekTimeString, optarg, 64);
				break;
			}
			case 'n': {
				flags |= PRINT_NO_TIME;
				break;
			}
			case 'm': {
				flags |= NO_SOUND | PLAY_FLAG;
				break;
			}
			default: {
				printError ("Unknown option %c near %s\n", opt, argv [longoptind]);
				printOptionHelp ();
				exit (1);
				break;
			}
		}
	}

	if (optind <= argc) {
		filename = argv [optind];
	}
	else {
		printOptionHelp ();
		exit (1);
	}

	if (outputFilename) {
		char const * mode;

		mode = "wb+";

		outputFile = fopen (outputFilename, mode);

		if (outputFile == NULL) {
			printError ("Couldn't open file for output: %s\n", outputFilename);
			return -1;
		}

		if (stringEndsWith (outputFilename, ".wav")) {
			outputType = OUTPUT_TYPE_WAVE;
			BKWaveFileWriterInit (& waveWriter, outputFile, 2, sampleRate);
		}
		else if (stringEndsWith (outputFilename, ".raw")) {
			outputType = OUTPUT_TYPE_RAW;
		}
		else {
			printError ("Only .raw and .wav is supported for output\n");
			return -1;
		}
	}

	if (BKSDLContextInit (ctx, 2, sampleRate) < 0) {
		printError ("Couldn't initialize context\n");
		return -1;
	}

	if (BKSDLContextInit (& pauseCtx, 2, sampleRate) < 0) {
		printError ("Couldn't initialize pause context\n");
		return -1;
	}

	char const * data = "gv:255; \
stepticks:14; \
 \
track:begin:square; \
 \
	dc:8;v:192; \
	mt:8;a:c5;s:1;r;s:1; \
	mt:8;a:c6;s:1;r;s:1; \
	mt:8;a:c5;s:1;r;s:1; \
	mt:8;a:c6;s:1;r;s:1; \
	\
	z; \
 \
track:end;";

	BKInt dataSize = strlen (data);

	if (BKSDLContextLoadData (& pauseCtx, data, dataSize) < 0) {
		printError ("Couldn't load pause sound\n");
		return -1;
	}

	setRunContext (ctx, 0);

	if (initSDL (ctx, & error) < 0) {
		printError ("Couldn't initialize SDL: %s\n", error);
		return -1;
	}

	if () {
		SDL_LockAudio ();

		if (BKSDLContextLoadFile (ctx, filename) < 0) {
			printError ("No such file: %s\n", filename);
			exit (1);
		}

		if (speed) {
			for (BKInt i = 0; i < ctx -> numTracks; i ++)
				ctx -> tracks [i] -> interpreter.stepTickCount = speed;

			ctx -> speed = speed;
		}
		else {
			speed = ctx -> speed;
		}

		if (flags & HAS_SEEK_TIME) {
			if (parseSeekTime (seekTimeString, & seekTime, speed) < 0) {
				exit(1);
				return -1;
			}
		}

		SDL_UnlockAudio ();
	}

	return 0;
}

static BKInt pushFrames (BKFrame inFrames [], BKUInt size, void * info)
{
	return 0;
}

static void seekContext (BKSDLContext * ctx, BKTime time)
{
	BKContextGenerateToTime (& ctx -> ctx, time, pushFrames, NULL);
}

static BKInt checkTrackStatus (void)
{
	BKSDLTrack * track;
	BKInt numActive = ctx.numTracks;

	for (BKInt i = 0; i < ctx.numTracks; i ++) {
		track = ctx.tracks [i];

		// exit if tracks have repeated
		if (flags & NO_SOUND) {
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

static BKInt handleKeys ()
{
	BKInt paused = 0;

	printNotice ("[space] = play/pause, [q] = stop\n");

	if (flags & PLAY_FLAG) {
		seekContext (& ctx, seekTime);

		paused = 0;
		SDL_PauseAudio (0);
		printf ("\rPlaying...");
	}
	else {
		paused = 1;
		printf ("\rPaused    ");
	}

	do {
		fd_set in;
		struct timeval tv;

		FD_ZERO (& in);
		FD_SET (STDIN_FILENO, & in);
		tv.tv_sec = 0;
		tv.tv_usec = 10000;

		set_nocanon(1);

		int res = select (STDIN_FILENO + 1, & in, NULL, NULL, & tv);

		if (res > 0) {
			int c = getchar_nocanon (0);

			switch (c) {
				case 'q': {
					printf ("\rStopped   ");
					goto end;
					break;
				}
				case ' ': {
					paused = !paused;

					if (paused) {
						if (flags & NO_PAUSE_SND_FLAG) {
							SDL_PauseAudio (1);
						}
						else {
							setRunContext (& pauseCtx, 1);
						}

						printf ("\rPaused    ");
					}
					else {
						SDL_PauseAudio (0);

						setRunContext (& ctx, 0);
						printf ("\rPlaying...");
					}

					break;
				}
			}
		}
		else if (res == 0) {
			int frames = BKTimeGetTime (ctx.ctx.currentTime) * 100 / ctx.ctx.sampleRate;
			int frac = frames % 100;
			frames /= 100;
			int secs = frames % 60;
			int mins = frames / 60;

			if (!(flags & PRINT_NO_TIME)) {
				int groups [64];

				for (int i = 0; i < ctx.numTracks; i ++) {
					if (ctx.tracks[i] -> interpreter.stackPtr > ctx.tracks[i] -> interpreter.stack) {
						groups [i] = ctx.tracks[i] -> interpreter.stackPtr [-1];
					} else {
						groups [i] = -1;
					}
				}

				printf ("\rPlaying %3d:%02d.%02d", mins, secs, frac);

				for (int i = 0; i < ctx.numTracks; i ++)
					printf ("  % 6d", groups [i]);

				fflush (stdout);
			}
		}
		else {
			printError ("select failed\n");
			exit (1);
		}

		// exit if all tracks have stopped
		if (!checkTrackStatus ()) {
			break;
		}

		//SDL_Delay(100);
	}
	while (1);

	set_nocanon (0);

	end:

	printf ("\n");

	return 0;
}

static void play (void)
{
	if (flags & NO_SOUND) {
		BKInt numFrames = 512;
		BKInt numChannels = ctx.ctx.numChannels;
		BKFrame * frames = malloc (numFrames * numChannels * sizeof (BKFrame));

		while (checkTrackStatus ()) {
			BKContextGenerate (& ctx.ctx, frames, numFrames);
			outputChunk (frames, numFrames * numChannels);
		}

		free (frames);
	}
	else {
		handleKeys ();

		SDL_PauseAudio (1);
		SDL_CloseAudio ();
	}
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
}

#ifdef main
#undef main
#endif

int main (int argc, const char * argv [])
{
	if (handleOptions (& ctx, argc, argv) == 0) {
		play ();
	}

	cleanup ();

    return 0;
}
