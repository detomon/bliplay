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

#include "BKTKWriter.h"

#define INIT_BUFFER_SIZE 4096
#define INIT_INDENT_CAPACITY 16

typedef struct BKTKWriter BKTKWriter;

struct BKTKWriter
{
	BKObject object;
	BKUSize bufferLen;
	BKUSize bufferCap;
	uint8_t * buffer;
	uint8_t * indentBuffer;
	BKUSize indentSize;
	BKUSize indentCap;
	BKTKWriterWriteFunc write;
	void * userInfo;
	BKUSize level;
};

enum BKTKWriterChar
{
	BKTKWriterCharString    = 1 << 0,
	BKTKWriterCharEscape    = 1 << 1,
	BKTKWriterCharLineBreak = 1 << 2,
};

extern BKClass const BKTKWriterClass;

static uint8_t const charTypes [256] =
{
	[(uint8_t) ':']    = BKTKWriterCharString,
	[(uint8_t) ';']    = BKTKWriterCharString,
	[(uint8_t) '[']    = BKTKWriterCharString,
	[(uint8_t) ']']    = BKTKWriterCharString,
	[(uint8_t) '"']    = BKTKWriterCharEscape,
	[(uint8_t) '!']    = BKTKWriterCharString,
	[(uint8_t) '%']    = BKTKWriterCharString,
	[(uint8_t) ' ']    = BKTKWriterCharString,
	[(uint8_t) '\xa0'] = BKTKWriterCharString, // no-break space
	[(uint8_t) '\\']   = BKTKWriterCharEscape,
	[(uint8_t) '\a']   = BKTKWriterCharString,
	[(uint8_t) '\b']   = BKTKWriterCharString,
	[(uint8_t) '\f']   = BKTKWriterCharString,
	[(uint8_t) '\n']   = BKTKWriterCharLineBreak,
	[(uint8_t) '\r']   = BKTKWriterCharLineBreak,
	[(uint8_t) '\t']   = BKTKWriterCharString,
	[(uint8_t) '\v']   = BKTKWriterCharString,
};

static uint8_t const base64Chars [64] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
};

static void BKTKWriterIndentBufferExtend (uint8_t * indentBuffer, BKUSize indentSize, BKUSize capacity, BKUSize newCapacity)
{
	for (BKUSize i = capacity; i < newCapacity; i ++) {
		memcpy (&indentBuffer [indentSize * i], &indentBuffer [0], indentSize);
	}
}

static BKInt BKTKWriterInit (BKTKWriter * writer, uint8_t const * indent, BKTKWriterWriteFunc write, void * userInfo)
{
	BKInt res;

	if ((res = BKObjectInit (writer, &BKTKWriterClass, sizeof (*writer))) != 0) {
		return res;
	}

	writer -> bufferCap = INIT_BUFFER_SIZE;
	writer -> buffer    = malloc (writer -> bufferCap * sizeof (*writer -> buffer));

	if (!writer -> buffer) {
		BKDispose (writer);
		return -1;
	}

	writer -> indentCap    = INIT_INDENT_CAPACITY;
	writer -> indentSize   = strlen ((void *) indent);
	writer -> indentBuffer = malloc (writer -> indentSize * writer -> indentCap * sizeof (*writer -> indentBuffer));

	if (!writer -> indentBuffer) {
		BKDispose (writer);
		return -1;
	}

	writer -> write    = write;
	writer -> userInfo = userInfo;
	writer -> level    = 0;

	memcpy (writer -> indentBuffer, indent, writer -> indentSize);
	BKTKWriterIndentBufferExtend (writer -> indentBuffer, writer -> indentSize, 1, writer -> indentCap);

	return 0;
}

static void BKTKWriterDispose (BKTKWriter * writer)
{
	free (writer -> buffer);
	free (writer -> indentBuffer);
}

static BKInt BKTKWriterFlushBuffer (BKTKWriter * writer)
{
	int res;

	res = writer -> write (writer -> userInfo, writer -> buffer, writer -> bufferLen);
	writer -> bufferLen = 0;

	return res;
}

static BKInt BKTKWriterCheckIfEscapeNeeded (const BKString * data)
{
	BKInt c;

	for (BKUSize i = 0; i < data->len; i ++) {
		c = data->str [i];

		if (charTypes [c]) {
			return 1;
		}
	}

	return 0;
}

static BKInt BKTKWriterWriteChar (BKTKWriter * writer, uint8_t c)
{
	BKInt res;

	if (writer -> bufferLen + 1 > writer -> bufferCap) {
		if ((res = BKTKWriterFlushBuffer (writer)) != 0) {
			return res;
		}
	}

	writer -> buffer [writer -> bufferLen] = c;
	writer -> bufferLen ++;

	return 0;

}

