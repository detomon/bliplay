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

#ifndef _BK_FILE_AMIGA_MOD_H_
#define _BK_FILE_AMIGA_MOD_H_

#include "BKContextWrapper.h"

#define BK_AMIGA_MOD_SONG_NAME_MAX_LENGTH 20
#define BK_AMIGA_MOD_SAMP_NAME_MAX_LENGTH 22
#define BK_AMIGA_MOD_NUM_SAMPLES 31
#define BK_AMIGA_MOD_NUM_CHANNELS 4
#define BK_AMIGA_MOD_NUM_ROWS 64
#define BK_AMIGA_MOD_NUM_PATTERNS 128
#define BK_AMIGA_NUM_VOLUME_STEPS 64

typedef struct BKFileAmigaModSample  BKFileAmigaModSample;
typedef struct BKFileAmigaModNote    BKFileAmigaModNote;
typedef struct BKFileAmigaModPattern BKFileAmigaModPattern;
typedef struct BKFileAmigaMod        BKFileAmigaMod;

struct BKFileAmigaModSample
{
	BKInt    sampleOffset;
	BKInt    sampleLength;
	BKInt    repeatStart;
	BKInt    repeatLength;
	uint16_t volume;
	uint16_t fineTune;
	char     sampleName [BK_AMIGA_MOD_SAMP_NAME_MAX_LENGTH + 1];
};

struct BKFileAmigaModNote
{
	uint8_t sample;
	uint8_t note;
	uint8_t effect;
	uint8_t effectData;
};

struct BKFileAmigaModPattern
{
	BKFileAmigaModNote rows [BK_AMIGA_MOD_NUM_ROWS][BK_AMIGA_MOD_NUM_CHANNELS];
};

struct BKFileAmigaMod
{
	BKObject                object;
	FILE                  * file;
	BKContextWrapper        contextWrapper;
	BKInt                   songLength;
	BKInt                   numPatterns;
	uint8_t                 songOffsets [BK_AMIGA_MOD_NUM_PATTERNS];
	char                    songName [BK_AMIGA_MOD_SONG_NAME_MAX_LENGTH + 1];
	char                    identifier [4];
	BKFileAmigaModSample    samples [BK_AMIGA_MOD_NUM_SAMPLES];
	BKFileAmigaModPattern * patterns;
	BKData                  sample;
};

/**
 * Initialize file object for reading from given file
 */
extern BKInt BKFileAmigaModInit (BKFileAmigaMod * mod);

/**
 * Allocate file object for reading from given file
 */
extern BKInt BKFileAmigaModAlloc (BKFileAmigaMod ** outMod);

/**
 * Read and parse file
 *
 * Possible errors:
 * BK_FILE_ERROR
 */
extern BKInt BKFileAmigaModRead (BKFileAmigaMod * mod, FILE * file);

#endif /* ! _BK_FILE_AMIGA_MOD_H_ */
