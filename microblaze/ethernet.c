#include "xparameters.h"
#include "xemaclite.h"

#include "main.h"
#include "ethernet.h"
#include "constants.h"
#include "types.h"

#define CMP_WINDOW_SIZE 2 * MAC_LEN + 3
#define TYPE_1 0x55
#define TYPE_2 0xAB
#define MAC_LEN 6

/**
 * Set the ethernet frameheader
 */
void setupEthFrame() {

	volatile u8* headerView = txBuf;

	// Destination address
	int destMacIx;
	for (destMacIx = 0; destMacIx < MAC_LEN; destMacIx++) {
		*headerView++ = destMac[destMacIx];
	}

	// Source address
	int srcMaxIx;
	for (srcMaxIx = 0; srcMaxIx < MAC_LEN; srcMaxIx++) {
		*headerView++ = myMac[srcMaxIx];
	}

	// Type
	*headerView++ = TYPE_1;
	*headerView++ = TYPE_2;
}

/**
 * Set up Ethernet subsystem
 */
void setupEth() {

	XEmacLite_Config* ethConf = XEmacLite_LookupConfig(XPAR_EMACLITE_0_DEVICE_ID);
	XEmacLite_CfgInitialize(&eth, ethConf, ethConf->BaseAddress);
	XEmacLite_SetMacAddress(&eth, myMac);
	XEmacLite_FlushReceive(&eth);
}

/**
 * Wait to receive a packet from the server with the specified flag (bit 14)
 */
void receive(u8 flag) {

	u8 cmpWindow[CMP_WINDOW_SIZE];

	volatile u16 rxLen = 0;
	while (rxLen == 0) {
		rxLen = XEmacLite_Recv(&eth, rxBuf);

		if (rxLen == 0) {
			continue;
		}

		int cmpWindowIx = 0;

		int myMacIx;
		for (myMacIx = 0; myMacIx < MAC_LEN; myMacIx++) {
			cmpWindow[cmpWindowIx] = myMac[myMacIx];
			cmpWindowIx++;
		}

		int destMacIx;
		for (destMacIx = 0; destMacIx < MAC_LEN; destMacIx++) {
			cmpWindow[cmpWindowIx] = destMac[destMacIx];
			cmpWindowIx++;
		}

		cmpWindow[cmpWindowIx++] = TYPE_1;
		cmpWindow[cmpWindowIx++] = TYPE_2;
		cmpWindow[cmpWindowIx++] = flag;

		bool acceptFrame = TRUE;
		int frameIx;
		for (frameIx = 0; frameIx < CMP_WINDOW_SIZE; frameIx++) {
			if (rxBuf[frameIx] != cmpWindow[frameIx]) {
				acceptFrame = FALSE;
				break;
			}
		}

		if (acceptFrame) {
			return;
		}

		rxLen = 0;
	}
}

/**
 * Send the content of the txBuf to the server
 */
void send(u32 len) {

	XEmacLite_FlushReceive(&eth);
	XEmacLite_Send(&eth, txBuf, len);
}