static BKInt BKTKWriterWriteChars (BKTKWriter * writer, const uint8_t * data, BKUSize size)
{
	BKInt res;

	if (writer -> bufferLen + size > writer -> bufferCap) {
		if ((res = BKTKWriterFlushBuffer (writer)) != 0) {
			return res;
		}
	}

	if (size < writer -> bufferCap) {
		memcpy (&writer -> buffer [writer -> bufferLen], data, size);
		writer -> bufferLen += size;
	}
	else {
		if ((res = writer -> write (writer -> userInfo, data, size)) != 0) {
			return res;
		}
	}

	return 0;
}

static BKInt BKTKWriterWrite (BKTKWriter * writer, const BKString * data)
{
	return BKTKWriterWriteChars (writer, data -> str, data -> len);
}

static BKInt BKTKWriterWriteString (BKTKWriter * writer, const BKString * data)
{
	BKInt c;
	BKInt res;
	BKUSize maxLen;
	BKUSize bufferLen;
	uint8_t * str = data -> str;
	BKUSize size = data -> len;

	if ((res = BKTKWriterWriteChar (writer, '\"')) != 0) {
		return res;
	}

	bufferLen = writer -> bufferLen;

	while (size) {
		// assume every character has to be escaped
		maxLen = BKMin ((writer -> bufferCap - writer -> bufferLen) / 2, size);

		for (BKUSize i = 0; i < maxLen; i ++) {
			c = str [i];

			if (charTypes [c] & BKTKWriterCharEscape) {
				writer -> buffer [bufferLen ++] = '\\';
			}

			writer -> buffer [bufferLen ++] = c;
		}

		writer -> bufferLen = bufferLen;

		if ((res = BKTKWriterFlushBuffer (writer)) != 0) {
			return res;
		}

		str += maxLen;
		size -= maxLen;
		bufferLen = 0;
	}

	writer -> bufferLen = bufferLen;

	if ((res = BKTKWriterWriteChar (writer, '\"')) != 0) {
		return res;
	}

	return 0;
}

static BKInt BKTKWriterWriteBase64 (BKTKWriter * writer, const BKString * data)
{
	BKInt res;
	BKUSize maxLen;
	BKUSize bufferLen;
	uint32_t value;
	uint8_t * str = data -> str;
	BKUSize size = data -> len;

	if ((res = BKTKWriterWriteChars (writer, (uint8_t *) "!\"", 2)) != 0) {
		return res;
	}

	bufferLen = writer -> bufferLen;

	while (size >= 3) {
		// leave space for base64 encoded string (33% larger)
		maxLen = BKMin ((writer -> bufferCap - writer -> bufferLen) / 2, size);

		for (; maxLen >= 3; maxLen -= 3) {
			value  = (uint32_t) str [0] << 16;
			value |= (uint32_t) str [1] << 8;
			value |= (uint32_t) str [2] << 0;

			writer -> buffer [bufferLen ++] = base64Chars [(value >> 18) & 0x3F];
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 12) & 0x3F];
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 6) & 0x3F];
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 0) & 0x3F];

			str += 3;
			size -= 3;
		}

		writer -> bufferLen = bufferLen;

		if ((res = BKTKWriterFlushBuffer (writer)) != 0) {
			return res;
		}

		bufferLen = writer -> bufferLen;
	}

	switch (size) {
		case 1: {
			value = str [0] << 16;
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 18) & 0x3F];
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 12) & 0x3F];
			writer -> buffer [bufferLen ++] = '=';
			writer -> buffer [bufferLen ++] = '=';
			break;
		}
		case 2: {
			value  = str [0] << 16;
			value |= str [1] << 8;
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 18) & 0x3F];
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 12) & 0x3F];
			writer -> buffer [bufferLen ++] = base64Chars [(value >> 6) & 0x3F];
			writer -> buffer [bufferLen ++] = '=';
			break;
		}
	}

	writer -> bufferLen = bufferLen;

	if ((res = BKTKWriterWriteChar (writer, '\"')) != 0) {
		return res;
	}

	return 0;
}

