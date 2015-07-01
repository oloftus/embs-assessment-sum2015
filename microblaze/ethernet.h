#ifndef ETHERNET_H_
#define ETHERNET_H_

void setupEth();
void setupEthFrame();
void receive(u8 flag);
void send(u32 len);

#endif
