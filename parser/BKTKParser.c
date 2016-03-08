#include "BKTKParser.h"

#define STACK_INIT_SIZE 32
#define BUFFER_INIT_SIZE 4096
#define ARGS_LEN_INIT_SIZE 64
#define BLOCK_POOL_SEGMENT_CAPACITY 128
#define ARGS_POOL_SEGMENT_SIZE 64

#define PTR_MASK (sizeof (void *) - 1)
#define SIZE_ROUND_UP(size) (((size) + PTR_MASK) & ~PTR_MASK)

extern BKClass const BKTKParserClass;

BKInt BKTKParserInit (BKTKParser * parser)
{
	BKInt res;

	if ((res = BKObjectInit (parser, &BKTKParserClass, sizeof (*parser))) != 0) {
		return res;
	}

	parser -> stackCapacity = STACK_INIT_SIZE;
	parser -> stack = malloc (parser -> stackCapacity * sizeof (*parser -> stack));

	if (!parser -> stack) {
		goto error;
	}

	parser -> bufferCap = BUFFER_INIT_SIZE;
	parser -> buffer = malloc (parser -> bufferCap * sizeof (*parser -> buffer));

	if (!parser -> buffer) {
		goto error;
	}

	parser -> argLengthsCapacity = ARGS_LEN_INIT_SIZE;
	parser -> argCursors = malloc (parser -> argLengthsCapacity * sizeof (*parser -> argCursors));

	if (!parser -> argCursors) {
		goto error;
	}

	parser -> argLengths = malloc (parser -> argLengthsCapacity * sizeof (*parser -> argLengths));

	if (!parser -> argLengths) {
		goto error;
	}

	parser -> argTypes = malloc (parser -> argLengthsCapacity * sizeof (*parser -> argTypes));

	if (!parser -> argTypes) {
		goto error;
	}

	parser -> argOffsets = malloc (parser -> argLengthsCapacity * sizeof (*parser -> argOffsets));

	if (!parser -> argOffsets) {
		goto error;
	}

	if ((res = BKBlockPoolInit (&parser -> blockPool, sizeof (BKTKParserNode), BLOCK_POOL_SEGMENT_CAPACITY)) != 0) {
		goto error;
	}

	if ((res = BKBlockPoolInit (&parser -> argsPool, ARGS_POOL_SEGMENT_SIZE, BLOCK_POOL_SEGMENT_CAPACITY)) != 0) {
		goto error;
	}

	BKTKParserReset (parser);
	parser -> escapedName = BK_STRING_INIT;

	return 0;

	error: {
		BKDispose (parser);

		return -1;
	}
}

/**
 * Ensure that at least one node can be allocated
 */
static BKInt BKTKParserNodeEnsureAlloc (BKTKParser * parser)
{
	return BKBlockPoolEnsureBlock(&parser -> blockPool);
}

static BKTKParserNode * BKTKParserNodeAlloc (BKTKParser * parser)
{
	return BKBlockPoolAlloc (&parser -> blockPool);
}

static void BKTKParserNodeFree (BKTKParser * parser, BKTKParserNode * node)
{
	BKBlockPoolFree (&parser -> blockPool, node);
}

static void BKTKParserNodeFreeArgs (BKTKParserNode * node)
{
	if (!(node -> flags & BKTKParserFlagDataIsBlock)) {
		BKStringDispose (&node -> name);
	}
}

/**
 * Copies arguments from one node to other node
 *
 * Free existing arguments if present
 */
static void BKTKParserNodeTransferArgs (BKTKParserNode * target, BKTKParserNode * source)
{
	BKTKParserNodeFreeArgs (target);

	// do not transfer group flag
	target -> flags = (target -> flags & BKTKParserFlagIsGroup) | (source -> flags & ~BKTKParserFlagIsGroup);
	target -> name = source -> name;
	target -> argCount = source -> argCount;
	target -> args = source -> args;
	target -> argTypes = source -> argTypes;
	target -> offset = source -> offset;
	target -> argOffsets = source -> argOffsets;
	target -> type = source -> type;

	source -> name = BK_STRING_INIT;
	source -> argCount = 0;
	source -> args = NULL;
	source -> argTypes = NULL;
	source -> offset = source -> offset;
	source -> argOffsets = NULL;
}