static BKInt BKTKWriterWriteComment (BKTKWriter * writer, const BKString * data)
{
	BKInt c;
	BKInt res;
	BKUSize maxLen;
	BKUSize bufferLen;
	uint8_t * str = data -> str;
	BKUSize size = data -> len;

	if ((res = BKTKWriterWriteChar (writer, '%')) != 0) {
		return res;
	}

	bufferLen = writer -> bufferLen;

	while (size) {
		maxLen = BKMin (writer -> bufferCap - writer -> bufferLen, size);

		for (BKUSize i = 0; i < maxLen; i ++) {
			c = str [i];

			if (charTypes [c] & BKTKWriterCharLineBreak) {
				c = ' ';
			}

			writer -> buffer [bufferLen ++] = c;
		}

		writer -> bufferLen = bufferLen;

		if ((res = BKTKWriterFlushBuffer (writer)) != 0) {
			return res;
		}

		str += maxLen;
		size -= maxLen;
		bufferLen = 0;
	}

	writer -> bufferLen = bufferLen;

	return 0;
}

static BKInt BKTKWriterWriteEscaped (BKTKWriter * writer, const BKString * data, BKTKType type)
{
	if (type == BKTKTypeArg) {
		if (BKTKWriterCheckIfEscapeNeeded (data)) {
			type = BKTKTypeString;
		}
	}

	switch (type) {
		case BKTKTypeArg: {
			return BKTKWriterWrite (writer, data);
			break;
		}
		case BKTKTypeData: {
			return BKTKWriterWriteBase64 (writer, data);
			break;
		}
		case BKTKTypeComment: {
			return BKTKWriterWriteComment (writer, data);
			break;
		}
		case BKTKTypeString:
		default: {
			return BKTKWriterWriteString (writer, data);
			break;
		}
	}

	return 0;
}

static BKInt BKTKWriterWriteIndention (BKTKWriter * writer)
{
	BKInt res;
	BKUSize level = writer -> level;
	BKUSize maxLevel;

	while (level > 0) {
		maxLevel = BKMin (level, writer -> indentCap);
		level -= maxLevel;

		if ((res = BKTKWriterWriteChars (writer, writer -> indentBuffer, writer -> indentSize * maxLevel)) != 0) {
			return res;
		}
	}

	return 0;
}

static BKInt BKTKWriterWriteCommand (BKTKWriter * writer, BKTKParserNode const * node)
{
	BKInt res;

	if ((res = BKTKWriterWriteEscaped (writer, &node -> name, node -> type)) != 0) {
		return res;
	}

	for (BKUSize i = 0; i < node -> argCount; i ++) {
		if ((res = BKTKWriterWriteChar (writer, ':')) != 0) {
			return res;
		}

		if ((res = BKTKWriterWriteEscaped (writer, &node -> args [i], node -> argTypes [i])) != 0) {
			return res;
		}
	}

	if ((res = BKTKWriterWriteChar (writer, '\n')) != 0) {
		return res;
	}

	return res;
}

static BKInt BKTKWriterWriteNodeBuffer (BKTKWriter * writer, BKTKParserNode const * node, BKInt firstIndent)
{
	BKInt res;

	if (firstIndent) {
		if ((res = BKTKWriterWriteIndention (writer)) != 0) {
			return res;
		}
	}

	if (node -> flags & BKTKParserFlagIsGroup) {
		if ((res = BKTKWriterWriteChar (writer, '[')) != 0) {
			return res;
		}

		writer -> level ++;

		if ((res = BKTKWriterWriteCommand (writer, node)) != 0) {
			return res;
		}

		if (node -> subNode) {
			if ((res = BKTKWriterWriteNodeBuffer(writer, node -> subNode, firstIndent)) != 0) {
				return res;
			}
		}

		writer -> level --;

		if ((res = BKTKWriterWriteIndention (writer)) != 0) {
			return res;
		}

		if ((res = BKTKWriterWriteChars (writer, (uint8_t *) "]\n", 2)) != 0) {
			return res;
		}
	}
	else {
		if ((res = BKTKWriterWriteCommand (writer, node)) != 0) {
			return res;
		}
	}

	if (node -> nextNode) {
		if ((res = BKTKWriterWriteNodeBuffer (writer, node -> nextNode, 1)) != 0) {
			return res;
		}
	}

	return 0;
}

BKInt BKTKWriterWriteNode (BKTKParserNode const * node, BKTKWriterWriteFunc write, void * userInfo, uint8_t const * indent)
{
	int res;
	BKTKWriter writer;

	if ((res = BKTKWriterInit (&writer, indent, write, userInfo)) != 0) {
		return res;
	}

	res = BKTKWriterWriteNodeBuffer (&writer, node, 1);

	if (res == 0) {
		res = BKTKWriterFlushBuffer (&writer);
	}

	BKDispose (&writer);

	return res;
}

BKClass const BKTKWriterClass =
{
	.instanceSize = sizeof (BKTKWriter),
	.dispose      = (void *) BKTKWriterDispose,
};
