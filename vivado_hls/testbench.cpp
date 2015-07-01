#include "toplevel.h"
#include "constants.h"
#include "types.h"

int roundUpIntDiv(int num, int denom) {

	int i = 0;
	while (num > 0) {
		num -= denom;
		i++;
	}

	return i;
}

int main() {
	hls::stream<uint32> in("in");
	hls::stream<uint32> out("out");

	int tileSize = 16;
	int tileDataLen = 72;

	uint8 cx = 7;
	uint8 cy = 7;

	uint8 tileCoord = cx; // Max 7, i.e. 0000 0111
	tileCoord <<= 4;
	tileCoord |= cy;

	uint8 tileData[MAX_TILE_DATA_SIZE] = { 0xFA, 0xAC, 0xBA, 0xBA, 0x46, 0xE5, 0xA3, 0x5C, 0x77, 0x14, 0x6E, 0x15, 0xA5,
			0x37, 0xA4, 0xC5, 0xE5, 0xE1, 0x36, 0xDC, 0x44, 0x5A, 0xA3, 0x15, 0xEA, 0x77, 0x9E, 0x70, 0x4A, 0x65, 0x15,
			0x4E, 0xAB, 0xA1, 0xF1, 0x38, 0xE9, 0x69, 0xD6, 0xB3, 0xA3, 0x19, 0x96, 0x86, 0xE1, 0x6D, 0xCD, 0xBA, 0x1A,
			0xA1, 0x19, 0xC5, 0xAE, 0x9B, 0x29, 0x5C, 0xF1, 0xCC, 0x79, 0x56, 0x46, 0x1C, 0xC5, 0x97, 0x8A, 0xAA, 0xAA,
			0x8D, 0xAA, 0xA8, 0xF8, 0x8C };


	int dataLen = roundUpIntDiv(tileDataLen, BYTES_IN_BUS);

	in.write(tileCoord);
	in.write(tileSize);
	in.write(dataLen);

	uint32 inVal;
	for (int i = 0; i < dataLen; i++) {
		inVal = 0;

		for (int j = 0; j < BYTES_IN_BUS; j++) {
			inVal <<= 8;
			inVal |= tileData[i * BYTES_IN_BUS + j];
		}

		in.write(inVal);
	}

	toplevel(in, out);

	printf("\r\n");

	int tileCoords = out.read();
	printf("tileCoords: 0x%x\r\n", tileCoords);

	int numEntrances = out.read();
	printf("numEntrances: 0x%x\r\n", numEntrances);

	if (numEntrances == 0) {
		return 0;
	}

	int openings = out.read();
	printf("openings: 0x%x\r\n", openings);

	int numDirections = out.read();
	printf("numDirections: 0x%x\r\n", numDirections);

	printf("directions: ");
	int direction;
	for (int i = 0; i < roundUpIntDiv(numDirections, 8); i++) {
		direction = out.read();
		printf("0x%x ", direction);
	}

	printf("\r\n");
	printf("\r\n");

	return 0;
}
