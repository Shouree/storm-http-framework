#!/bin/bash

# Disassemble the hex info provided on command line.

# Convert from hex to binary.
truncate --size 0 /tmp/blob
echo "0:" "$@" | xxd -r - /tmp/blob

# Disassemble the output.
objdump -D -b binary -mi386:x86-64 -Msuffix /tmp/blob

rm /tmp/blob
