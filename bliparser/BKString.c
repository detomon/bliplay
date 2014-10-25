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

#include <assert.h>
#include "BKString.h"

#define MIN_CAPACITY 16

enum BKStringFlag
{
	BKStringFlagAllocated = 1 << 0,
};

static BKSize BKStrlen (char const * chars)
{
	return chars ? strlen (chars) : 0;
}

static BKSize BKStrcmp (char const * chars1, BKSize length1, char const * chars2, BKSize length2)
{
	if (length1 != length2) {
		return 0;
	}

	return memcmp (chars1, chars2, length1) == 0;
}

static BKInt BKStringGrow (BKString * string, BKSize minCapacity)
{
	void * newChars;
	BKSize newCapacity;

	newCapacity = BKMax (string -> capacity + minCapacity, string -> capacity * 1.5);
	newCapacity = BKMax (MIN_CAPACITY, newCapacity);

	newChars = realloc (string -> chars, newCapacity);

	if (newChars == NULL) {
		return -1;
	}

	string -> capacity = newCapacity;
	string -> chars    = newChars;

	return 0;
}

static BKInt BKStringAppendCharsWithLength (BKString * string, char const * chars, BKSize length)
{
	if (string -> length + length >= string -> capacity) {
		// + terminating NUL char
		if (BKStringGrow (string, length + 1) < 0) {
			return -1;
		}
	}

	memcpy (& string -> chars [string -> length], chars, length);
	string -> length += length;
	string -> chars [string -> length] = '\0';

	return 0;
}

BKInt BKStringInit (BKString * string, char const * chars, BKSize length)
{
	memset (string, 0, sizeof (* string));

	if (chars) {
		if (length < 0) {
			length = BKStrlen (chars);
		}

		if (BKStringAppendCharsWithLength (string, chars, length) < 0) {
			return -1;
		}
	}

	return 0;
}

BKInt BKStringAlloc (BKString ** outString, char const * chars, BKSize length)
{
	BKInt res;
	BKString * string = malloc (sizeof (* string));

	* outString = NULL;

	if (string == NULL) {
		return -1;
	}

	res = BKStringInit (string, chars, length);

	if (res != 0) {
		free (string);
		return res;
	}

	string -> flags |= BKStringFlagAllocated;
	* outString = string;

	return 0;
}

void BKStringDispose (BKString * string)
{
	if (string == NULL) {
		return;
	}

	if (string -> chars) {
		free (string -> chars);
	}

	if (string -> flags & BKStringFlagAllocated) {
		free (string);
	}

	memset (string, 0, sizeof (* string));
}

BKSize BKStringGetLength (BKString const * string)
{
	return string ? string -> length : 0;
}

BKInt BKStringAppend (BKString * string, BKString const * tail)
{
	if (tail == NULL) {
		return 0;
	}

	return BKStringAppendCharsWithLength (string, tail -> chars, tail -> length);
}

BKInt BKStringAppendChars (BKString * string, char const * chars)
{
	if (chars == NULL) {
		return 0;
	}

	return BKStringAppendCharsWithLength (string, chars, BKStrlen (chars));
}

void BKStringEmpty (BKString * string, BKInt keepData)
{
	if (string == NULL) {
		return;
	}

	if (keepData == 0) {
		if (string -> chars) {
			free (string -> chars);
			string -> chars = NULL;
		}

		string -> capacity = 0;
	}

	if (string -> chars) {
		string -> chars [0] = '\0';
	}

	string -> length = 0;
}

BKInt BKStringCharsIsEqual (char const * chars1, BKSize length1, char const * chars2, BKSize length2)
{
	if (length1 != length2) {
		return 0;
	}

	// length2 is also 0 here
	if (length1 == 0) {
		return 1;
	}

	return BKStrcmp (chars1, length1, chars2, length2);
}

BKInt BKStringIsEqual (BKString const * string1, BKString const * string2)
{
	BKSize length1, length2;

	length1 = BKStringGetLength (string1);
	length2 = BKStringGetLength (string2);

	return BKStringCharsIsEqual (string1 -> chars, length1, string2 -> chars, length2);
}

BKInt BKStringIsEqualToChars (BKString const * string, char const * chars)
{
	BKSize length1, length2;

	length1 = BKStringGetLength (string);
	length2 = BKStrlen (chars);

	return BKStringCharsIsEqual (string -> chars, length1, chars, length2);
}

static BKInt BKStringCharsBeginsWithChars (char const * chars1, BKSize length1, char const * chars2, BKSize length2)
{
	if (length2 > length1) {
		return 0;
	}

	return memcmp (chars1, chars2, length2) == 0;
}

BKInt BKStringBeginsWith (BKString const * string, BKString const * tail)
{
	if (string == NULL || tail == NULL) {
		return 0;
	}

	return BKStringCharsBeginsWithChars (string -> chars, string -> length, tail -> chars, tail -> length);
}

BKInt BKStringBeginsWithChars (BKString const * string, char const * head)
{
	if (string == NULL || head == NULL) {
		return 0;
	}

	return BKStringCharsBeginsWithChars (string -> chars, string -> length, head, BKStrlen (head));
}

