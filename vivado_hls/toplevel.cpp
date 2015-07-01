#include <stdio.h>
#include <stdlib.h>
#include <ap_int.h>

#include "toplevel.h"
#include "types.h"
#include "constants.h"

uint32 inGrid[MAX_TILE_DATA_32];
uint1 tile[MAX_TILE_DATA_SIZE];
uint8 openings[6];
uint8 directions[MAX_NUM_DIRECTIONS];

/**
 * Given a cell row and col coord, return this in 4-bit representation (like the last column)
 * Return the offsets of the cell walls in the nsweIx parameter
 */
uint8 getCell(uint8 tileSize, uint8 row, uint8 col, uint16* nwseIx) {

	bool inLastRow = false;
	bool inPenultimateRow = false;
	bool inLastColumn = col == tileSize - 1;

	if (row == tileSize - 1) {
		inLastRow = true;
	} else if (row == tileSize - 2) {
		inPenultimateRow = true;
	}

	uint8 xOffset = row * BITS_PER_CELL;
	uint16 arrIx = xOffset * tileSize + xOffset; // Add extra bits for last column

	if (inLastRow) { // Last row has extra bits
		arrIx += col * BITS_PER_LAST_CELL;
	} else {
		arrIx += col * BITS_PER_CELL;
	}

	uint16 north = arrIx;
	uint16 west = arrIx + 1;
	uint16 south;
	uint16 east = arrIx + 3;

	// Extra 2-bits for the last column
	uint16 nextRow = tileSize * BITS_PER_CELL + BITS_PER_CELL;

	if (inPenultimateRow) {
		// Last row has an extra 2 bits per cell
		south = arrIx + nextRow + col * BITS_PER_CELL;
	} else if (inLastRow) {
		south = arrIx + 2;

		// Don't use WEST values- use the cell to the left's EAST value
		east += 2;
	} else {
		// In not the last column, we need to use the value below anyway
		// In the last column, don't use the SOUTH value, use the cell below's NORTH value
		south = arrIx + nextRow;
	}

	if (inLastRow && inLastColumn) {
		east = arrIx + 3;
	}

	nwseIx[0] = north;
	nwseIx[1] = west;
	nwseIx[2] = south;
	nwseIx[3] = east;

	uint8 cell = 0;
	cell |= tile[north];
	cell <<= 1;
	cell |= tile[west];
	cell <<= 1;
	cell |= tile[south];
	cell <<= 1;
	cell |= tile[east];

	return cell;
}

/**
 * Search the sides of the tile for openings.
 * They are reported in the order NWSE. This order is very important since the
 * Microblaze relies on this when piecing together the maze.
 */
uint8 findOpenings(uint8 tileSize) {
#pragma HLS INLINE

	uint8 openingsIx = 0;
	uint8 rowLength = (tileSize * BITS_PER_CELL + BITS_PER_CELL); // Extra 2-bits for the last column

	findEntranceNorthLoop: for (uint8 i = 0; i < tileSize; i++) {
#pragma HLS LOOP_TRIPCOUNT min=8 max=16 // Min: MIN_TILE_SIZE (8) Max: MAX_TILE_SIZE (16) Actual: tileSize
		if (!tile[i * BITS_PER_CELL]) {
			openings[openingsIx++] = 0;
			openings[openingsIx++] = i;
			openings[openingsIx++] = NORTH;
		}
	}

	findEntranceWestLoop: for (uint8 i = 0; i < tileSize; i++) {
#pragma HLS LOOP_TRIPCOUNT min=8 max=16 // Min: MIN_TILE_SIZE (8) Max: MAX_TILE_SIZE (16) Actual: tileSize
		uint8 bitPos = 1;

		if (!tile[i * rowLength + bitPos]) {
			openings[openingsIx++] = i;
			openings[openingsIx++] = 0;
			openings[openingsIx++] = WEST;
		}
	}

	findEntranceSouthLoop: for (uint8 i = 0; i < tileSize; i++) {
#pragma HLS LOOP_TRIPCOUNT min=8 max=16 // Min: MIN_TILE_SIZE (8) Max: MAX_TILE_SIZE (16) Actual: tileSize
		uint8 lastRowIx = tileSize - 1;
		uint8 bitPos = 2;
		uint16 offset = rowLength * lastRowIx; // Last row

		if (!tile[offset + i * BITS_PER_LAST_CELL + bitPos]) {
			openings[openingsIx++] = lastRowIx;
			openings[openingsIx++] = i;
			openings[openingsIx++] = SOUTH;
		}
	}

	findEntranceEastLoop: for (uint8 i = 0; i < tileSize; i++) {
#pragma HLS LOOP_TRIPCOUNT min=8 max=16 // Min: MIN_TILE_SIZE (8) Max: MAX_TILE_SIZE (16) Actual: tileSize
		uint8 lastIx = tileSize - 1;
		uint8 bitPos = 3;
		uint8 bitsPerCell = i == lastIx ? BITS_PER_LAST_CELL : BITS_PER_CELL;
		uint16 offset = bitsPerCell * lastIx; // Last column

		if (!tile[offset + i * rowLength + bitPos]) {
			openings[openingsIx++] = i;
			openings[openingsIx++] = lastIx;
			openings[openingsIx++] = EAST;
		}
	}

	return openingsIx;
}

