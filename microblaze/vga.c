#include "xemaclite.h"

#include "main.h"
#include "constants.h"
#include "types.h"

#define MODE_VERT 1
#define MODE_HORIZ 0

static u32 currX = 0;
static u32 currY = 0;

static u32 currCol = 0;
static u32 currRow = 0;

static u8 mode = MODE_HORIZ;

/**
 * Draw a rectangle on VGA
 */
void drawRect(u32 x, u32 y, u32 width, u32 height, u8 colour) {

	for (u32 yIx = y; yIx < y + height; yIx++) {
		for (u32 xIx = x; xIx < x + width; xIx++) {
			*((volatile u8 *) VGA_BUF_OFFSET + xIx + (VGA_VIEWPORT_WIDTH * yIx)) = colour;
		}
	}
}

/**
 * Clear the screen
 */
void clearScreen() {

	for (u32 i = 0; i < FRAME_BUF_SIZE; i++) {
		*((volatile u8 *) VGA_BUF_OFFSET + i) = WHITE;
	}
}

/**
 * Set up the VGA subsystem
 */
void setupVga(u32 bufferLocation) {

	*((volatile u32*) XPAR_EMBS_VGA_0_BASEADDR) = VGA_BUF_OFFSET;
	*((volatile u32*) XPAR_EMBS_VGA_0_BASEADDR + 1) = 1; // Enable
}

/**
 * Draw a small piece of a maze tile
 * Called with a character that represents which bit of a maze to draw with coords as to where to draw it
 * This is passed as function handle for the maze printing function that works for VGA and UART
 */
void printToVga(u8 cx, u8 cy, char ch, bool isLast) {

	u32 tileOffsetY = cy * tileSize * (CELL_WIDTH + BORDER_WIDTH);
	u32 tileOffsetX = cx * tileSize * (CELL_WIDTH + BORDER_WIDTH);

	switch (ch) {
		case CORNER:
			drawRect(tileOffsetX + currX, tileOffsetY + currY, BORDER_WIDTH, BORDER_WIDTH, BLACK);
			currX += BORDER_WIDTH;
			break;
		case HORIZ_LINE:
			drawRect(tileOffsetX + currX, tileOffsetY + currY, CELL_WIDTH, BORDER_WIDTH, BLACK);
			currX += CELL_WIDTH;
			break;
		case VERT_LINE:
			drawRect(tileOffsetX + currX, tileOffsetY + currY, BORDER_WIDTH, CELL_WIDTH, BLACK);
			currX += BORDER_WIDTH;
			break;
		case SPACE:
		case HORIZ_GAP:
			currX += CELL_WIDTH;
			break;
		case VERT_GAP:
			currX += BORDER_WIDTH;
			break;
		case RETURN:
			currX = 0;
			currY += mode == MODE_VERT ? CELL_WIDTH : BORDER_WIDTH;
			mode = mode == MODE_VERT ? MODE_HORIZ : MODE_VERT;
			break;
	}

	// If this is the last character of a tile, reset the positions and mode
	if (isLast) {
		currX = 0;
		currY = 0;
		mode = MODE_HORIZ;
	}
}

/**
 * Draw the solution directions on the maze tiles in an overlay
 */
void printSolutionVga(u16 numDirections, u8* entranceCoords) {

	currRow = entranceCoords[0];
	currCol = entranceCoords[1];

	for (int i = 0; i < numDirections ; i++) {
		currX = BORDER_WIDTH + currCol * (CELL_WIDTH + BORDER_WIDTH);
		currY = BORDER_WIDTH + currRow * (CELL_WIDTH + BORDER_WIDTH);

		switch (solutionDirections[i]) { // Neils direction format
			case 0:
				currRow--;
				break;
			case 1:
				currCol--;
				break;
			case 2:
				currRow++;
				break;
			case 3:
				currCol++;
				break;
		}

		drawRect(currX, currY, CELL_WIDTH, CELL_WIDTH, GREEN);
	}
}
