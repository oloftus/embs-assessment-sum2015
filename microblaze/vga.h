#include "types.h"

#ifndef VGATEST1_H_
#define VGATEST1_H_

void setupVga(u32 bufferLocation);
void clearScreen();
void printToVga(u8 cx, u8 cy, char ch, bool isLast);
void printSolutionVga(u16 numDirections, u8* entranceCoords);

#endif
