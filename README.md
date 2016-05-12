# PMiner - Stratum Bitcoin Miner

Pminer is intended to be a light and fast bitcoin minner.

#### Dependencies for the miner:
* Jansson

#### Dependencies for the tests:
* Cmocka
* Openssl

#### Installing all dependencies:
* sudo apt-get install libcmocka-dev libssl-dev libjansson-dev

#### Features:
* SHA256d library in C and ASM, optimized to run with SSE2.
* Multi-threaded mining
* TODO: Interface to communicate with a Xillin FPGA board.
* SHA256 Verilog for mining with Xillin FPGA boards.

#### Usage:
* -h hostname:port 
* -u username 
* -p password 
* -c cores 

#### Example:
./main -h us1.ghash.io:3333 -u up102073465.worker1 -p pass -c 4