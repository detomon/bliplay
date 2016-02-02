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

#include <math.h>
#include "BKTone.h"
#include "BKFileAmigaMod.h"

#define BK_REVERSE16(value) ((value & 0xFF) << 8 | ((value & 0xFF00) >> 8))

/**
 * Volume values used for lookup
 * Generated with `volume_values.c`
 */
static BKFrame const volumeValues [64 + 1] =
{
	    0,     0,     0,     0,     0,     0,     0,     1,
	    2,     3,     6,     9,    14,    21,    29,    41,
	   55,    73,    95,   122,   154,   193,   239,   294,
	  357,   431,   517,   615,   727,   855,  1000,  1163,
	 1346,  1551,  1779,  2034,  2315,  2627,  2970,  3348,
	 3762,  4215,  4709,  5248,  5835,  6471,  7160,  7906,
	 8711,  9578, 10512, 11516, 12593, 13748, 14984, 16305,
	17716, 19220, 20823, 22529, 24342, 26267, 28309, 30474,
	32767,
};

/**
 * Signed values for 4-bit integers
 */
static BKInt const nibbleValues [16] =
{
	[0x0] =  0,
	[0x1] = +1,
	[0x2] = +2,
	[0x3] = +3,
	[0x4] = +4,
	[0x5] = +5,
	[0x6] = +6,
	[0x7] = +7,
	[0x8] = -8,
	[0x9] = -7,
	[0xA] = -6,
	[0xB] = -5,
	[0xC] = -4,
	[0xD] = -3,
	[0xE] = -2,
	[0xF] = -1,
};

extern BKClass BKFileAmigaModClass;

/**
 * Returns 1 if system is big endian otherwise 0
 */
static BKInt BKSystemIsBigEndian (void)
{
	union { BKUInt i; char c [4]; } sentinel;

	sentinel.i = 0x01020304;

	return sentinel.c[0] == 0x01;
}

static BKInt BKFileAmigaModInitGeneric (BKFileAmigaMod * mod)
{
	BKTime period;

	if (BKContextWrapperInit (& mod -> contextWrapper, 2, 44100, 0) < 0) {
		return -1;
	}

	// tune clock rate to PAL rate
 	period = BKTimeFromSeconds (& mod -> contextWrapper.ctx, 1.0 / 50.0);

	if (BKSetPtr (& mod -> contextWrapper.ctx, BK_CLOCK_PERIOD, & period, sizeof (period)) < 0) {
		return -1;
	}

	return 0;
}

BKInt BKFileAmigaModInit (BKFileAmigaMod * mod)
{
	if (BKObjectInit (mod, & BKFileAmigaModClass, sizeof (*mod)) < 0) {
		return -1;
	}

	if (BKFileAmigaModInitGeneric (mod) < 0) {
		BKDispose (mod);
		return -1;
	}

	return 0;
}

BKInt BKFileAmigaModAlloc (BKFileAmigaMod ** outMod)
{
	if (BKObjectAlloc ((void **) outMod, & BKFileAmigaModClass, 0) < 0) {
		return -1;
	}

	if (BKFileAmigaModInitGeneric (*outMod) < 0) {
		BKDispose (*outMod);
		return -1;
	}

	return 0;
}

static void BKFileAmigaModDispose (BKFileAmigaMod * mod)
{
	if (mod -> patterns) {
		free (mod -> patterns);
	}

	BKDispose (& mod -> sample);
}

static BKInt BKFileAmigaModGetMaxNumPatterns (BKFileAmigaMod * mod)
{
	BKInt maxPattern = 0;

	for (int i = 0; i < BK_AMIGA_MOD_NUM_PATTERNS; i ++) {
		maxPattern = BKMax (maxPattern, mod -> songOffsets [i]);
	}

	return maxPattern + 1;
}

