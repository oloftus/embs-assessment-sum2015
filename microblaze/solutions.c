#include <stdio.h>
#include "xemaclite.h"
#include "fsl.h"

#include "main.h"
#include "constants.h"
#include "types.h"
#include "util.h"
#include "vga.h"
#include "visual.h"

u8 solverCoreId = 0;
u8 numSolverCores = 2;

/**
 * Alternate between the first and second solver core
 */
u8 getSolverCoreId() {

	solverCoreId++;

	if (solverCoreId == numSolverCores) {
		solverCoreId = 0;
	}

	return solverCoreId;
}

/**
 * Utility function to put things on a solver core
 */
void solverPut(u8 solverCoreId, u32 data) {

	switch (solverCoreId) {
		case 0:
			putfslx(data, 0, FSL_DEFAULT);
			break;
		case 1:
			putfslx(data, 1, FSL_DEFAULT);
			break;
	}
}


/**
 * Utility function to get things from a solver core
 */
u32 solverGet(u8 solverCoreId) {

	u32 data = 0;

	switch (solverCoreId) {
		case 0:
			getfslx(data, 0, FSL_DEFAULT);
			break;
		case 1:
			getfslx(data, 1, FSL_DEFAULT);
			break;
	}

	return data;
}

/**
 * Populate the opening parameter with the global coordinate and direction of the maze entrance
 */
void findEntrance(u8* opening) {

	u16 cellIx;

	// Search each side of the maze for an opening. Openings are search in order NWSE.
	u8 i;
	for (i = 0; i < mazeSize; i++) { // NORTH

		cellIx = i * OPENINGS_BYTES_PER_TILE;

		if (openings[cellIx + SIDE_OFFSET_1] == NORTH) {
			opening[0] = 0;
			opening[1] = i * tileSize + openings[cellIx + COL_OFFSET_1];
			opening[2] = NORTH;

			return;
		}
	}

	for (i = 0; i < mazeSize; i++) { // WEST

		cellIx = i * OPENINGS_ROW_SIZE;

		if (openings[cellIx + SIDE_OFFSET_1] == WEST) {
			opening[0] = i * tileSize + openings[cellIx + ROW_OFFSET_1];
			opening[1] = 0;
			opening[2] = WEST;

			return;
		}

		// West could be in the second position (openings ix 3,4,5)
		if (openings[cellIx + SIDE_OFFSET_2] == WEST) {
			opening[0] = i * tileSize + openings[cellIx + ROW_OFFSET_2];
			opening[1] = 0;
			opening[2] = WEST;

			return;
		}
	}

	for (i = 0; i < mazeSize; i++) { // SOUTH

		u8 lastRowIx = mazeSize - 1;
		u8 lastRowOffset = lastRowIx * OPENINGS_ROW_SIZE;
		cellIx = lastRowOffset + i * OPENINGS_BYTES_PER_TILE;

		if (openings[cellIx + SIDE_OFFSET_1] == SOUTH) {
			opening[0] = mazeSize * tileSize - 1;
			opening[1] = i * tileSize + openings[cellIx + COL_OFFSET_1];
			opening[2] = SOUTH;

			return;
		}

		if (openings[cellIx + SIDE_OFFSET_2] == SOUTH) {
			opening[0] = mazeSize * tileSize - 1;
			opening[1] = i * tileSize + openings[cellIx + COL_OFFSET_2];
			opening[2] = SOUTH;

			return;
		}
	}

	for (i = 0; i < mazeSize; i++) { // EAST

		u8 lastColOffset = (mazeSize - 1) * OPENINGS_BYTES_PER_TILE;
		cellIx = lastColOffset + i * OPENINGS_ROW_SIZE;

		if (openings[cellIx + SIDE_OFFSET_2] == EAST) {
			opening[0] = i * tileSize + openings[cellIx + ROW_OFFSET_2];
			opening[1] = mazeSize * tileSize - 1;
			opening[2] = EAST;

			return;
		}
	}
}

/**
 * Convert between internal direction representation as 4 bits to server direction representation of 2 bits
 */
u8 convertToSolutionDirection(u8 dir) {

	switch (dir) {
		case NORTH:
			return 0;
			break;
		case WEST:
			return 1;
			break;
		case SOUTH:
			return 2;
			break;
		case EAST:
			return 3;
			break;
	}

	return -1;
}

/**
 * Return the reverse of the input direction, e.g. N ->> S, E ->> W
 */
u8 invertDirection(u8 dir) {

	if (NORTH_SOUTH & dir) {
		return NORTH_SOUTH ^ dir;
	} else {
		return EAST_WEST ^ dir;
	}

	return -1;
}

/**
 * Given the directions for an individual tile and a location in the final solution array
 * insert the directions in the correct format into the final solution buffer. Invert is specified.
 */
