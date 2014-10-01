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

#include "BKWaveFileReader.h"
#include "BKWaveFile_internal.h"

BKInt BKWaveFileReaderInit (BKWaveFileReader * reader, FILE * file)
{
	memset (reader, 0, sizeof (* reader));

	reader -> file = file;

	return 0;
}

void BKWaveFileReaderDispose (BKWaveFileReader * reader)
{
	memset (reader, 0, sizeof (* reader));
}

static void BKWaveFileHeaderFmtRead (BKWaveFileHeaderFmt * headerFmt)
{
	if (BKSystemIsBigEndian ()) {
		headerFmt -> subchunkSize  = BKInt32Reverse (headerFmt -> subchunkSize);
		headerFmt -> audioFormat   = BKInt16Reverse (headerFmt -> audioFormat);
		headerFmt -> numChannels   = BKInt16Reverse (headerFmt -> numChannels);
		headerFmt -> sampleRate    = BKInt32Reverse (headerFmt -> sampleRate);
		headerFmt -> byteRate      = BKInt32Reverse (headerFmt -> byteRate);
		headerFmt -> blockAlign    = BKInt16Reverse (headerFmt -> blockAlign);
		headerFmt -> bitsPerSample = BKInt16Reverse (headerFmt -> bitsPerSample);
	}
}

static void BKWaveFileHeaderDataRead (BKWaveFileHeaderData * headerData)
{
	if (BKSystemIsBigEndian ()) {
		headerData ->  subchunkSize = BKInt32Reverse (headerData ->  subchunkSize);
	}
}

BKInt BKWaveFileReaderReadHeader (BKWaveFileReader * reader, BKInt * outNumChannels, BKInt * outSampleRate, BKInt * outNumFrames)
{
	BKSize size;
	BKSize frameSize;
	BKWaveFileHeader header;
	BKWaveFileHeaderFmt headerFmt;
	BKWaveFileHeaderData headerData;

	if (reader -> sampleRate == 0) {
		size = fread (& header, 1, sizeof (header), reader -> file);

		if (size < sizeof (header))
			return -1;

		if (memcmp (header.chunkID, "RIFF", 4) != 0)
			return -1;

		if (memcmp (header.format, "WAVE", 4) != 0)
			return -1;

		size = fread (& headerFmt, 1, sizeof (headerFmt), reader -> file);

		if (size < sizeof (headerFmt))
			return -1;

		BKWaveFileHeaderFmtRead (& headerFmt);

		if (memcmp (headerFmt.subchunkID, "fmt ", 4) != 0)
			return -1;

		if (headerFmt.audioFormat != 1)
			return -1;

		if (headerFmt.bitsPerSample != 8 && headerFmt.bitsPerSample != 16)
			return -1;

		size = fread (& headerData, 1, sizeof (headerData), reader -> file);

		if (size < sizeof (headerData))
			return -1;

		BKWaveFileHeaderDataRead (& headerData);

		if (memcmp (headerData.subchunkID, "data", 4) != 0)
			return -1;

		reader -> sampleRate  = headerFmt.sampleRate;
		reader -> numChannels = headerFmt.numChannels;
		reader -> numBits     = headerFmt.bitsPerSample;

		switch (reader -> numBits) {
			case 8: {
				frameSize = 1;
				break;
			}
			default:
			case 16: {
				frameSize = 2;
				break;
			}
		}

		reader -> dataSize  = headerData.subchunkSize;
		reader -> numFrames = (BKInt)reader -> dataSize / reader -> numChannels / frameSize;
	}

	* outNumChannels = reader -> numChannels;
	* outSampleRate  = reader -> sampleRate;
	* outNumFrames   = reader -> numFrames;

	return 0;
}

BKInt BKWaveFileReaderReadFrames (BKWaveFileReader * reader, BKFrame outFrames [])
{
	char    buffer [1024];
	char  * bufferPtr;
	BKSize  readSize, writeSize;
	BKSize  remainingSize;
	BKSize  frameSize;
	BKInt   reverseEndian;
	BKFrame frame;

	switch (reader -> numBits) {
		case 8: {
			frameSize = 1;
			break;
		}
		default:
		case 16: {
			frameSize = 2;
			break;
		}
	}

	writeSize     = 0;
	reverseEndian = BKSystemIsBigEndian ();
	remainingSize = reader -> dataSize;

	while (remainingSize) {
		readSize = BKMin (remainingSize, sizeof (buffer));
		readSize = readSize & ((1 << reader -> numBits) - 1);
		readSize = fread (buffer, 1, readSize, reader -> file);
		readSize = readSize & ((1 << reader -> numBits) - 1);

		bufferPtr = buffer;

		remainingSize -= readSize;
		writeSize += readSize;

		while (readSize) {
			switch (frameSize) {
				case 1: {
					frame = ((BKFrame) (* (char *) bufferPtr)) << 8;
					break;
				}
				default:
				case 2: {
					frame = (* (BKFrame *) bufferPtr);
					if (reverseEndian)
						frame = BKInt16Reverse (frame);
					break;
				}
			}

			(* outFrames) = frame;

			bufferPtr += frameSize;
			readSize -= frameSize;
			outFrames ++;
		}
	}

	memset (outFrames, 0, reader -> dataSize - writeSize);

	return 0;
}
