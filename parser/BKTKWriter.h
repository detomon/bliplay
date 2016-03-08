#ifndef _BK_TK_WRITER_H_
#define _BK_TK_WRITER_H_

#include "BKTKParser.h"

/**
 * Writer callback
 */
typedef BKInt (* BKTKWriterWriteFunc) (void * userInfo, uint8_t const * data, BKUSize size);

/**
 * Write node
 */
extern BKInt BKTKWriterWriteNode (BKTKParserNode const * node, BKTKWriterWriteFunc write, void * userInfo, uint8_t const * indent);

#endif /* ! _BK_TK_WRITER_H_ */