static BKInt BKStringCharsEndWithChars (char const * chars1, BKSize length1, char const * chars2, BKSize length2)
{
	if (length2 > length1) {
		return 0;
	}

	return memcmp (chars1 + (length2 - length1), chars2, length2) == 0;
}

BKInt BKStringEndsWith (BKString const * string, BKString const * tail)
{
	if (string == NULL || tail == NULL) {
		return 0;
	}

	return BKStringCharsEndWithChars (string -> chars, string -> length, tail -> chars, tail -> length);
}

BKInt BKStringEndsWithChars (BKString const * string, char const * tail)
{
	if (string == NULL || tail == NULL) {
		return 0;
	}

	return BKStringCharsEndWithChars (string -> chars, string -> length, tail, BKStrlen (tail));
}

BKInt BKStringReplaceInRange (BKString * string, BKString const * replace, BKSize replaceOffset, BKSize replaceLength)
{
	BKSize length = BKStringGetLength (replace);

	return BKStringReplaceCharsInRange (string, replace ? replace -> chars : NULL, length, replaceOffset, replaceLength);
}

BKInt BKStringReplaceCharsInRange (BKString * string, char const * replace, BKSize length, BKSize replaceOffset, BKSize replaceLength)
{
	BKSize stringLength, sizeDiff, replaceEnd;

	if (string == NULL) {
		return -1;
	}

	stringLength = string -> length;


	replaceOffset = BKClamp (replaceOffset, 0, stringLength);
	replaceLength = BKMin (replaceLength, stringLength - replaceOffset);
	replaceEnd    = replaceOffset + replaceLength;

	sizeDiff = length - replaceLength;

	if (sizeDiff > 0) {
		// 1 terminating NUL char
		if (BKStringGrow (string, sizeDiff + 1)) {
			return -1;
		}
	}

	if (sizeDiff >= 0) {
		memmove (& string -> chars [replaceEnd + sizeDiff], & string -> chars [replaceEnd], stringLength - replaceEnd);
		memcpy (& string -> chars [replaceOffset], replace, length);
	}
	else {
		memcpy (& string -> chars [replaceOffset], replace, length);
		memmove (& string -> chars [replaceEnd + sizeDiff], & string -> chars [replaceEnd], stringLength - replaceEnd);
	}

	string -> length += sizeDiff;
	string -> chars [string -> length] = '\0';

	return 0;
}

/**
 * Returns index of first char from right not in `stripChars`
 */
static BKInt BKStringCharsStripRightChars (char const * chars, BKSize length, char const * strip, BKSize slength)
{
	BKInt i, j;
	BKInt found;

	if (length == 0 || slength == 0) {
		return length;
	}

	for (i = length - 1; i >= 0; i --) {
		found = 0;

		for (j = 0; j < slength; j ++) {
			if (chars [i] == strip [j]) {
				found = 1;
				break;
			}
		}

		if (!found) {
			break;
		}
	}

	return i + 1;
}

/**
 * Returns index of first char from right in `stripChars`
 *
 * If no character is found, -1 is returned
 */
static BKInt BKStringCharsStripRightToChars (char const * chars, BKSize length, char const * nstrip, BKSize slength)
{
	BKInt i, j;
	BKInt found;

	if (length == 0 || slength == 0) {
		return length;
	}

	for (i = length - 1; i >= 0; i --) {
		found = 0;

		for (j = 0; j < slength; j ++) {
			if (chars [i] == nstrip [j]) {
				found = 1;
				break;
			}
		}

		if (found) {
			return i;
		}
	}

	return -1;
}

static void BKStringGetBasenameOffset (char const * chars, BKSize length, BKSize * outStart, BKSize * outEnd)
{
	*outEnd   = BKStringCharsStripRightChars (chars, length, "/\\", 2);
	*outStart = BKStringCharsStripRightToChars (chars, *outEnd, "/\\", 2) + 1;
}

BKInt BKStringGetBasename (BKString ** outBasename, BKString const * path)
{
	BKSize length, start, end;

	if (*outBasename == NULL) {
		if (BKStringAlloc (outBasename, NULL, 0) < 0) {
			return -1;
		}
	}

	length = BKStringGetLength (path);
	BKStringGetBasenameOffset (path -> chars, length, & start, & end);

	if (BKStringAppendCharsWithLength (*outBasename, & path -> chars [start], end - start)) {
		return -1;
	}

	return 0;
}

BKInt BKStringGetDirname (BKString ** outDirname, BKString const * path)
{
	BKSize length, start, end;

	if (*outDirname == NULL) {
		if (BKStringAlloc (outDirname, NULL, 0) < 0) {
			return -1;
		}
	}

	length = BKStringGetLength (path);
	BKStringGetBasenameOffset (path -> chars, length, & start, & end);

	end = BKStringCharsStripRightChars (path -> chars, start, "/\\", 2);
	start = 0;

	if (start == end) {
		if (BKStringBeginsWithChars (path, "/")) {
			if (BKStringAppendCharsWithLength (*outDirname, "/", 1)) {
				return -1;
			}
		}
		else {
			if (BKStringAppendCharsWithLength (*outDirname, ".", 1)) {
				return -1;
			}
		}
	}
	else {
		if (BKStringAppendCharsWithLength (*outDirname, & path -> chars [start], end - start)) {
			return -1;
		}
	}

	return 0;
}
