/**
 * Copyright (c) 2012-2015 Simon Schoenenberger
 * http://blipkit.audio
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

#ifndef _BK_STRING_H_
#define _BK_STRING_H_

#include "BKObject.h"

typedef struct BKString BKString;

struct BKString
{
	BKObject object;
	BKSize   length;
	BKSize   capacity;
	char   * chars;
};

/**
 * Initialize static string object
 *
 * If `chars` is not NULL, `length` is the length of the optionally NUL-terminated
 * string `chars`. If `length` is -1, the length of `chars` is determined with
 * `strlen` and `chars` *must* be NUL-terminated.
 */
extern BKInt BKStringInit (BKString * string, char const * chars, BKSize length);

/**
 * Allocate string object with charcters
 *
 * If `chars` is not NULL, `length` is the length of the optionally NUL-terminated
 * string `chars`. If `length` is -1, the length of `chars` is determined with
 * `strlen` and `chars` *must* be NUL-terminated.
 */
extern BKInt BKStringAlloc (BKString ** outString, char const * chars, BKSize length);

/**
 * Get length of string
 */
extern BKSize BKStringGetLength (BKString const * string);

/**
 * Append string object
 */
extern BKInt BKStringAppend (BKString * string, BKString const * tail);

/**
 * Append NUL-terminated string
 */
extern BKInt BKStringAppendChars (BKString * string, char const * chars);

/**
 * Empty string
 *
 * If `keepData` is 1 the allocated buffer is reused
 */
extern void BKStringEmpty (BKString * string, BKInt keepData);

/**
 * Returns 1 if `chars1` is equal to `chars2`
 */
extern BKInt BKStringCharsIsEqual (char const * chars1, BKSize length1, char const * chars2, BKSize length2);

/**
 * Returns 1 if `string` is equal to `string2`
 */
extern BKInt BKStringIsEqual (BKString const * string, BKString const * string2);

/**
 * Returns 1 if `string` is equal to `chars`
 */
extern BKInt BKStringIsEqualToChars (BKString const * string, char const * chars);

/**
 * Returns 1 if string begins with other string
 */
extern BKInt BKStringBeginsWith (BKString const * string, BKString const * head);

/**
 * Returns 1 if string begins with chars
 */
extern BKInt BKStringBeginsWithChars (BKString const * string, char const * chars);

/**
 * Returns 1 if string ends with other string
 */
extern BKInt BKStringEndsWith (BKString const * string, BKString const * tail);

/**
 * Returns 1 if string ends with chars
 */
extern BKInt BKStringEndsWithChars (BKString const * string, char const * chars);

/**
 * Replaces a range with a substring
 */
extern BKInt BKStringReplaceInRange (BKString * string, BKString const * replace, BKSize replaceOffset, BKSize replaceLength);

/**
 * Replaces a range with characters
 */
extern BKInt BKStringReplaceCharsInRange (BKString * string, char const * replace, BKSize length, BKSize replaceOffset, BKSize replaceLength);

/**
 * Get base name of path
 *
 * If `outBasename` is NULL, a new string is allocated and returned
 * If `outBasename` is an existing string, the content is overwritten
 */
extern BKInt BKStringGetBasename (BKString ** outBasename, BKString const * path);

/**
 * Get directory name of path
 *
 * If `outDirname` is NULL, a new string is allocated and returned
 * If `outDirname` is an existing string, the content is overwritten
 */
extern BKInt BKStringGetDirname (BKString ** outDirname, BKString const * path);

#endif /* ! _BK_STRING_H_ */