static BKInt BKTKParserBufferEnsureSpace (BKTKParser * parser, BKUSize addSize)
{
	uint8_t * newBuffer;
	BKUSize newCapacity;

	addSize += 256;

	if (parser -> bufferLen + addSize >= parser -> bufferCap) {
		newCapacity = BKNextPow2 (parser -> bufferLen + addSize);
		newBuffer = realloc (parser -> buffer, newCapacity);

		if (!newBuffer) {
			return -1;
		}

		parser -> buffer = newBuffer;
		parser -> bufferCap = newCapacity;
	}

	return 0;
}

static BKInt BKTKParserStackEnsureSpace (BKTKParser * parser)
{
	BKTKParserItem * newBuffer;
	BKUSize newCapacity;
	BKUSize const minAddCap = 16;

	if (parser -> stackSize + minAddCap >= parser -> stackCapacity) {
		newCapacity = BKNextPow2 (parser -> stackSize + minAddCap);
		newBuffer = realloc (parser -> stack, newCapacity * sizeof (*newBuffer));

		if (!newBuffer) {
			return -1;
		}

		parser -> stack = newBuffer;
		parser -> stackCapacity = newCapacity;
	}

	return 0;
}

static BKInt BKTKParserArgsEnsureSpace (BKTKParser * parser)
{
	BKUSize * newLengths;
	BKUSize * newArgCursors;
	BKTKType * newArgTypes;
	BKUSize newCapacity;
	BKUSize const minAddCap = 16;

	if (parser -> argCount + minAddCap >= parser -> argLengthsCapacity) {
		newCapacity = BKNextPow2 (parser -> argCount + minAddCap);
		newLengths = realloc (parser -> argLengths, newCapacity * sizeof (*newLengths));

		if (!newLengths) {
			return -1;
		}

		parser -> argLengths = newLengths;
		parser -> argLengthsCapacity = newCapacity;

		newArgCursors = realloc (parser -> argCursors, newCapacity * sizeof (*newArgCursors));

		if (!newArgCursors) {
			return -1;
		}

		parser -> argCursors = newArgCursors;

		newArgTypes = realloc (parser -> argTypes, newCapacity * sizeof (*newArgTypes));

		if (!newArgTypes) {
			return -1;
		}

		parser -> argTypes = newArgTypes;
	}

	return 0;
}

/**
 * Push new node to stack
 */
static BKTKParserItem * BKTKParserStackPush (BKTKParser * parser)
{
	BKTKParserNode * node;
	BKTKParserItem * item;

	node = BKTKParserNodeAlloc (parser);

	if (!node) {
		return NULL;
	}

	item = &parser -> stack [parser -> stackSize ++];
	memset (item, 0, sizeof (*item));
	item -> node = node;

	return item;
}

/**
 * Push root node to stack
 */
static BKTKParserItem * BKTKParserStackPushRoot (BKTKParser * parser)
{
	BKTKParserNode * node;
	BKTKParserItem * item;

	node = &parser -> rootNode;
	memset (node, 0, sizeof (*node));

	item = &parser -> stack [parser -> stackSize ++];
	memset (item, 0, sizeof (*item));
	item -> node = node;

	return item;
}

/**
 * Pop node from stack
 */
static BKTKParserItem * BKTKParserStackPop (BKTKParser * parser)
{
	return &parser -> stack [-- parser -> stackSize];
}

/**
 * Get last item on stack
 */
static BKTKParserItem * BKTKParserStackLast (BKTKParser * parser)
{
	return &parser -> stack [parser -> stackSize - 1];
}

/**
 * Link the 2 topmost stack item
 *
 * If no previous node is present in the current hierarchy, the last node is
 * linked as subnode of the first one, otherwise it is linked as next node.
 */
