#include <stdio.h>
#include "xparameters.h"
#include "xemaclite.h"
#include "fsl.h"

#include "main.h"
#include "constants.h"
#include "types.h"
#include "servercomm.h"
#include "ethernet.h"
#include "uart.h"
#include "visual.h"
#include "solutions.h"
#include "vga.h"

u8 myMac[] = { 0x00, 0x11, 0x22, 0x33, 0x00, 0x0E };
u8 destMac[] = { 0x00, 0x11, 0x22, 0x44, 0x00, 0x50 };

u8 mazeSize, tileSize;
bool perfect;
u8 seed[4];

XEmacLite eth;

volatile u8* txBuf = (u8*) TX_BUF_OFFSET;
volatile u8* rxBuf = (u8*) RX_BUF_OFFSET;
volatile u32* directions = (u32*) DIRECTIONS_OFFSET;
volatile u8* openings = (u8*) OPENINGS_OFFSET;
volatile u8* printBuf = (u8*) PRINT_BUF_OFFSET;
volatile u8* directionsBuf = (u8*) DIRECTIONS_BUF_OFFSET;
volatile u8* solutionDirections = (u8*) SOLUTIONS_DIRECTIONS_OFFSET;

int main() {

	print("\r\nMAZE SOLVER\r\n");

	setupEth();
	setupVga(VGA_BUF_OFFSET);
	clearScreen();
	getParams();
	getTiles(&handleMazeData);
	u8 entranceCoords[3]; // Global coords
	findEntrance(entranceCoords);
	u16 numDirections = calculateSolution(entranceCoords);
	printSolutionVga(numDirections, entranceCoords);
	u8 response = sendSolution(entranceCoords[1], entranceCoords[0], numDirections);

	switch (response) {
		case 0x00:
			print("\r\nWRONG");
			break;
		case 0x01:
			print("\r\nCould be shorter");
			break;
		case 0x02:
			print("\r\nCORRECT");
			break;
	}

	return 0;
}
