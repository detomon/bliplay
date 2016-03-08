#ifndef _BK_TK_TOKENIZER_H_
#define _BK_TK_TOKENIZER_H_

#include "BKTKBase.h"

#define BKTK_NUM_TOKS 8

typedef enum BKTKType BKTKType;
typedef enum BKTKState BKTKState;
typedef struct BKTKTokenizer BKTKTokenizer;
typedef struct BKTKToken BKTKToken;

typedef BKInt (* BKTKPutTokensFunc) (BKTKToken const * tokens, BKUSize count, void * arg);

/**
 * Defines a token type
 */
enum BKTKType
{
	BKTKTypeOther = 0,
	BKTKTypeArg,
	BKTKTypeString,
	BKTKTypeData,
	BKTKTypeArgSep,
	BKTKTypeCmdSep,
	BKTKTypeGrpOpen,
	BKTKTypeGrpClose,
	BKTKTypeComment,
	BKTKTypeSpace,
	BKTKTypeLineBreak,
	BKTKTypeEscape,
	BKTKTypeNone,
	BKTKTypeEnd,
};

/**
 * Defines the tokenizers internal state
 */
enum BKTKState
{
	BKTKStateRoot = 0,
	BKTKStateArg,
	BKTKStateStringStart,
	BKTKStateString,
	BKTKStateStringEsc,
	BKTKStateStringChar,
	BKTKStateCommentStart,
	BKTKStateComment,
	BKTKStateDataStart,
	BKTKStateDataEnd,
	BKTKStateData,
	BKTKStateSpace,
	BKTKStateEnd,
	BKTKStateError, // after end
};

/**
 * Defines a token in the given string
 */
struct BKTKToken
{
	BKTKType        type;
	BKUSize         dataLen;
	uint8_t const * data;
	BKTKOffset      offset;
};

/**
 * The tokenizer struct
 */
struct BKTKTokenizer
{
	BKObject   object;
	BKTKState  state;
	BKUSize    bufferLen, bufferCap;
	uint8_t  * buffer;
	BKUInt     tokensLen;
	BKUInt     acceptCount;
	BKUInt     base64Len;
	BKTKOffset offset;
	BKTKToken  tokens [BKTK_NUM_TOKS];
	uint32_t   base64Value;
	BKUInt     charCount;
	uint32_t   charValue;
};

/**
 * Initializer tokenizer
 */
extern BKInt BKTKTokenizerInit (BKTKTokenizer * tok);

/**
 * Reset tokenizer for scanning a new string
 *
 * Keeps the allocated buffer
 */
extern void BKTKTokenizerReset (BKTKTokenizer * tok);

/**
 * Parse full string or multiple partial strings
 *
 * When tokens are available, `pushToken` is called with `tokens` and the given
 * `arg`.
 *
 * `putTokens` should return 0. A value != 0 indicates an error and aborts the
 * tokenizer.
 *
 *  Call the function with `size` = 0 to terminate the tokenizer.
 */
extern BKInt BKTKTokenizerPutChars (BKTKTokenizer * tok, uint8_t const * chars, BKUSize size, BKTKPutTokensFunc putTokens, void * arg);

/**
 * Check if tokenizer is finished
 *
 * Returns 1 if tokenizer is finished or an error ocurred
 */
BK_INLINE BKInt BKTKTokenizerIsFinished (BKTKTokenizer const * tok);

/**
 * Check if tokenizer has encountered an error
 *
 * Error text is a NUL-terminated string contained in `tok -> buffer`
 */
BK_INLINE BKInt BKTKTokenizerHasError (BKTKTokenizer const * tok);


// --- Inline implementations

BK_INLINE BKInt BKTKTokenizerIsFinished (BKTKTokenizer const * tok)
{
	return tok -> state >= BKTKStateEnd;
}

BK_INLINE BKInt BKTKTokenizerHasError (BKTKTokenizer const * tok)
{
	return tok -> state == BKTKStateError;
}

#endif /* ! _BK_TK_TOKENIZER_H_ */
