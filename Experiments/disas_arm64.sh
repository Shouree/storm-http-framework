#!/bin/bash

# Disassemble hex from command line.
truncate --size 0 /tmp/blob
echo -ne "$(echo " $@" | sed -E 's/ +(..)/\\x\1/g')" > /tmp/blob
objdump -D -b binary -maarch64 /tmp/blob
rm /tmp/blob
