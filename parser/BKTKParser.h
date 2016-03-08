#ifndef _BK_TK_PARSER_H_
#define _BK_TK_PARSER_H_

#include "BKTKBase.h"
#include "BKTKTokenizer.h"

typedef enum BKTKParserState BKTKParserState;
typedef struct BKTKParser BKTKParser;
typedef struct BKTKParserNode BKTKParserNode;
typedef struct BKTKParserItem BKTKParserItem;

/**
 * Defines the parsers internal state
 */
enum BKTKParserState
{
	BKTKParserStateRoot = 0,
	BKTKParserStateCmd,
	BKTKParserStateArg,
	BKTKParserStateArgSep,
	BKTKParserStateCmdSep,
	BKTKParserStateGrpOpen,
	BKTKParserStateGrpClose,
	BKTKParserStateComment,
	BKTKParserStateEnd,
	BKTKParserStateError,
};

/**
 * Flags used internally
 */
enum BKTKParserFlag {
	BKTKParserFlagIsGroup     = 1 << 0,
	BKTKParserFlagDataIsBlock = 1 << 1,
};

/**
 * Defines a node
 *
 * Nodes are linked like a tree: arrows pointing to the right are `nextNode`s,
 * arrows pointing downwards are `subNode`s.
 *
 * root
 *  |
 *  v
 * node -> node -> ...
 *          |
 *          v
 *         node -> ...
 *          |
 *          v
 *          .
 *          .
 */
struct BKTKParserNode
{
	BKUInt           flags;
	BKString         name;
	BKString       * args;
	BKUSize          argCount;
	BKTKType       * argTypes;
	BKTKOffset       offset;
	BKTKOffset     * argOffsets;
	BKTKType         type;
	BKTKParserNode * nextNode;
	BKTKParserNode * subNode;
};

/**
 * Defines an internal parser stack item
 */
struct BKTKParserItem
{
	BKTKParserNode * node;
	BKUSize          itemCount;
};

struct BKTKParser
{
	BKObject         object;
	BKUSize          stackSize;
	BKUSize          stackCapacity;
	BKUSize          itemCount;
	BKTKParserItem * stack;
	BKUSize          bufferLen;
	BKUSize          bufferCap;
	uint8_t        * buffer;
	BKUSize          argCount;
	BKUSize          argLengthsCapacity;
	BKUSize        * argCursors;
	BKUSize        * argLengths;
	BKTKType       * argTypes;
	BKTKOffset     * argOffsets;
	BKTKParserNode   rootNode;
	BKTKParserNode * freeNodes;
	BKTKParserState  state;
	BKBlockPool      blockPool;
	BKBlockPool      argsPool;
	BKString         escapedName;
};

/**
 * Initialize parser
 */
extern BKInt BKTKParserInit (BKTKParser * parser);

/**
 * Reset parser for parsing new tokens
 *
 * Keeps the allocated buffers
 */
extern void BKTKParserReset (BKTKParser * parser);

/**
 * Put tokens
 *
 * Returns a value != 0 if an error ocurred.
 */
extern BKInt BKTKParserPutTokens (BKTKParser * parser, BKTKToken const * tokens, BKUSize count);

/**
 * Get node tree
 *
 * Returns the first node
 */
extern BKTKParserNode * BKTKParserGetNodeTree (BKTKParser * parser);

/**
 * Check if parser has encountered an error
 */
BK_INLINE BKInt BKTKParserHasError (BKTKParser const * parser);

/**
 * Check if parser is finished
 *
 * Returns 1 if parser is finished or an error occured
 */
BK_INLINE BKInt BKTKParserIsFinished (BKTKParser const * parser);


// --- Inline implementations

BK_INLINE BKInt BKTKParserHasError (BKTKParser const * parser)
{
	return parser -> state == BKTKParserStateError;
}

BK_INLINE BKInt BKTKParserIsFinished (BKTKParser const * parser)
{
	return parser -> state >= BKTKParserStateEnd;
}

#endif /* ! _BK_TK_PARSER_H_ */