static void BKTKParserStackLink (BKTKParser * parser)
{
	BKTKParserNode * a, * b;

	a = parser -> stack [parser -> stackSize - 1].node;
	b = parser -> stack [parser -> stackSize - 2].node;

	if (parser -> itemCount) {
		b -> nextNode = a;
	}
	else {
		b -> subNode = a;
	}
}

/**
 * Reduce the 2 topmost stack items
 *
 * This operation corresponds to popping the 2 topmost items, link them and
 * push the last one back to the stack
 */
static void BKTKParserStackReduce (BKTKParser * parser)
{
	BKTKParserStackLink (parser);
	parser -> stack [parser -> stackSize - 2] = parser -> stack [parser -> stackSize - 1];
	BKTKParserStackPop (parser);
}

static void BKTKParserSetError (BKTKParser * parser, char const * msg, ...)
{
	va_list args;

	va_start (args, msg);
	vsnprintf ((void *) parser -> buffer, parser -> bufferCap, msg, args);
	va_end (args);

	parser -> bufferLen = strnlen ((char *) parser -> buffer, parser -> bufferCap);
}

static void BKTKParserFreeNodes (BKTKParserNode * node)
{
	BKTKParserNode * nextNode;

	for (; node; node = nextNode) {
		nextNode = node -> nextNode;

		if (node -> subNode) {
			BKTKParserFreeNodes (node -> subNode);
		}

		// `node -> name` also contains `node -> args` and `node -> argLengths`
		BKTKParserNodeFreeArgs (node);

		// is in block pool
		//free (node);
	}
}

void BKTKParserReset (BKTKParser * parser)
{
	BKTKParserItem * item;

	if (parser -> stackSize) {
		item = &parser -> stack [0];
		BKTKParserFreeNodes (item -> node);
	}

	parser -> state     = BKTKParserStateRoot;
	parser -> stackSize = 0;
	parser -> bufferLen = 0;
	parser -> itemCount = 0;
	parser -> argCount  = 0;

	item = BKTKParserStackPushRoot (parser);
	item -> node -> type = BKTKTypeGrpOpen;
}

static void BKTKParserDispose (BKTKParser * parser)
{
	BKTKParserItem * item;
	BKTKParserNode * node;

	if (parser -> stackSize) {
		item = &parser -> stack [0];
		node = item -> node;

		BKTKParserFreeNodes (node -> nextNode);
		BKTKParserFreeNodes (node -> subNode);
	}

	if (parser -> stack) {
		free (parser -> stack);
	}

	if (parser -> buffer) {
		free (parser -> buffer);
	}

	if (parser -> argLengths) {
		free (parser -> argLengths);
	}

	if (parser -> argCursors) {
		free (parser -> argCursors);
	}

	if (parser -> argTypes) {
		free (parser -> argTypes);
	}

	BKBlockPoolDispose (&parser -> blockPool);
	BKBlockPoolDispose (&parser -> argsPool);
	BKStringDispose (&parser -> escapedName);
}

static BKTKType BKTKParserGetSimpleType (BKTKType type)
{
	switch (type) {
		case BKTKTypeArg:
		case BKTKTypeString:
		case BKTKTypeData: {
			type = BKTKTypeArg;
			break;
		}
		case BKTKTypeCmdSep:
		case BKTKTypeLineBreak:
		case BKTKTypeGrpOpen:
		case BKTKTypeGrpClose:
		case BKTKTypeArgSep:
		case BKTKTypeComment: {
			break;
		}
		case BKTKTypeNone:
		case BKTKTypeOther: {
			type = BKTKTypeOther;
			break;
		}
		default: {
			type = BKTKTypeEnd;
			break;
		}
	}

	return type;
}

static void BKTKParserEndBuffer (BKTKParser * parser)
{
	// terminate buffer
	parser -> buffer [parser -> bufferLen ++] = '\0';

	// align next buffer segment to pointer size
	while (parser -> bufferLen & PTR_MASK) {
		parser -> buffer [parser -> bufferLen ++] = '\0';
	}
}

