#ifndef _BK_TK_COMPILER_H_
#define _BK_TK_COMPILER_H_

#include "BKTKBase.h"
#include "BKInstrument.h"
#include "BKTKInterpreter.h"
#include "BKTKParser.h"

/**
 * Globally used flags
 */
enum BKTKFlag
{
	BKTKFlagUsed      = 1 << 0,
	BKTKFlagAutoIndex = 1 << 1,
};

struct BKTKCompiler
{
	BKObject    object;
	BKHashTable instruments;
	BKHashTable waveforms;
	BKHashTable samples;
	BKArray     tracks;
	BKString    auxString;
	BKString    error;
	BKInt       lineno;
};

/**
 * Initialize compiler object
 */
extern BKInt BKTKCompilerInit (BKTKCompiler * compiler);

/**
 * Parse tree from BKParser
 */
extern BKInt BKTKCompilerCompile (BKTKCompiler * compiler, BKTKParserNode const * tree);

/**
 * Reset compiler to compile another node
 */
extern BKInt BKTKCompilerReset (BKTKCompiler * compiler);

#endif /* ! _BK_TK_COMPILER_H_ */
