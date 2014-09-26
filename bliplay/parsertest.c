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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "BKSTParser.h"

 BKSTTokenizer tokenizer;

int main (int argc, char * argv [])
{
	BKSTTokenType token;
	BKSTParser parser;
	BKSTCmd cmd;

	FILE * file = fopen ("/Users/simon/Downloads/test-new-format.blip", "r");

	BKSTParserInitWithFile (& parser, file);

	while ((token = BKSTParserNextCommand (& parser, & cmd))) {
		printf ("%d %s %ld %d:%d\n", token, cmd.name, cmd.numArgs, cmd.lineno, cmd.colno);

		if (cmd.numArgs > 0) {
			for (BKInt i = 0; i < cmd.numArgs; i ++) {
				printf ("  '%s' (%ld)\n", cmd.args[i].arg, cmd.args[i].size);
			}
		}
	}

	BKSTParserDispose (& parser);

	fclose (file);

    return 0;
}