/**
 * Push argument and append data to buffer
 */
static void BKTKParserPushArg (BKTKParser * parser, BKTKToken const * token)
{
	parser -> argCursors [parser -> argCount] = parser -> bufferLen;
	parser -> argLengths [parser -> argCount] = token -> dataLen;
	parser -> argTypes [parser -> argCount]   = token -> type;
	parser -> argOffsets [parser -> argCount] = token -> offset;
	parser -> argCount ++;

	memcpy (&parser -> buffer [parser -> bufferLen], token -> data, token -> dataLen);
	parser -> bufferLen += token -> dataLen;

	BKTKParserEndBuffer (parser);
}

static void BKTKParserBeginCmd (BKTKParser * parser, BKTKToken const * token)
{
	BKTKParserItem * item;

	// item ensured by `BKTKParserStackEnsureSpace` and `BKTKParserNodeEnsureAlloc`
	item = BKTKParserStackPush (parser);

	if (parser -> itemCount) {
		BKTKParserStackReduce (parser);
	}
	else {
		BKTKParserStackLink (parser);
	}

	parser -> itemCount ++;
	item -> node -> offset = token -> offset;

	BKTKParserPushArg (parser, token);
}

/**
 * Pack buffered arguments of current node
 *
 * Argument informations are allocated as single block and packed into node name
 */
static BKInt BKTKParserEndCommand (BKTKParser * parser)
{
	uint8_t * buffer;
	BKString * args;
	BKTKType * argTypes;
	BKTKOffset * argOffsets;
	BKUSize bufferSize, argsSize, argTypesSize, argOffsetsSize;
	BKTKParserNode * node;
	BKUSize size;

	if (!parser -> argCount) {
		return 0;
	}

	node = BKTKParserStackLast (parser) -> node;

	if (parser -> argCount) {
		bufferSize     = SIZE_ROUND_UP (parser -> bufferLen);
		argsSize       = (parser -> argCount - 1) * sizeof (BKString);
		argTypesSize   = (parser -> argCount - 1) * sizeof (*parser -> argTypes);
		argOffsetsSize = (parser -> argCount - 1) * sizeof (*parser -> argOffsets);

		size = bufferSize + argsSize + argTypesSize + argOffsetsSize;

		// most commands require less than ARGS_POOL_SEGMENT_SIZE bytes
		if (size <= ARGS_POOL_SEGMENT_SIZE) {
			buffer = BKBlockPoolAlloc (&parser -> argsPool);
			node -> flags |= BKTKParserFlagDataIsBlock;
		}
		else {
			buffer = malloc (size);
		}

		if (!buffer) {
			return -1;
		}

		args       = (void *) &buffer [bufferSize];
		argTypes   = (void *) &args [parser -> argCount - 1];
		argOffsets = (void *) &argTypes [parser -> argCount - 1];

		memcpy (buffer,     parser -> buffer,          bufferSize);
		memcpy (argTypes,   &parser -> argTypes [1],   argTypesSize);
		memcpy (argOffsets, &parser -> argOffsets [1], argOffsetsSize);

		for (BKUSize i = 1; i < parser -> argCount; i ++) {
			args [i - 1] = (BKString) {
				.str = &buffer [parser -> argCursors [i]],
				.len = parser -> argLengths [i],
				.cap = parser -> argLengths [i],
			};
		}

		if (parser -> argCount > 1) {
			node -> args       = args;
			node -> argTypes   = argTypes;
			node -> argOffsets = argOffsets;
		}

		node -> name = (BKString) {
			.str = buffer,
			.len = parser -> argLengths [0],
			.cap = parser -> argLengths [0],
		};

		node -> type   = parser -> argTypes [0];
		node -> offset = parser -> argOffsets [0];
	}

	node -> argCount = parser -> argCount - 1;

	parser -> argCount = 0;
	parser -> bufferLen = 0;
	parser -> buffer [0] = '\0';

	return 0;
}

