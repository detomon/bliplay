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

#ifndef _BK_WAVE_FILE_WRITER_H_
#define _BK_WAVE_FILE_WRITER_H_

#include <stdio.h>
#include "BKBase.h"
#include "BKData.h"

typedef struct BKWaveFileWriter BKWaveFileWriter;

struct BKWaveFileWriter
{
	FILE * file;
	BKInt  sampleRate;
	BKInt  numChannels;
	size_t fileSize;
	size_t dataSize;
	BKInt  reverseEndian;
};

/**
 * Initialize WAVE file writer object
 *
 * Prepare a writer object to write frames to an opened and writable file
 * `file`. Number of channels `numChannels` defines the layout of the frames
 * which will be appended. `sampleRate` defines the play speed.
 */
extern BKInt BKWaveFileWriterInit (BKWaveFileWriter * writer, FILE * file, BKInt numChannels, BKInt sampleRate);

/**
 * Dispose writer object
 *
 * This doesn't close the file given at initialization.
 */
extern void BKWaveFileWriterDispose (BKWaveFileWriter * writer);

/**
 * Append frames to WAVE file
 *
 * Write `frames` with length `numFrames` to WAVE file. The number of frames
 * should be a multiple of `numChannels` given at initialization.
 */
extern BKInt BKWaveFileWriterAppendFrames (BKWaveFileWriter * writer, BKFrame const * frames, BKInt numFrames);

/**
 * Terminate WAVE file
 *
 * If no more frames will be appended this function must be called to set the
 * required WAVE header values.
 */
extern BKInt BKWaveFileWriterTerminate (BKWaveFileWriter * writer);

/**
 * Write data object to WAVE file
 *
 * `file` must be an opened and writable file. `data` is a data object
 * containing the sound data. Data objects do not carry the sample rate of their
 * frames so the sample rate has to be given with `sampleRate`.
 */
extern BKInt BKWaveFileWriteData (FILE * file, BKData const * data, BKInt sampleRate);

#endif /* ! _BK_WAVE_FILE_WRITER_H_ */
