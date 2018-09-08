/*
 * Copyright (c) 2012-2016 Simon Schoenenberger
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

#include "BKTKTokenizer.h"

#define BUF_INIT_LEN 4096
#define MIN_BUFFER_FREE_SPACE 256
#define CHAR_END 256

static BKTKType const tokenChars [257] =
{
	[(uint8_t) ':']    = BKTKTypeArgSep,
	[(uint8_t) ';']    = BKTKTypeCmdSep,
	[(uint8_t) '[']    = BKTKTypeGrpOpen,
	[(uint8_t) ']']    = BKTKTypeGrpClose,
	[(uint8_t) '"']    = BKTKTypeString,
	[(uint8_t) '!']    = BKTKTypeData,
	[(uint8_t) '%']    = BKTKTypeComment,
	[(uint8_t) ' ']    = BKTKTypeSpace,
	[(uint8_t) '\xa0'] = BKTKTypeSpace, // no-break space
	[(uint8_t) '\\']   = BKTKTypeEscape,
	[(uint8_t) '\a']   = BKTKTypeSpace,
	[(uint8_t) '\b']   = BKTKTypeSpace,
	[(uint8_t) '\f']   = BKTKTypeSpace,
	[(uint8_t) '\n']   = BKTKTypeLineBreak,
	[(uint8_t) '\r']   = BKTKTypeLineBreak,
	[(uint8_t) '\t']   = BKTKTypeSpace,
	[(uint8_t) '\v']   = BKTKTypeSpace,
	[CHAR_END]         = BKTKTypeEnd,
};

static uint8_t const escapeChars [256] =
{
	['a'] = '\a',
	['b'] = '\b',
	['f'] = '\f',
	['n'] = '\n',
	['r'] = '\r',
	['t'] = '\t',
	['v'] = '\v',
	['x'] = 'h',
};

/**
 * Base64 conversion table
 *
 * Converts a base64 character to its representing integer value.
 * '-' is an alias of '+', likewise '_' is an alias of '/'.
 * Invalid characters have the value -1.
 */
static signed char const base64Chars [256] =
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static signed char const hexChars [256] =
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

extern BKClass const BKTKTokenizerClass;

BKInt BKTKTokenizerInit (BKTKTokenizer * tok)
{
	BKInt res;

	if ((res = BKObjectInit (tok, &BKTKTokenizerClass, sizeof (*tok))) != 0) {
		return res;
	}

	tok -> bufferCap = BUF_INIT_LEN;
	tok -> buffer = malloc (tok -> bufferCap);

	if (!tok -> buffer) {
		BKDispose (tok);

		return -1;
	}

	BKTKTokenizerReset (tok);

	return 0;
}

static void BKTKTokenizerDispose (BKTKTokenizer * tok)
{
	free (tok -> buffer);
}

void BKTKTokenizerReset (BKTKTokenizer * tok)
{
	tok -> state         = BKTKStateRoot;
	tok -> offset.lineno = 1;
	tok -> offset.colno  = 0;
	tok -> bufferLen     = 0;
	tok -> base64Len     = 0;
	tok -> charCount     = 0;
	tok -> charValue     = 0;
}

/**
 * Relocate data of tokend to new allocated buffer
 */
static void BKTKTokenizerRelocateTokenData (BKTKTokenizer * tok, uint8_t * newBuffer)
{
	BKTKToken * token;

	token = &tok -> token;
	token -> data = newBuffer + (token -> data - tok -> buffer);
}

static BKInt BKTKTokenizerEnsureBufferSpace (BKTKTokenizer * tok, BKUSize additionalSize)
{
	uint8_t * newBuffer;
	BKUSize newCapacity;

	if (tok -> bufferLen + additionalSize + MIN_BUFFER_FREE_SPACE >= tok -> bufferCap) {
		newCapacity = BKNextPow2 (tok -> bufferCap + additionalSize + MIN_BUFFER_FREE_SPACE);
		newBuffer = realloc (tok -> buffer, newCapacity);

		if (!newBuffer) {
			return -1;
		}

		BKTKTokenizerRelocateTokenData (tok, newBuffer);

		tok -> buffer = newBuffer;
		tok -> bufferCap = newCapacity;
	}

	return 0;
}