static void BKTKParserOpenGroup (BKTKParser * parser, BKTKToken const * token)
{
	BKTKParserItem * item;

	// item ensured by `BKTKParserStackEnsureSpace` and `BKTKParserNodeEnsureAlloc`
	item = BKTKParserStackPush (parser);

	if (parser -> itemCount) {
		BKTKParserStackReduce (parser);
	}
	else {
		BKTKParserStackLink (parser);
	}

	parser -> itemCount ++;

	item = BKTKParserStackLast (parser);
	item -> itemCount = parser -> itemCount;

	item -> node -> flags  |= BKTKParserFlagIsGroup;
	item -> node -> type    = BKTKTypeGrpOpen;
	item -> node -> offset  = token -> offset;

	parser -> itemCount = 0;
}

static BKInt BKTKParserCloseGroup (BKTKParser * parser)
{
	BKInt res = 0;
	BKTKParserItem * item;

	if ((res = BKTKParserEndCommand (parser)) != 0) {
		return res;
	}

	if (parser -> itemCount) {
		BKTKParserStackPop (parser);
	}

	item = BKTKParserStackLast (parser);

	// move data from first child node to group node and free first child node
	if (item -> node -> subNode) {
		BKTKParserNode * subNode;

		BKTKParserNodeTransferArgs (item -> node, item -> node -> subNode);
		subNode = item -> node -> subNode;
		item -> node -> subNode = subNode -> nextNode;
		BKTKParserNodeFree (parser, subNode);
	}

	parser -> itemCount = item -> itemCount;

	return res;
}

