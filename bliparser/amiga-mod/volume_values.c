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
#include <stdio.h>
#include "BKFileAmigaMod.h"

/**
 * Volume calculation (see mod-spec.txt:352)
 * volume: 0 - 64; muted if 0 (division by 0)
 *
 * 10 ^ (0.1 * (20 * log10(volume / 64))) * BK_MAX_VOLUME
 */

static BKFrame values [BK_AMIGA_NUM_VOLUME_STEPS + 1];

int main (int argc, char const * argv [])
{
	double value;

	values [0] = 0;

	for (int i = 1; i <= BK_AMIGA_NUM_VOLUME_STEPS; i ++) {
		value = pow (10, log ((double) i / 64.0) * 20.0 * 0.1);
		values [i] = value * BK_MAX_VOLUME;
	}

	printf (
		"/**\n"
		" * Volume values used for lookup\n"
		" * Generated with `%s`\n"
		" */\n"
		"static BKFrame const volumeValues [%d + 1] =\n{\n\t",
		__FILE__, BK_AMIGA_NUM_VOLUME_STEPS
	);

	for (int i = 0; i <= BK_AMIGA_NUM_VOLUME_STEPS; i ++) {
		if (i && i % 8 == 0) {
			printf ("\n\t");
		}

		printf ("%5d, ", values [i]);
	}

	printf ("\n};\n\n");

	return 0;
}