static void BKTKTokenizerBufferPutChar (BKTKTokenizer * tok, BKInt c)
{
	tok -> buffer [tok -> bufferLen ++] = c;
}

static void BKTKTokenizerBufferPutBase64Char (BKTKTokenizer * tok, BKInt c)
{
	BKUInt value;
	BKInt v = base64Chars [c];

	if (v < 0) {
		return;
	}

	tok -> base64Value = (tok -> base64Value << 6) | v;
	tok -> base64Len ++;

	if (tok -> base64Len >= 4) {
		value = tok -> base64Value;

		BKTKTokenizerBufferPutChar (tok, (value >> 16) & 0xFF);
		BKTKTokenizerBufferPutChar (tok, (value >>  8) & 0xFF);
		BKTKTokenizerBufferPutChar (tok, (value >>  0) & 0xFF);

		tok -> base64Len = 0;
		tok -> base64Value = 0;
	}
}

static void BKTKTokenizerBufferEndBase64 (BKTKTokenizer * tok)
{
	BKUInt value;

	value = tok -> base64Value;
	value <<= (4 - tok -> base64Len) * 6; // align left of 3 byte chunk

	switch (tok -> base64Len) {
		case 1: { // actually an error
			BKTKTokenizerBufferPutChar (tok, (value >> 16) & 0xFF);
			break;
		}
		case 2: {
			BKTKTokenizerBufferPutChar (tok, (value >> 16) & 0xFF);
			break;
		}
		case 3: {
			BKTKTokenizerBufferPutChar (tok, (value >> 16) & 0xFF);
			BKTKTokenizerBufferPutChar (tok, (value >>  8) & 0xFF);
			break;
		}
	}
}

static void BKTKTokenizerEndBuffer (BKTKTokenizer * tok)
{
	// terminate current buffer segment
	tok -> buffer [tok -> bufferLen ++] = '\0';
	// reset buffer
	tok -> bufferLen = 0;
}

static void BKTKTokenizerSetError (BKTKTokenizer * tok, char const * msg, ...)
{
	va_list args;

	va_start (args, msg);
	vsnprintf ((void *) tok -> buffer, tok -> bufferCap, msg, args);
	va_end (args);

	tok -> bufferLen = BKStrnlen ((char *) tok -> buffer, tok -> bufferCap);
}