/**
 * Assuming the maze is perfect, the dead-end filling will expose a single path.
 * Walk this path and return the directions.
 */
uint16 findPath(uint8 tileSize) {
#pragma HLS INLINE

	uint16 directionIx = 0;
	int8 row = openings[0]; // Openings are ordered NORTH, WEST, SOUTH, EAST
	int8 col = openings[1];
	uint8 cellEntry = openings[2];

	// Min: 1 Max: (MAX_TILE_SIZE * MAX_TILE_SIZE) (256) Actual: (tileSize * tileSize)
	wallFollowLoop: for (uint16 i = 0; i < MAX_TILE_SIZE * MAX_TILE_SIZE; i++) {

		uint16 nwseIx[4];
		uint8 cell = getCell(tileSize, row, col, nwseIx); // Get the cell in 4-bit representation
		cell |= cellEntry; // Plug the hole so you don't go backwards

		// Invert the cell to get the exit and convert from OPEN_YYY to YYY representation
		uint8 lastDirection = 0xF0 ^ ~cell;
		directions[directionIx++] = lastDirection;

		// cellEntry is opposite of lastDirection. Set this up for the next iteration.
		if (NORTH_SOUTH & lastDirection) {
			cellEntry = NORTH_SOUTH ^ lastDirection;
		} else {
			cellEntry = EAST_WEST ^ lastDirection;
		}

		// Modify the current position in the tile
		switch (lastDirection) {
			case NORTH:
				row--;
				break;
			case WEST:
				col--;
				break;
			case SOUTH:
				row++;
				break;
			case EAST:
				col++;
				break;
		}

		// Break when you move outside the tile
		if (row == tileSize || row == -1 || col == tileSize || col == -1) {
			break;
		}
	}

	return directionIx;
}

/**
 * Read data from the AXI stream and convert from uint32 to uint1
 */
void readData(hls::stream<uint32> &in, uint8 tileDataLen) {
#pragma HLS INLINE

	readLoop: for (uint8 i = 0; i < tileDataLen; i++) {
#pragma HLS LOOP_TRIPCOUNT min=5 max=18 // Min: MIN_TILE_DATA_32 (5) Max: MAX_TILE_DATA_32 (18) Actual: tileDataLen
		inGrid[i] = in.read();

		// Convert data from 18 * 32 => 576 * 1
		readShiftLoop: for (uint8 j = 0; j < BUS_WIDTH; j++) {
#pragma HLS UNROLL

			uint16 ix = i * BUS_WIDTH + (BUS_WIDTH - j - 1); // (BUS_WIDTH - j - 1) necessary to preserve direction
			tile[ix] = (inGrid[i] & (1 << j)) ? 1 : 0;
		}
	}
}

/**
 * Do dead-end filling
 */
