#include "xemaclite.h"

#include "types.h"

#ifndef MAIN_H_
#define MAIN_H_

extern u8 myMac[];
extern u8 destMac[];
extern u8 mazeSize;
extern u8 tileSize;
extern bool perfect;
extern u8 seed[4];
extern XEmacLite eth;

extern volatile u8* txBuf;
extern volatile u8* rxBuf;
extern volatile u8* openings; // Openings buffered from solver core
extern volatile u32* directions; // Directions buffered from solver core
extern volatile u8* directionsBuf; // Temporary buffer for when inverting tile directions
extern volatile u8* solutionDirections; // Final set of directions
extern volatile u8* printBuf;

#endif