static BKInt BKTKTokenizerPutCharsChunk (BKTKTokenizer * tok, uint8_t const * chars, BKUSize size, BKTKPutTokenFunc putToken, void * arg)
{
	BKInt c;
	BKInt res = -1;
	BKInt accept;
	BKInt again;
	BKTKState state;
	BKTKType type, charType;
	BKTKToken * token;
	BKTKOffset offset;
	uint8_t const * end = &chars [size];

	if (tok -> state >= BKTKStateEnd) {
		return 0;
	}

	// ensure space for worst case
	if (BKTKTokenizerEnsureBufferSpace (tok, size * 2) < 0) {
		goto allocationError;
	}

	state = tok -> state;
	offset = tok -> offset;

	do {
		if (chars < end) {
			c = *chars ++;
		}
		else if (size == 0) {
			c = -1;
		}
		else {
			// need new data
			break;
		}

		if (c < 0) {
			charType = BKTKTypeEnd;
		}
		else {
			charType = tokenChars [c];
		}

		type = BKTKTypeNone;

		if (charType == BKTKTypeLineBreak) {
			offset.lineno ++;
			offset.colno = 0;
		}
		else {
			offset.colno ++;
		}

		do {
			again = 0;
			accept = 0;

			// parser states
			switch (state) {
				case BKTKStateRoot: {
					int capture = 1;

					switch (charType) {
						case BKTKTypeSpace: {
							state = BKTKStateSpace;
							type = BKTKTypeSpace;
							capture = 0;
							break;
						}
						case BKTKTypeString: {
							state = BKTKStateStringStart;
							type = BKTKTypeString;
							break;
						}
						case BKTKTypeData: {
							state = BKTKStateDataStart;
							type = BKTKTypeData;
							break;
						}
						case BKTKTypeComment: {
							state = BKTKStateCommentStart;
							type = BKTKTypeComment;
							break;
						}
						case BKTKTypeGrpOpen: {
							type = BKTKTypeGrpOpen;
							accept = 1;
							break;
						}
						case BKTKTypeGrpClose: {
							type = BKTKTypeGrpClose;
							accept = 1;
							break;
						}
						case BKTKTypeArgSep: {
							type = BKTKTypeArgSep;
							accept = 1;
							break;
						}
						case BKTKTypeCmdSep: {
							type = BKTKTypeCmdSep;
							accept = 1;
							break;
						}
						case BKTKTypeLineBreak: {
							type = BKTKTypeLineBreak;
							accept = 1;
							break;
						}
						case BKTKTypeOther: {
							state = BKTKStateArg;
							type = BKTKTypeArg;
							break;
						}
						case BKTKTypeEnd: {
							state = BKTKStateEnd;
							type = BKTKTypeEnd;
							break;
						}
						default: {
							BKTKTokenizerSetError (tok, "Unknown token on line %u:%u",
								offset.lineno, offset.colno);
							goto error;
							break;
						}
					}

					if (capture) {
						token = &tok -> token;
						token -> type   = type;
						token -> data   = &tok -> buffer [tok -> bufferLen];
						token -> offset = offset;
					}

					break;
				}
				case BKTKStateArg: {
					switch (charType) {
						case BKTKTypeSpace: {
							state = BKTKStateSpace;
							accept = 1;
							break;
						}
						case BKTKTypeString: {
							BKTKTokenizerSetError (tok, "Unexpected string literal on line %u:%u",
								offset.lineno, offset.colno);
							goto error;
							break;
						}
						case BKTKTypeData: {
							BKTKTokenizerSetError (tok, "Unexpected data literal on line %u:%u",
								offset.lineno, offset.colno);
							goto error;
							break;
						}
						case BKTKTypeArgSep:
						case BKTKTypeCmdSep:
						case BKTKTypeLineBreak:
						case BKTKTypeGrpOpen:
						case BKTKTypeGrpClose:
						case BKTKTypeComment: {
							state = BKTKStateRoot;
							accept = 1;
							again = 1;
							break;
						}
						case BKTKTypeEnd: {
							state = BKTKStateRoot;
							accept = 1;
							again = 1;
							break;
						}
						default: {
							break;
						}
					}
					break;
				}
				case BKTKStateStringStart:
				case BKTKStateString: {
					switch (charType) {
						case BKTKTypeString: {
							state = BKTKStateRoot;
							accept = 1;
							break;
						}
						case BKTKTypeEscape: {
							state = BKTKStateStringEsc;
							break;
						}
						case BKTKTypeEnd: {
							token = &tok -> token;
							BKTKTokenizerSetError (tok, "Premature end of string starting on line %u:%u",
								token -> offset.lineno, token -> offset.colno);
							goto error;
							break;
						}
						default: {
							state = BKTKStateString;
							break;
						}
					}
					break;
				}
				case BKTKStateStringEsc: {
					switch (charType) {
						case BKTKTypeEnd: {
							token = &tok -> token;
							BKTKTokenizerSetError (tok, "Premature end of string starting on line %u:%u",
								token -> offset.lineno, token -> offset.colno);
							goto error;
							break;
						}
						default: {
							if (escapeChars [c]) {
								c = escapeChars [c];

								if (c == 'h') {
									tok -> charCount = 0;
									tok -> charValue = 0;
									state = BKTKStateStringChar;
									break;
								}
							}

							state = BKTKStateString;
							break;
						}
					}
					break;
				}
				case BKTKStateStringChar: {
					BKInt value;

					switch (charType) {
						case BKTKTypeEnd: {
							token = &tok -> token;
							BKTKTokenizerSetError (tok, "Premature end of string starting on line %u:%u",
								token -> offset.lineno, token -> offset.colno);
							goto error;
							break;
						}
						case BKTKTypeString: {
							BKTKTokenizerSetError (tok, "Premature end of char sequence on line %u:%u",
								offset.lineno, offset.colno);
							goto error;
							break;
						}
						default: {
							value = hexChars [c];

							if (value < 0) {
								BKTKTokenizerSetError (tok, "Invalid hex char \\x%02x on line %u:%u",
									c, offset.lineno, offset.colno);
								goto error;
								break;
							}

							tok -> charValue = (tok -> charValue << 4) | value;

							if (++ tok -> charCount == 2) {
								c = tok -> charValue;
								state = BKTKStateString;
							}

							break;
						}
					}

					break;
				}
				case BKTKStateCommentStart:
				case BKTKStateComment: {
					switch (charType) {
						case BKTKTypeLineBreak: {
							state = BKTKStateRoot;
							accept = 1;
							break;
						}
						case BKTKTypeEnd: {
							state = BKTKStateRoot;
							accept = 1;
							again = 1;
							break;
						}
						default: {
							state = BKTKStateComment;
							break;
						}
					}
					break;
				}
				case BKTKStateDataStart: {
					switch (charType) {
						case BKTKTypeString: {
							state = BKTKStateData;
							tok -> base64Len = 0;
							break;
						}
						default: {
							BKTKTokenizerSetError (tok, "Expected quote on line %u:%u",
								offset.lineno, offset.colno);
							goto error;
							break;
						}
					}
					break;
				}
				case BKTKStateData: {
					switch (charType) {
						case BKTKTypeString: {
							state = BKTKStateDataEnd;
							break;
						}
						case BKTKTypeEnd: {
							BKTKTokenizerSetError (tok, "Expected quote on line %u:%u",
								offset.lineno, offset.colno);
							goto error;
							break;
						}
						default: {
							break;
						}
					}
					break;
				}
				case BKTKStateSpace: {
					switch (charType) {
						case BKTKTypeSpace: {
							break;
						}
						case BKTKTypeEnd: {
							state = BKTKStateRoot;
							again = 1;
							break;
						}
						default: {
							state = BKTKStateRoot;
							again = 1;
							break;
						}
					}
					break;
				}
				default: {
					break;
				}
			}

			// token actions
			switch (state) {
				case BKTKStateArg:
				case BKTKStateString:
				case BKTKStateComment: {
					BKTKTokenizerBufferPutChar (tok, c);
					break;
				}
				case BKTKStateData: {
					BKTKTokenizerBufferPutBase64Char (tok, c);
					break;
				}
				case BKTKStateDataEnd: {
					BKTKTokenizerBufferEndBase64 (tok);
					state = BKTKStateRoot;
					accept = 1;
					break;
				}
				case BKTKStateRoot: {
					switch (type) {
						case BKTKTypeArgSep:
						case BKTKTypeCmdSep: {
							BKTKTokenizerBufferPutChar (tok, c);
							break;
						}
						default: {
							break;
						}
					}
					break;
				}
				case BKTKStateEnd: {
					type = BKTKTypeEnd;
					accept = 1;
					break;
				}
				case BKTKStateError: {
					goto error;
					break;
				}
				default: {
					break;
				}
			}

			if (accept) {
				token = &tok -> token;
				token -> dataLen = &tok -> buffer [tok -> bufferLen] - token -> data;
				BKTKTokenizerEndBuffer (tok);

				if ((res = putToken (&tok -> token, arg)) != 0) {
					BKTKTokenizerSetError (tok, "User error: %d", res);
					goto error;
				}
			}
		}
		while (again);
	}
	while (chars < end);

	tok -> state = state;
	tok -> offset = offset;

	return 0;

	allocationError: {
		BKTKTokenizerSetError (tok, "Allocation error");
		goto error;
	}

	error: {
		tok -> state = BKTKStateError;
		return res;
	}
}

/**
 * Splits input chars into smaller chunks for better buffer usage
 */
BKInt BKTKTokenizerPutChars (BKTKTokenizer * tok, uint8_t const * chars, BKUSize size, BKTKPutTokenFunc putToken, void * arg)
{
	int res;
	size_t chunkSize;
	size_t const maxSize = 1024;

	do {
		chunkSize = size > maxSize ? maxSize : size;

		if ((res = BKTKTokenizerPutCharsChunk (tok, chars, chunkSize, putToken, arg)) != 0) {
			break;
		}

		chars += chunkSize;
		size -= chunkSize;
	}
	while (size);

	return 0;
}

BKClass const BKTKTokenizerClass =
{
	.instanceSize = sizeof (BKTKTokenizer),
	.dispose      = (void *) BKTKTokenizerDispose,
};
