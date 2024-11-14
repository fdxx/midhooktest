#!/bin/bash

if [ -z "$1" ]; then
  echo "$0 file.asm"
  exit 1
fi

ASM_FILE="$1"

nasm -f bin "$ASM_FILE" -o "$ASM_FILE.o"
hexdump -v -e '1/1 "0x%02X, "' "$ASM_FILE.o" | sed 's/, $/\n/'

nasm -f elf32 "$ASM_FILE" -o "$ASM_FILE.o"
objdump -d "$ASM_FILE.o"

rm -rf "$ASM_FILE.o"

