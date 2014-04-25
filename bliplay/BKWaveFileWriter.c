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

#include "BKWaveFileWriter.h"

/**
 * https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 */

typedef struct BKWaveFileHeader     BKWaveFileHeader;
typedef struct BKWaveFileHeaderFmt  BKWaveFileHeaderFmt;
typedef struct BKWaveFileHeaderData BKWaveFileHeaderData;

struct BKWaveFileHeader
{
	char     chunkID [4];
	uint32_t chunkSize;
	char     format [4];
};

struct BKWaveFileHeaderFmt
{
	char     subchunkID [4];
	uint32_t subchunkSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};

struct BKWaveFileHeaderData
{
	char     subchunkID [4];
	uint32_t subchunkSize;
	char     data [];
};

static BKWaveFileHeader const waveFileHeader =
{
	.chunkID   = "RIFF",
	.chunkSize = 0,
	.format    = "WAVE",
};

static BKWaveFileHeaderFmt const waveFileHeaderFmt =
{
	.subchunkID    = "fmt ",
	.subchunkSize  = 16,
	.audioFormat   = 1,
	.numChannels   = 2,
	.sampleRate    = 44100,
	.byteRate      = 44100 * 2 * 16/8,
	.blockAlign    = 2 * 16/8,
	.bitsPerSample = 16,
};

static BKWaveFileHeaderData const waveFileHeaderData =
{
	.subchunkID   = "data",
	.subchunkSize = 0,
};

static BKInt BKSystemIsBigEndian (void)
{
	union { BKUInt i; char c [4]; } sentinel;

	sentinel.i = 0x01020304;

	return sentinel.c[0] == 0x01;
}

static uint32_t BKInt32Reverse (uint32_t i)
{
	char c;
	union { uint32_t i; char c [4]; } sentinel;

	sentinel.i = i;
	c = sentinel.c [0]; sentinel.c [0] = sentinel.c [3]; sentinel.c [3] = c;
	c = sentinel.c [1]; sentinel.c [1] = sentinel.c [2]; sentinel.c [2] = c;
	i = sentinel.i;

	return i;
}

static uint16_t BKInt16Reverse (uint16_t i)
{
	char c;
	union { uint16_t i; char c [2]; } sentinel;

	sentinel.i = i;
	c = sentinel.c [0]; sentinel.c [0] = sentinel.c [1]; sentinel.c [1] = c;
	i = sentinel.i;

	return i;
}

BKInt BKWaveFileWriterInit (BKWaveFileWriter * writer, FILE * file, BKInt numChannels, BKInt sampleRate)
{
	memset (writer, 0, sizeof (* writer));

	writer -> file          = file;
	writer -> sampleRate    = sampleRate;
	writer -> numChannels   = numChannels;
	writer -> reverseEndian = BKSystemIsBigEndian ();

	return 0;
}

void BKWaveFileWriterDispose (BKWaveFileWriter * writer)
{
	memset (writer, 0, sizeof (* writer));
}

static BKInt BKWaveFileWriterWriteHeader (BKWaveFileWriter * writer)
{
	BKInt                numChannels, sampleRate, numBits;
	BKWaveFileHeader     header;
	BKWaveFileHeaderFmt  fmtHeader;
	BKWaveFileHeaderData dataHeader;

	header     = waveFileHeader;
	fmtHeader  = waveFileHeaderFmt;
	dataHeader = waveFileHeaderData;

	numChannels = writer -> numChannels;
	sampleRate  = writer -> sampleRate;
	numBits     = 16;

	fmtHeader.numChannels   = numChannels;
	fmtHeader.sampleRate    = sampleRate;
	fmtHeader.bitsPerSample = numBits;
	fmtHeader.blockAlign    = numChannels * numBits / 8;
	fmtHeader.byteRate      = sampleRate * fmtHeader.blockAlign;

	if (writer -> reverseEndian) {
		fmtHeader.subchunkSize  = BKInt32Reverse (fmtHeader.subchunkSize);
		fmtHeader.numChannels   = BKInt16Reverse (fmtHeader.numChannels);
		fmtHeader.sampleRate    = BKInt32Reverse (fmtHeader.sampleRate);
		fmtHeader.bitsPerSample = BKInt16Reverse (fmtHeader.bitsPerSample);
		fmtHeader.blockAlign    = BKInt16Reverse (fmtHeader.blockAlign);
		fmtHeader.byteRate      = BKInt32Reverse (fmtHeader.byteRate);
	}

	fwrite (& header, sizeof (header), 1, writer -> file);
	fwrite (& fmtHeader, sizeof (fmtHeader), 1, writer -> file);
	fwrite (& dataHeader, sizeof (dataHeader), 1, writer -> file);

	writer -> fileSize += ftell (writer -> file) - 8;

	return 0;
}

BKInt BKWaveFileWriterAppendFrames (BKWaveFileWriter * writer, BKFrame const * frames, BKInt numFrames)
{
	size_t  dataSize;
	size_t  writeSize;
	size_t  bufferSize = 1024;
	BKFrame buffer [bufferSize];

	if (writer -> dataSize == 0) {
		if (BKWaveFileWriterWriteHeader (writer) < 0)
			return -1;
	}

	dataSize = sizeof (BKFrame) * numFrames;

	if (writer -> reverseEndian) {
		while (numFrames) {
			writeSize = BKMin (bufferSize, numFrames);

			for (BKInt i = 0; i < writeSize; i ++)
				buffer [i] = BKInt16Reverse (frames [i]);

			frames += writeSize;
			numFrames -= writeSize;

			fwrite (buffer, sizeof (BKFrame), writeSize, writer -> file);
		}
	}
	else {
		fwrite (frames, sizeof (BKFrame), numFrames, writer -> file);
	}

	writer -> fileSize += dataSize;
	writer -> dataSize += dataSize;

	return 0;
}

BKInt BKWaveFileWriterTerminate (BKWaveFileWriter * writer)
{
	off_t                offset;
	BKWaveFileHeader     header;
	BKWaveFileHeaderData dataHeader;

	offset = 0;
	fseek (writer -> file, offset, SEEK_SET);
	fread (& header, sizeof (header), 1, writer -> file);

	header.chunkSize = writer -> fileSize;

	if (writer -> reverseEndian)
		header.chunkSize = BKInt32Reverse (header.chunkSize);

	fseek (writer -> file, offset, SEEK_SET);
	fwrite (& header, sizeof (header), 1, writer -> file);

	offset = sizeof (BKWaveFileHeader) + sizeof (BKWaveFileHeaderFmt);
	fseek (writer -> file, offset, SEEK_SET);
	fread (& dataHeader, sizeof (dataHeader), 1, writer -> file);

	dataHeader.subchunkSize = writer -> dataSize;

	if (writer -> reverseEndian)
		dataHeader.subchunkSize = BKInt32Reverse (dataHeader.subchunkSize);

	fseek (writer -> file, offset, SEEK_SET);
	fwrite (& dataHeader, sizeof (dataHeader), 1, writer -> file);

	fseek (writer -> file, 0, SEEK_END);
	fflush (writer -> file);

	return 0;
}

BKInt BKWaveFileWriteData (FILE * file, BKData const * data, BKInt sampleRate)
{
	BKWaveFileWriter writer;

	if (BKWaveFileWriterInit (& writer, sampleRate, data -> numChannels, file) < 0)
		return -1;

	BKWaveFileWriterAppendFrames (& writer, data -> frames, data -> numFrames * data -> numChannels);
	BKWaveFileWriterTerminate (& writer);

	BKWaveFileWriterDispose (& writer);

	return 0;
}
