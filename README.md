# PMiner - Stratum Bitcoin Miner

Pminer is intended to be a light and fast bitcoin minner.

Libraries required:
* Jansson
* Cmocka
* openssl -> for the tests

Features:
* SHA256d library in C and ASM, optimized to run with SSE2.
* Multi-threaded mining
* TODO: Interface to communicate with a Xillin FPGA board.
* SHA256 Verilog for mining with Xillin FPGA boards.