u16 getTileDirections(volatile u32* tileDirections, u32 solutionDirectionsOffset, bool invert,
		volatile u8* tileOpenings) {

	u8* solutionDirectionsView = (u8*) (solutionDirections + solutionDirectionsOffset);
	u8* directionsBufView = (u8*) directionsBuf;
	u16 numDirections = 0;

	u16 directionIx;
	for (directionIx = 0; directionIx < (MAX_NUM_DIRECTIONS * 4) / 32; directionIx++) { // 4 bits per direction, directions split into 32 bit chunks
		bool breakLoop = FALSE;
		u32 directionSet = tileDirections[directionIx];

		u32 directionMask = 0xF0000000;

		u8 shiftIx;
		for (shiftIx = 0; shiftIx < BUS_WIDTH / DIRECTION_LENGTH; shiftIx++) {
			u32 direction = (directionSet & directionMask) >> (BUS_WIDTH / DIRECTION_LENGTH - shiftIx - 1)
					* DIRECTION_LENGTH;
			directionMask >>= DIRECTION_LENGTH;

			if (direction == DIRECTIONS_END_MARKER) {
				breakLoop = TRUE;
				break;
			}

			// If we're inverting, write to a buffer instead of the final solution array
			if (invert) {
				*directionsBufView++ = direction;
			} else {
				*solutionDirectionsView++ = convertToSolutionDirection(direction);
			}

			numDirections++;
		}

		if (breakLoop) {
			break;
		}
	}

	/*
	 * If we're inverting, read the directions from the buffer into the final solution array
	 * Since there is always a direction to move out of the tile, we need to move that direction from the
	 * previous end to the new end manually
	 */
	if (invert) {
		Xint16 directionIx;
		for (directionIx = numDirections - 2; directionIx >= 0; directionIx--) { // Don't write the last direction at the beginning
			*solutionDirectionsView++ = convertToSolutionDirection(invertDirection(*(directionsBuf + directionIx)));
		}
		*solutionDirectionsView++ = convertToSolutionDirection(tileOpenings[2]); // Instead write the last direction at the end
	}

	return numDirections;
}

/**
 * Piece together all the tile solutions into one final solution starting at entranceCoords
 */
u16 calculateSolution(u8* entranceCoords) {

	// Make global cell coords into tile coords
	Xint8 nextTileRow = entranceCoords[0] / tileSize;
	Xint8 nextTileCol = entranceCoords[1] / tileSize;

	u8 lastExitRow, lastExitCol;

	// Depending on the entrance, set the lastExitRow/-Col to the appropriate cell
	if (entranceCoords[2] & NORTH) {
		lastExitRow = tileSize - 1;
		lastExitCol = entranceCoords[1] % tileSize;
	} else if (entranceCoords[2] & WEST) {
		lastExitRow = entranceCoords[0] % tileSize;
		lastExitCol = tileSize - 1;
	} else if (entranceCoords[2] & SOUTH) {
		lastExitRow = 0;
		lastExitCol = entranceCoords[1] % tileSize;
	} else { // EAST
		lastExitRow = entranceCoords[0] % tileSize;
		lastExitCol = 0;
	}

	u16 numDirections = 0;
	volatile u32* tileDirections;
	volatile u8* tileOpenings;
	u8 opening1Row, opening1Col;

	// Repeat until you've moved out of the maze
	while (nextTileRow != mazeSize && nextTileRow != -1 && nextTileCol != mazeSize && nextTileCol != -1) {

		tileDirections = directions + U32_PER_TILE * (nextTileRow * mazeSize + nextTileCol);
		tileOpenings = openings + OPENINGS_BYTES_PER_TILE * (nextTileRow * MAX_MAZE_SIZE + nextTileCol);

		opening1Row = *(tileOpenings + ROW_OFFSET_1);
		opening1Col = *(tileOpenings + COL_OFFSET_1);

		// Decide whether you have to invert the tile directions by checking the last exit/current entrance alignment
		if ((entranceCoords[2] & NORTH && lastExitCol == opening1Col && lastExitRow == tileSize - 1 && opening1Row == 0) ||
			(entranceCoords[2] & SOUTH && lastExitCol == opening1Col && lastExitRow == 0 && opening1Row == tileSize - 1) ||
			(entranceCoords[2] & WEST && lastExitRow == opening1Row && lastExitCol == tileSize - 1 && opening1Col == 0) ||
			(entranceCoords[2] & EAST && lastExitRow == opening1Row && lastExitCol == 0 && opening1Col == tileSize - 1)) {

			numDirections += getTileDirections(tileDirections, numDirections, FALSE, tileOpenings); // No need to invert
			// Set the next entrance to the previous exit
			entranceCoords[2] = *(tileOpenings + SIDE_OFFSET_2);

			// Set up the coords of the exit cell
			lastExitRow = *(tileOpenings + ROW_OFFSET_2);
			lastExitCol = *(tileOpenings + COL_OFFSET_2);
		} else {
			numDirections += getTileDirections(tileDirections, numDirections, TRUE, tileOpenings); // Need to invert
			entranceCoords[2] = *(tileOpenings + SIDE_OFFSET_1);

			// If you inverted, your exit is what was your entrance
			lastExitRow = opening1Row;
			lastExitCol = opening1Col;
		}

		// Move the current position indicator based on which side you exited the tile from
		switch (entranceCoords[2]) {
			case NORTH:
				nextTileRow--;
				break;
			case WEST:
				nextTileCol--;
				break;
			case SOUTH:
				nextTileRow++;
				break;
			case EAST:
				nextTileCol++;
				break;
		}

		// When you move to a new cell, entrance direction is the opposite of the exit direction of the previous tile
		entranceCoords[2] = entranceCoords[2] & NORTH_SOUTH ? NORTH_SOUTH ^ entranceCoords[2] : EAST_WEST ^ entranceCoords[2];
	}

	return numDirections;
}

