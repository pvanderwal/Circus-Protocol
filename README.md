# Circus-Protocol

Circus protocol is designed to use the built in serial ports on Arduino's etc. so it requires no additional hardware,m other than Cat-5 cable. Cat-5 isn;t strictly required, but using Cat-5(or 6) cable allows you to send data and power (at various voltages) on the same cable and run it back to a central hub that handles data and power distribution (multiple voltages), plus you can connect one wire to the reset pin which allows you to reprogram individual nodes in place from the central hub.

It's designed to work as a ring, with the transmit of one micro-processor connected to the receive of then next, and so on. The ring starts and ends at a "Ring Master" (hence the name)
If you use a Mega 2560 as the Ring Master you can have a Three Ring Circus (ha ha)

Current version uses a simple 4 byte token, 2 bytes payload, 1 byte is a combination target address and command (either read from, or write to register), and 1 byte CRC-8

It supports 15 nodes per ring, each node has 8 registers that are two bytes each, the protocol can read or write to any of the 8 registers.

First register is for general purpose control of the node, turn the node on/off, enable/disable individual timers, etc.

Up to four of the registers can be used as a 16 bit timers. The library includes an optional "Tic" timer that divides a day up into 65,536 Tics, each tic is about 1.38 seconds long.

It also includes an optional counter register. The library can automatically configure a hardware interrupt or setup a pin with configurable software debounce to increment the counter.

Any register not used as a timer or counter can be a general purpose data register.

Finally the last register can be used as the Tic timer which allows the Ring Master to synchronize the time in all the nodes with one token

Optionally you can designate one or more of the node addresses to be a group address instead, this lets you send a single command to a group of nodes at once (all lights on/off for example)

Note: the code is currently very experimental.
