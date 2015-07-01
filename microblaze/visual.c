#include <stdio.h>
#include "xparameters.h"
#include "xemaclite.h"
#include "xuartlite_l.h"

#include "main.h"
#include "types.h"
#include "constants.h"

/**
 * Draw the horizontal part of a tile row
 */
void printHorizontal(u8 cx, u8 cy, bool isLastLine, printFunc_t printFunc) {

	u8 mul = 2;
	if (isLastLine) {
		mul = 4;
	}

	for (u8 col = 0; col < tileSize; col++) {
		printFunc(cx, cy, CORNER, FALSE);
		if (printBuf[col * mul]) {
			printFunc(cx, cy, HORIZ_LINE, FALSE);
		} else {
			printFunc(cx, cy, HORIZ_GAP, FALSE);
		}
	}
	printFunc(cx, cy, CORNER, FALSE);
	printFunc(cx, cy, RETURN, FALSE);
}

/**
 * Draw the vertical part of a tile row
 */
void printVertical(u8 cx, u8 cy, bool isLastLine, printFunc_t printFunc) {

	u8 mul = 2;
	if (isLastLine) {
		mul = 4;
	}

	for (u8 col = 0; col < tileSize + 1; col++) {

		u8 ix = col * mul + 1;
		if (col == tileSize && isLastLine) {
			ix = col * mul - 1;
		}

		if (printBuf[ix]) {
			printFunc(cx, cy, VERT_LINE, FALSE);
		} else {
			printFunc(cx, cy, VERT_GAP, FALSE);
		}
		printFunc(cx, cy, SPACE, FALSE);
	}
	printFunc(cx, cy, RETURN, FALSE);
}

/**
 * Draw the horizontal part of a last tile row
 */
void printLastLine(u8 cx, u8 cy, printFunc_t printFunc) {

	u8 i;
	for (i = 0; i < tileSize; i++) {
		printFunc(cx, cy, CORNER, FALSE);
		if (printBuf[i * 4 + 2]) {
			printFunc(cx, cy, HORIZ_LINE, FALSE);
		} else {
			printFunc(cx, cy, HORIZ_GAP, FALSE);
		}
	}
	printFunc(cx, cy, CORNER, FALSE);
	printFunc(cx, cy, RETURN, TRUE);
}

/**
 * Print a tile. You must pass a handler that actually writes to an output source. VGA and UART are supported.
 */
void printTile(u8 cx, u8 cy, volatile u8* tile, u8 tileDataLen, printFunc_t printFunc) {

	u8 printBufSize = 0;
	u8 datum, selector;
	u8 datumIx, selectorIx;
	u8 cellRow = 0;

	// Write the pieces of each tile cell individually
	for (datumIx = 0; datumIx < tileDataLen; datumIx++) {
		datum = tile[datumIx];
		selector = 0x80; // 1000 0000

		for (selectorIx = 0; selectorIx < 8; selectorIx++) {
			printBuf[printBufSize] = (datum & selector) ? 1 : 0;
			selector >>= 1;
			printBufSize++;

			// Print the last row of the tile
			if (cellRow == tileSize - 1 && printBufSize == tileSize * BITS_PER_LAST_CELL) {
				printHorizontal(cx, cy, TRUE, printFunc);
				printVertical(cx, cy, TRUE, printFunc);
				printLastLine(cx, cy, printFunc);

				printBufSize = 0;
				cellRow++;
			} else if (cellRow < tileSize - 1 && printBufSize == (tileSize - 1) * BITS_PER_CELL + BITS_PER_LAST_CELL) {
				// Print rows of the tile.
				printHorizontal(cx, cy, FALSE, printFunc);
				printVertical(cx, cy, FALSE, printFunc);

				printBufSize = 0;
				cellRow++;
			}
		}
	}
}

/**
 * Print handler that writes a maze to the UART
 */
void printToUart(u8 cx, u8 cy, char ch, bool isLast) {

	switch (ch) {
		case CORNER:
			print(CORNER_UART);
			break;
		case HORIZ_LINE:
			print(HORIZ_LINE_UART);
			print(HORIZ_LINE_UART);
			print(HORIZ_LINE_UART);
			break;
		case HORIZ_GAP:
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, HORIZ_GAP_UART);
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, HORIZ_GAP_UART);
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, HORIZ_GAP_UART);
			break;
		case VERT_LINE:
			print(VERT_LINE_UART);
			break;
		case VERT_GAP:
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, VERT_GAP_UART);
			break;
		case SPACE:
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, SPACE);
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, SPACE);
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, SPACE);
			break;
		case RETURN:
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, RETURN);
			XUartLite_SendByte(XPAR_RS232_DTE_BASEADDR, '\n');
			break;
	}

	if (isLast) {
		print("\r\n");
	}
}
