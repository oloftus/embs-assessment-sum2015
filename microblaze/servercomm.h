#include <stdio.h>
#include "xparameters.h"
#include "xemaclite.h"

#include "types.h"

#ifndef SERVERCOMM_H_
#define SERVERCOMM_H_

void getTiles(handleMazeData_t handleMazeData);
u8 sendSolution(u8 entranceX, u8 entranceY, u16 numDirections);

#endif
