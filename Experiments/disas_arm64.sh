#!/bin/bash

# Disassemble hex from command line.
truncate --size 0 /tmp/blob
echo "0:" "$@" | xxd -r - /tmp/blob
objdump -D -b binary -maarch64 /tmp/blob
rm /tmp/blob
