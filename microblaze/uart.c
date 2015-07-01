#include "xparameters.h"
#include "xuartlite_l.h"

#include "main.h"
#include "constants.h"
#include "types.h"

/**
 * Read a yYnN type response from the user
 */
bool readUartYesNo() {

	char result = -1;
	char inputChar;

	while (FOREVER) {
		inputChar = XUartLite_RecvByte(XPAR_RS232_DTE_BASEADDR);

		if (inputChar == '\n' || inputChar == '\r') {
			if (result == -1) {
				continue;
			}

			return result;
		}

		switch (inputChar) {
			case 'y':
			case 'Y':
				result = TRUE;
				break;
			case 'n':
			case 'N':
				result = FALSE;
				break;
		}
	}

	return -1;
}

/**
 * Read an int from the user
 */
u32 readUartInt() {

	u32 integer = 0;
	char inputChar;

	while (FOREVER) {
		inputChar = XUartLite_RecvByte(XPAR_RS232_DTE_BASEADDR);

		if (inputChar == '\n' || inputChar == '\r') {
			break;
		}

		int digit = inputChar - '0';
		integer = (integer * 10) + digit;
	}

	return integer;
}

/**
 * Get the parameters from the user
 */
void getParams() {

	print("\r\nMaze size:\r\n");
	mazeSize = readUartInt();

	print("\r\nTile Size:\r\n");
	tileSize = readUartInt();

	print("\r\nSeed:\r\n");
	u32 seedNum = readUartInt();

	seed[3] = (seedNum >> 0 * 8) & 0xFF;
	seed[2] = (seedNum >> 1 * 8) & 0xFF;
	seed[1] = (seedNum >> 2 * 8) & 0xFF;
	seed[0] = (seedNum >> 3 * 8) & 0xFF;

	print("\r\nPerfect?:\r\n");
	perfect = readUartYesNo();

	if (!perfect) {
		print("\r\nThis solver only support perfect mazes. Will send request with PERFECT=TRUE instead.\r\n");
		perfect = TRUE;
	}
}

