#include "types.h"

#ifndef VISUAL_H_
#define VISUAL_H_

void printTile(u8 cx, u8 cy, volatile u8* tile, u8 tileDataLen, printFunc_t printFunc);
void printToUart(u8 cx, u8 cy, char ch, bool isLast);

#endif