void findDeadEnds(uint8 tileSize) {
#pragma HLS INLINE

	fillDeadEndsLoop: for (uint16 j = 0; j < MAX_TILE_SIZE * MAX_TILE_SIZE; j++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=256 // Min: 1 Max: MAX_TILE_SIZE * MAX_TILE_SIZE (256) Actual: tileSize * tileSize
		uint16 numFilled = 0;

		cellDeadEndRowLoop: for (uint8 row = 0; row < tileSize; row++) {
#pragma HLS PIPELINE II=0
#pragma HLS LOOP_TRIPCOUNT min=8 max=16 // Min: MIN_TILE_SIZE (8) Max: MAX_TILE_SIZE (16) Actual: tileSize
			cellDeadEndColLoop: for (uint8 col = 0; col < tileSize; col++) {
#pragma HLS LOOP_FLATTEN
#pragma HLS LOOP_TRIPCOUNT min=8 max=16 // Min: MIN_TILE_SIZE (8) Max: MAX_TILE_SIZE (16) Actual: tileSize

				// Get each cell in the tile and if it has 3 walls (is a dead end) fill that dead end
				uint16 nwseIx[4];
				uint8 cell = getCell(tileSize, row, col, nwseIx);

				switch (cell) {
					case OPEN_NORTH:
						tile[nwseIx[0]] = 1;
						numFilled++;
						break;
					case OPEN_WEST:
						tile[nwseIx[1]] = 1;
						numFilled++;
						break;
					case OPEN_SOUTH:
						tile[nwseIx[2]] = 1;
						numFilled++;
						break;
					case OPEN_EAST:
						tile[nwseIx[3]] = 1;
						numFilled++;
						break;
				}
			}
		}

		// Stop if we've filled all the dead-ends
		if (numFilled == 0) {
			break;
		}
	}
}

/**
 * Write out the entrance coordinates and directions
 */
void writeEntrance(hls::stream<uint32>& out) {
#pragma HLS INLINE

	uint32 output = 0;

	output |= openings[0]; // Entrance row
	output <<= 4;
	output |= openings[1]; // Entrance column
	output <<= 4;
	output |= openings[2]; // Entrance side
	output <<= 4;
	output |= openings[3]; // Exit row
	output <<= 4;
	output |= openings[4]; // Exit col
	output <<= 4;
	output |= openings[5]; // Exit side

	out.write(output);
}

/**
 * Write the directions as u32 to the output AXI stream
 */
void writeDirections(hls::stream<uint32>& out, uint16 numDirections) {
#pragma HLS INLINE

	out.write(numDirections);

	uint16 directionIx = 0;

	writeDirectionsLoop: for (uint8 i = 0; i < MAX_NUM_DIRECTIONS / DIRECTIONS_IN_BUS; i++) {
#pragma HLS LOOP_TRIPCOUNT min=8 max=32 // Min: MIN_NUM_DIRECTIONS / DIRECTIONS_IN_BUS (8) Max: MAX_NUM_DIRECTIONS / DIRECTIONS_IN_BUS (32) Actual: numDirections
		uint32 output = 0;

		// Compress from a series of u8 to a smaller series of u32 with padding
		writeDirectionsShiftLoop: for (uint8 j = 0; j < DIRECTIONS_IN_BUS; j++) {
#pragma HLS UNROLL
			output <<= DIRECTION_SIZE;

			uint8 padChar = 0;
			output |= i * DIRECTIONS_IN_BUS + j < numDirections ? directions[directionIx + j] : padChar;
		}

		out.write(output);

		directionIx += DIRECTIONS_IN_BUS;

		if (directionIx >= numDirections) {
			break;
		}
	}
}

void toplevel(hls::stream<uint32>& in, hls::stream<uint32>& out) {

#pragma HLS INTERFACE ap_fifo port=in
#pragma HLS INTERFACE ap_fifo port=out
#pragma HLS RESOURCE variable=in core=AXI4Stream
#pragma HLS RESOURCE variable=out core=AXI4Stream
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS ARRAY_PARTITION variable=openings complete dim=1
#pragma HLS ARRAY_PARTITION variable=inGrid complete dim=1
#pragma HLS ARRAY_MAP variable=directions instance=instance1 horizontal
#pragma HLS ARRAY_MAP variable=tile instance=instance1 horizontal

	uint8 tileCoords = in.read();
	uint8 tileSize = in.read();
	uint8 tileDataLen = in.read(); // Number of 32-bit bits
	readData(in, tileDataLen);

	uint8 numOpenings = findOpenings(tileSize);
	out.write(tileCoords);
	out.write(numOpenings);

	if (numOpenings > 0) {
		writeEntrance(out);
		findDeadEnds(tileSize);
		uint16 numDirections = findPath(tileSize);
		writeDirections(out, numDirections);
	}
}