static BKInt BKFileAmigaModReadHeader (BKFileAmigaMod * mod)
{
	uint8_t header [1084];
	size_t size;
	uint8_t * ptr;
	BKInt isBigEndian = BKSystemIsBigEndian ();
	BKFileAmigaModSample * sample;
	BKInt sampleOffset = 0;

	// read whole header at once
	size = fread (header, sizeof (uint8_t), sizeof (header) / sizeof (uint8_t), mod -> file);

	if (size < sizeof (header)) {
		return BK_FILE_ERROR;
	}

	ptr = & header [0];
	strncpy (mod -> songName, (void *) ptr, BK_AMIGA_MOD_SONG_NAME_MAX_LENGTH);
	mod -> songName [BK_AMIGA_MOD_SONG_NAME_MAX_LENGTH] = '\0';

	ptr = & header [20];

	for (int i = 0; i < BK_AMIGA_MOD_NUM_SAMPLES; i ++) {
		sample = &mod -> samples [i];

		strncpy (sample -> sampleName, (void *) ptr, BK_AMIGA_MOD_SAMP_NAME_MAX_LENGTH);
		sample -> sampleName [BK_AMIGA_MOD_SAMP_NAME_MAX_LENGTH] = '\0';
		ptr += 22;
		sample -> sampleLength = *(uint16_t *) ptr;
		ptr += 2;
		sample -> fineTune = nibbleValues [(*(uint8_t *) ptr) & 0xF];
		ptr ++;
		sample -> volume = BKClamp (*(uint8_t *) ptr, 0, BK_AMIGA_NUM_VOLUME_STEPS);
		ptr ++;
		sample -> repeatStart = *(uint16_t *) ptr;
		ptr += 2;
		sample -> repeatLength = *(uint16_t *) ptr;
		ptr += 2;

		if (!isBigEndian) {
			sample -> sampleLength = BK_REVERSE16 (sample -> sampleLength);
			sample -> repeatStart  = BK_REVERSE16 (sample -> repeatStart);
			sample -> repeatLength = BK_REVERSE16 (sample -> repeatLength);
		}

		sample -> volume = volumeValues [sample -> volume];
		sample -> sampleLength *= 2;
		sample -> repeatStart  *= 2;
		sample -> repeatLength *= 2;

		sample -> sampleOffset = sampleOffset;
		sampleOffset += sample -> sampleLength;
	}

	ptr = & header [950];
	mod -> songLength = *(uint8_t *) ptr;

	ptr = & header [952];
	memcpy (mod -> songOffsets, ptr, BK_AMIGA_MOD_NUM_PATTERNS);

	ptr = & header [1080];
	memcpy (mod -> identifier, ptr, 4);

	mod -> numPatterns = BKFileAmigaModGetMaxNumPatterns (mod);

	return 0;
}

static BKInt BKFileAmigaModReadPatterns (BKFileAmigaMod * mod)
{
	uint8_t patternData [sizeof (BKFileAmigaModPattern)];
	uint32_t const * channels;
	uint8_t const * channel;
	BKFileAmigaModPattern * pattern;
	BKFileAmigaModNote * row;
	size_t size;
	uint8_t * ptr;

	mod -> patterns = malloc (sizeof (BKFileAmigaModPattern) * mod -> numPatterns);

	if (mod -> patterns == NULL) {
		return BK_ALLOCATION_ERROR;
	}

	for (int i = 0; i < mod -> numPatterns; i ++) {
		size = fread (patternData, sizeof (uint8_t), sizeof (patternData) / sizeof (uint8_t), mod -> file);

		if (size < sizeof (BKFileAmigaModPattern)) {
			return BK_FILE_ERROR;
		}

		ptr = patternData;
		pattern = & mod -> patterns [i];

		for (int j = 0; j < BK_AMIGA_MOD_NUM_ROWS; j ++) {
			row      = pattern -> rows [j];
			channels = (void *) ptr;

			for (int k = 0; k < BK_AMIGA_MOD_NUM_CHANNELS; k ++) {
				channel = (void *) & channels [k];
				row [k].sample     = (channel [0] & 0xF0) | (channel [2] >> 4);
				row [k].note       = ((channel [0] & 0x0F) << 8) | channel [1];
				row [k].effect     = channel [2] & 0x0F;
				row [k].effectData = channel [3];

				// extended command
				if (row [k].effect == 0x0E) {
					row [k].effect = row [k].effectData >> 4;
					row [k].effectData &= 0x0F;
				}

				if (row [k].note) {
					float hz = 7093789.2 / (row [k].note * 2.f);
					float n = log2 (hz / 440.f) * 12.f - BK_C_3 - 3;

					row [k].note = round (n);
				}
			}

			ptr += 16;
		}
	}

	return 0;
}

static BKInt BKFileAmigaModCompile (BKFileAmigaMod * mod)
{
	// ...

	return 0;
}

static BKInt BKFileAmigaModReadSample (BKFileAmigaMod * mod)
{
	BKInt res;

	if ((res = BKDataInit (& mod -> sample)) < 0) {
		return res;
	}

	if ((res = BKDataLoadRaw (& mod -> sample, mod -> file, 1, BK_8_BIT_SIGNED)) < 0) {
		return res;
	}

	return 0;
}

BKInt BKFileAmigaModRead (BKFileAmigaMod * mod, FILE * file)
{
	BKInt res;

	mod -> file = file;

	if ((res = BKFileAmigaModReadHeader (mod)) < 0) {
		return res;
	}

	if ((res = BKFileAmigaModReadPatterns (mod)) < 0) {
		return res;
	}

	if ((res = BKFileAmigaModReadSample (mod)) < 0) {
		return res;
	}

	if ((res = BKFileAmigaModCompile (mod)) < 0) {
		return res;
	}

	return 0;
}

BKClass BKFileAmigaModClass =
{
	.instanceSize = sizeof (BKFileAmigaMod),
	.dispose      = (BKDisposeFunc) BKFileAmigaModDispose,
};