/**
 * When you receive a tile from the server print it and set up data structures ready to pass it to other components
 */
void handleMazeData(u32 tileDataLen, u8 cx, u8 cy) {

	u8 solverCoreId = getSolverCoreId(); // Which solver core will we put this on?

	printTile(cx, cy, rxBuf + RX_BUF_TILE_DATA_OFFSET, tileDataLen, &printToVga);

	// Put data to the solver core
	// How many bytes do we need to transport the tile data to the solver?
	int dataLen = roundUpIntDiv(tileDataLen, BYTES_IN_BUS);

	u8 tileCoord = cx;
	tileCoord <<= 4;
	tileCoord |= cy;

	solverPut(solverCoreId, tileCoord);
	solverPut(solverCoreId, tileSize);
	solverPut(solverCoreId, dataLen);

	// Compress tile data from a series of u8 into u32 for passing to the solver
	u32 tileData;
	for (u8 i = 0; i < dataLen; i++) {
		tileData = 0;

		for (u8 j = 0; j < BYTES_IN_BUS; j++) {
			tileData <<= 8;
			tileData |= rxBuf[RX_BUF_TILE_DATA_OFFSET + i * BYTES_IN_BUS + j];
		}

		solverPut(solverCoreId, tileData);
	}

	// Get data from solver core
	u32 tileCoords = solverGet(solverCoreId);
	u8 tileCol = (tileCoords & 0x000000F0) >> 4;
	u8 tileRow = tileCoords & 0x0000000F;

	u32 numEntrances = solverGet(solverCoreId);

	// If there is no path through the tile set the openings to 0 and insert the end marker in the directions array
	if (numEntrances == 0) {
		volatile u8* tileOpenings = openings + (tileRow * MAX_MAZE_SIZE + tileCol) * OPENINGS_BYTES_PER_TILE;
		for (u8 i = 0; i < OPENINGS_BYTES_PER_TILE; i++) {
			tileOpenings[i] = 0;
		}
		int directionsSetIx = (tileRow * mazeSize + tileCol) * U32_PER_TILE; // 4-bits per direction in a 32-bit int
		directions[directionsSetIx] = DIRECTIONS_END_MARKER2;

		return;
	}

	u32 entrances = solverGet(solverCoreId);

	volatile u8* tileOpenings = openings + (tileRow * MAX_MAZE_SIZE + tileCol) * OPENINGS_BYTES_PER_TILE;
	u32 entranceMask = 0x0F000000;

	// Populate tileOpenings array with tile opening data from the solver
	for (u8 i = 0; i < OPENINGS_BYTES_PER_TILE; i++) {
		entranceMask >>= 4;
		tileOpenings[i] = (entrances & entranceMask) >> (OPENINGS_BYTES_PER_TILE - i - 1) * 4;
	}

	u32 numDirections = solverGet(solverCoreId);

	int directionsSetIx = (tileRow * mazeSize + tileCol) * U32_PER_TILE; // 4-bits per direction in a 32-bit int
	int numDirectionSets = roundUpIntDiv(numDirections, 8);

	// Read the tile solution into the directions buffer converting from u32 to u8 and add an end marker
	for (int i = 0; i < numDirectionSets; i++) {
		volatile u32 directionSet = solverGet(solverCoreId);

		u32 directionsMask = 0xF0000000;
		int directionIx;
		for (directionIx = 0; directionIx < 8; directionIx++) {
			if ((directionSet & directionsMask) == 0) {
				directionSet |= DIRECTIONS_END_MARKER_MASK & directionsMask;
				break;
			}
			directionsMask >>= 4;
		}

		directions[directionsSetIx++] = directionSet;

		if (directionIx == 8 && i == numDirectionSets - 1) { // i.e. no marker bit set
			directions[directionsSetIx++] = DIRECTIONS_END_MARKER2;
		}
	}
}
