--- ClickerDecoderSharedMem Notes

Playing around with the BBB and a TSOP38238 IR Receiver Module
I am using an anciet Magnavox VCR controller, so the codes are probably no longer germane.
The Controller prints codes it doesn't know, so you can teach it new tricks.

This version with new BBB PRU software and uses shared memory for 2-way
communication between PRU and host
Based on examples from PRUCookbook

(Ins and Outs relative to BBB)
P9_31 = IR input
P9_29 = LED
P9_28 = a toggled test point

To Run:
source setup.sh							- to set some parameters and configure I/O pins
gcc ClickDecController.c -o Controller	- to compile Controller
make									- to compile PRU code and install and start
./Controller							- to start Controller

