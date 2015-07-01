#include <stdio.h>
#include "xparameters.h"
#include "xemaclite.h"

#include "main.h"
#include "ethernet.h"
#include "types.h"
#include "constants.h"
#include "util.h"

/**
 * Setup txBuf with user entered data to request tile
 */
void setupTileRequest(u8 cx, u8 cy) {

	u8* tileReqView = (u8*) (txBuf + XEL_HEADER_SIZE);

	*tileReqView++ = 0x01;

	*tileReqView++ = seed[0];
	*tileReqView++ = seed[1];
	*tileReqView++ = seed[2];
	*tileReqView++ = seed[3];

	*tileReqView++ = perfect;
	*tileReqView++ = mazeSize;
	*tileReqView++ = tileSize;

	*tileReqView++ = cx;
	*tileReqView++ = cy;
}

/**
 * Request all tiles from the server and send them for handling
 */
void getTiles(handleMazeData_t handleMazeData) {

	u8 tileReqIx;
	u8 cx, cy;

	for (tileReqIx = 0; tileReqIx < mazeSize * mazeSize; tileReqIx++) {
		cx = tileReqIx % mazeSize;
		cy = tileReqIx / mazeSize;

		setupEthFrame(txBuf, myMac, destMac);
		setupTileRequest(cx, cy);

		send(XEL_HEADER_SIZE + TILE_REQ_LEN);
		receive(0x02);

		int tileDataLen = (((short) rxBuf[24]) << 8) + rxBuf[25];

		handleMazeData(tileDataLen, cx, cy);
	}
}

/**
 * Setup the txBuf to with the solution
 */
u16 setupSolutionResponse(u8 entranceX, u8 entranceY, u16 numDirections) {

	u8* solutionResponseView = (u8*) (txBuf + XEL_HEADER_SIZE);

	*solutionResponseView++ = 0x03;

	*solutionResponseView++ = seed[0];
	*solutionResponseView++ = seed[1];
	*solutionResponseView++ = seed[2];
	*solutionResponseView++ = seed[3];

	*solutionResponseView++ = perfect;
	*solutionResponseView++ = mazeSize;
	*solutionResponseView++ = tileSize;

	*solutionResponseView++ = entranceX;
	*solutionResponseView++ = entranceY;

	numDirections--;

	*solutionResponseView++ = (numDirections & 0xFF00) >> 8;
	*solutionResponseView++ = numDirections & 0x00FF;

	u32 solutionResponseViewDirections = (int) solutionResponseView;

	int directionIx = 0;
	bool done = FALSE;
	while (!done) {
		u8 out = 0;

		// Convert the directions in the solution into u8s for sending to the server with padding
		for (int i = 0; i < 4; i++) {
			out <<= 2;
			if (directionIx != numDirections) { // Don't send last direction out of the maze
				out |= solutionDirections[directionIx++];
			} else {
				done = TRUE;
			}
		}

		*solutionResponseView++ = out;
	}

	return (int) solutionResponseView - solutionResponseViewDirections; // Return size of the solution in bytes
}

/**
 * Send the solution to the server and return the response from the server
 */
u8 sendSolution(u8 entranceX, u8 entranceY, u16 numDirections) {

	setupEthFrame();
	u16 numDirectionBytes = setupSolutionResponse(entranceX, entranceY, numDirections);
	send(XEL_HEADER_SIZE + SOLUTION_RESP_LEN + numDirectionBytes);
	receive(0x04);

	return rxBuf[15];
}