static BKInt BKTKParserPutToken (BKTKParser * parser, BKTKToken const * token)
{
	BKInt res = 0;
	BKTKParserState state;
	BKTKType type = BKTKParserGetSimpleType (token -> type);

	state = parser -> state;

	if (state >= BKTKParserStateEnd) {
		return 0;
	}

	if (token == BKTKTypeOther) {
		return 0;
	}

	if ((res = BKTKParserBufferEnsureSpace (parser, token -> dataLen)) != 0) {
		goto allocationError;
	}

	if ((res = BKTKParserStackEnsureSpace (parser)) != 0) {
		goto allocationError;
	}

	if ((res = BKTKParserArgsEnsureSpace (parser)) != 0) {
		goto allocationError;
	}

	if ((res = BKTKParserNodeEnsureAlloc (parser)) != 0) {
		goto allocationError;
	}

	// parser states
	switch (state) {
		case BKTKParserStateRoot: {
			switch (type) {
				case BKTKTypeArg: {
					state = BKTKParserStateCmd;
					break;
				}
				case BKTKTypeCmdSep: {
					state = BKTKParserStateRoot;
					break;
				}
				case BKTKTypeGrpOpen: {
					state = BKTKParserStateGrpOpen;
					break;
				}
				case BKTKTypeGrpClose: {
					state = BKTKParserStateGrpClose;
					break;
				}
				case BKTKTypeArgSep: {
					BKTKParserSetError (parser, "Argument separator does not follow any argument on line %u:%u",
						token -> offset.lineno, token -> offset.colno);
					goto error;
					break;
				}
				case BKTKTypeEnd: {
					state = BKTKParserStateEnd;
					break;
				}
				case BKTKTypeComment: {
					state = BKTKParserStateComment;
					break;
				}
				default: {
					break;
				}
			}

			break;
		}
		case BKTKParserStateArg: {
			switch (type) {
				case BKTKTypeArg: {
					state = BKTKParserStateArg;
					break;
				}
				case BKTKTypeArgSep: {
					state = BKTKParserStateArgSep;
					break;
				}
				case BKTKTypeCmdSep:
				case BKTKTypeLineBreak: {
					state = BKTKParserStateCmdSep;
					break;
				}
				case BKTKTypeGrpOpen: {
					state = BKTKParserStateGrpOpen;
					break;
				}
				case BKTKTypeGrpClose: {
					state = BKTKParserStateGrpClose;
					break;
				}
				case BKTKTypeComment: {
					state = BKTKParserStateComment;
					break;
				}
				case BKTKTypeEnd: {
					state = BKTKParserStateEnd;
					break;
				}
				default: {
					goto unexpectedError;
					break;
				}
			}
			break;
		}
		case BKTKParserStateArgSep: {
			switch (type) {
				case BKTKTypeArg: {
					state = BKTKParserStateArg;
					break;
				}
				case BKTKTypeLineBreak: {
					break;
				}
				case BKTKTypeEnd:
				default: {
					BKTKParserSetError (parser, "Expected argument following argument separator on line %u:%u",
						token -> offset.lineno, token -> offset.colno);
					goto error;
					break;
				}
			}

			break;
		}
		case BKTKParserStateError: {
			goto error;
			break;
		}
		default: {
			break;
		}
	}

	// state actions
	switch (state) {
		case BKTKParserStateCmd: {
			BKTKParserBeginCmd (parser, token);
			state = BKTKParserStateArg;
			break;
		}
		case BKTKParserStateArg: {
			BKTKParserPushArg (parser, token);
			break;
		}
		case BKTKParserStateGrpOpen: {
			if ((res = BKTKParserEndCommand (parser)) != 0) {
				goto allocationError;
			}

			BKTKParserOpenGroup (parser, token);

			state = BKTKParserStateRoot;
			break;
		}
		case BKTKParserStateGrpClose: {
			if (parser -> stackSize <= 2) {
				BKTKParserSetError (parser, "Unexpected closing group on line %u:%u",
					token -> offset.lineno, token -> offset.colno);
				goto error;
			}

			if ((res = BKTKParserEndCommand (parser)) != 0) {
				goto allocationError;
			}

			if ((res = BKTKParserCloseGroup (parser)) != 0) {
				goto allocationError;
			}

			state = BKTKParserStateRoot;
			break;
		}
		case BKTKParserStateCmdSep: {
			if ((res = BKTKParserEndCommand (parser)) != 0) {
				goto allocationError;
			}

			state = BKTKParserStateRoot;
			break;
		}
		case BKTKParserStateComment: {
			if ((res = BKTKParserEndCommand (parser)) != 0) {
				goto allocationError;
			}

			BKTKParserBeginCmd(parser, token);

			if ((res = BKTKParserEndCommand (parser)) != 0) {
				goto allocationError;
			}

			state = BKTKParserStateRoot;
			break;
		}
		case BKTKParserStateEnd: {
			if (parser -> stackSize > 2) {
				BKTKParserSetError (parser, "Unclosed groups (%u)", parser -> stackSize - 2);
				goto error;
			}

			if ((res = BKTKParserEndCommand (parser)) != 0) {
				goto allocationError;
			}

			break;
		}
		default: {
			break;
		}
	}

	parser -> state = state;

	return res;

	allocationError: {
		BKTKParserSetError (parser, "Allocation error");
		goto error;
	}

	unexpectedError: {
		BKTKParserSetError (parser, "Unexpected token '%s' on line %u:%u",
			BKStringEscape (&parser -> escapedName, (char *) token -> data) -> str,
			token -> offset.lineno, token -> offset.colno);
		goto error;
	}

	error: {
		parser -> state = BKTKParserStateError;

		return -1;
	}
}

BKInt BKTKParserPutTokens (BKTKParser * parser, BKTKToken const * tokens, BKUSize count)
{
	BKInt res;
	BKTKToken const * token;

	for (BKInt i = 0; i < count; i ++) {
		token = &tokens [i];

		if ((res = BKTKParserPutToken (parser, token)) != 0) {
			return res;
		}
	}

	return 0;
}

BKTKParserNode * BKTKParserGetNodeTree (BKTKParser * parser)
{
	return parser -> stack [0].node -> subNode;
}

BKClass const BKTKParserClass =
{
	.instanceSize = sizeof (BKTKParser),
	.dispose      = (void *) BKTKParserDispose,
};
